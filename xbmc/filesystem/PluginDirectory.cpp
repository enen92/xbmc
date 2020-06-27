/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PluginDirectory.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/PluginSource.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

std::map<int, CPluginDirectory *> CPluginDirectory::globalHandles;
int CPluginDirectory::handleCounter = 0;
CCriticalSection CPluginDirectory::m_handleLock;

AsyncGetPluginResultAction::AsyncGetPluginResultAction(const std::string& strPath,
                                                       CFileItem& resultItem,
                                                       const bool resume)
  : m_path(strPath), m_item(&resultItem), m_resume(resume)
{
  m_event.Reset();
  m_cancelled = false;
}

bool AsyncGetPluginResultAction::ExecutionHadSuccess() const
{
  return m_success;
}

void AsyncGetPluginResultAction::Cancel()
{
  m_cancelled = true;
}

bool AsyncGetPluginResultAction::ExecuteSynchronously()
{
  bool success = m_pluginDirHandler.ExecuteScriptAndWaitOnResult(m_path, m_resume);
  if (success)
  {
    m_pluginDirHandler.UpdateResultItem(*m_item, m_pluginDirHandler.m_fileResult);
  }
  return success;
}

void AsyncGetPluginResultAction::Run()
{
  const auto& scriptExecutionInfo = m_pluginDirHandler.TriggerScriptExecution(m_path, m_resume);

  // if the script executes quick enough there's no need to launch the observer
  if (!m_pluginDirHandler.m_fetchComplete.WaitMSec(20))
  {
    // launch an observer for the running script
    CPluginDirectory::CScriptObserver scriptObs(scriptExecutionInfo.Id, m_event);

    // let the script run until cancel is flagged or the execution ends
    while (!m_cancelled && !m_event.Signaled() && !m_pluginDirHandler.m_fetchComplete.WaitMSec(20))
      ;

    // Force stop the running script in case it was manually cancelled
    if (m_cancelled)
      m_pluginDirHandler.ForceStopRunningScript(scriptExecutionInfo);

    // Finish and abort the observer
    Finish();
    scriptObs.Abort();
  }
  else
  {
    Finish();
  }
}

void AsyncGetPluginResultAction::Finish()
{
  // set the action sucess
  m_success = m_pluginDirHandler.m_success && !m_cancelled;

  // update result item
  if (m_success)
    m_pluginDirHandler.UpdateResultItem(*m_item, m_pluginDirHandler.m_fileResult);

  // flag the action event
  m_event.Set();
}

CPluginDirectory::CScriptObserver::CScriptObserver(int scriptId, CEvent &event) :
  CThread("scriptobs"), m_scriptId(scriptId), m_event(event)
{
  Create();
}

void CPluginDirectory::CScriptObserver::Process()
{
  while (!m_bStop)
  {
    if (!CScriptInvocationManager::GetInstance().IsRunning(m_scriptId))
    {
      m_event.Set();
      break;
    }
    CThread::Sleep(20);
  }
}

void CPluginDirectory::CScriptObserver::Abort()
{
  // will wait until thread exits
  StopThread();
}

CPluginDirectory::CPluginDirectory()
  : m_fetchComplete(true)
  , m_cancelled(false)
{
  m_listItems = new CFileItemList;
  m_fileResult = new CFileItem;
}

CPluginDirectory::~CPluginDirectory(void)
{
  delete m_listItems;
  delete m_fileResult;
}

int CPluginDirectory::getNewHandle(CPluginDirectory *cp)
{
  CSingleLock lock(m_handleLock);
  int handle = ++handleCounter;
  globalHandles[handle] = cp;
  return handle;
}

void CPluginDirectory::reuseHandle(int handle, CPluginDirectory* cp)
{
  CSingleLock lock(m_handleLock);
  globalHandles[handle] = cp;
}

void CPluginDirectory::removeHandle(int handle)
{
  CSingleLock lock(m_handleLock);
  if (!globalHandles.erase(handle))
    CLog::Log(LOGWARNING, "Attempt to erase invalid handle %i", handle);
}

CPluginDirectory *CPluginDirectory::dirFromHandle(int handle)
{
  CSingleLock lock(m_handleLock);
  std::map<int, CPluginDirectory *>::iterator i = globalHandles.find(handle);
  if (i != globalHandles.end())
    return i->second;
  CLog::Log(LOGWARNING, "Attempt to use invalid handle %i", handle);
  return NULL;
}

const CPluginDirectory::SCRIPT_EXECUTION_INFO CPluginDirectory::TriggerScriptExecution(
    const std::string& strPath, const bool resume)
{
  SCRIPT_EXECUTION_INFO scriptExecutionInfo;

  CURL url(strPath);
  // try the plugin type first, and if not found, try an unknown type
  if (!CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), m_addon, ADDON_PLUGIN) &&
      !CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), m_addon, ADDON_UNKNOWN) &&
      !CAddonInstaller::GetInstance().InstallModal(url.GetHostName(), m_addon))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return scriptExecutionInfo;
  }

  // get options
  std::string options = url.GetOptions();
  url.SetOptions(""); // do this because we can then use the url to generate the basepath
                      // which is passed to the plugin (and represents the share)

  std::string basePath(url.Get());
  // reset our wait event, and grab a new handle
  m_fetchComplete.Reset();
  int handle = CScriptInvocationManager::GetInstance().GetReusablePluginHandle(m_addon->LibPath());

  if (handle < 0)
    handle = getNewHandle(this);
  else
    reuseHandle(handle, this);

  // clear out our status variables
  m_fileResult->Reset();
  m_listItems->Clear();
  m_listItems->SetPath(strPath);
  m_listItems->SetLabel(m_addon->Name());
  m_cancelled = false;
  m_success = false;
  m_totalItems = 0;

  // setup our parameters to send the script
  std::string strHandle = StringUtils::Format("%i", handle);
  std::vector<std::string> argv;
  argv.push_back(basePath);
  argv.push_back(strHandle);
  argv.push_back(options);

  std::string strResume = "resume:false";
  if (resume)
    strResume = "resume:true";
  argv.push_back(strResume);

  // run the script
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s','%s')", __FUNCTION__, m_addon->Name().c_str(), argv[0].c_str(), argv[1].c_str(), argv[2].c_str(), argv[3].c_str());
  std::string file = m_addon->LibPath();
  bool reuseLanguageInvoker = false;
  if (m_addon->ExtraInfo().find("reuselanguageinvoker") != m_addon->ExtraInfo().end())
    reuseLanguageInvoker = m_addon->ExtraInfo().at("reuselanguageinvoker") == "true";

  int id = CScriptInvocationManager::GetInstance().ExecuteAsync(file, m_addon, argv,
                                                                reuseLanguageInvoker, handle);

  scriptExecutionInfo.Handle = handle;
  scriptExecutionInfo.Id = id;
  return scriptExecutionInfo;
}

bool CPluginDirectory::ExecuteScriptAndWaitOnResult(const std::string& strPath, const bool resume)
{
  const auto& scriptExecutionInfo = TriggerScriptExecution(strPath, resume);
  bool success = false;

  // verify a valid id was assigned to the script execution
  if (scriptExecutionInfo.Id < 0)
  {
    if (m_addon)
      CLog::Log(LOGERROR, "Unable to run plugin %s", m_addon->Name().c_str());
    return false;
  }

  // Wait for directory fetch to complete, end, or be cancelled
  while (!m_cancelled &&
         CScriptInvocationManager::GetInstance().IsRunning(scriptExecutionInfo.Id) &&
         !m_fetchComplete.WaitMSec(20))
    ;

  // wait for 30 secs for script to finish and force stop it if has been canceled
  WaitForScriptToFinish(scriptExecutionInfo, 30000, m_cancelled);
  success = !m_cancelled && m_success;

  // free our handle and return the success of the operation
  removeHandle(scriptExecutionInfo.Handle);
  return success;
}

void CPluginDirectory::UpdateResultItem(CFileItem& finalItem, const CFileItem* resultItem)
{
  // update the play path and metadata, saving the old one as needed
  if (!finalItem.HasProperty("original_listitem_url"))
    finalItem.SetProperty("original_listitem_url", finalItem.GetPath());

  finalItem.SetDynPath(resultItem->GetPath());
  finalItem.SetMimeType(resultItem->GetMimeType());
  finalItem.SetContentLookup(resultItem->ContentLookup());
  finalItem.UpdateInfo(*resultItem);

  if (resultItem->HasVideoInfoTag() && resultItem->GetVideoInfoTag()->GetResumePoint().IsSet())
    finalItem.m_lStartOffset =
        STARTOFFSET_RESUME; // resume point set in the resume item, so force resume
}

bool CPluginDirectory::AddItem(int handle, const CFileItem *item, int totalItems)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return false;

  CFileItemPtr pItem(new CFileItem(*item));
  dir->m_listItems->Add(pItem);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

bool CPluginDirectory::AddItems(int handle, const CFileItemList *items, int totalItems)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return false;

  CFileItemList pItemList;
  pItemList.Copy(*items);
  dir->m_listItems->Append(pItemList);
  dir->m_totalItems = totalItems;

  return !dir->m_cancelled;
}

void CPluginDirectory::EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;

  // set cache to disc
  dir->m_listItems->SetCacheToDisc(cacheToDisc ? CFileItemList::CACHE_IF_SLOW : CFileItemList::CACHE_NEVER);

  dir->m_success = success;
  dir->m_listItems->SetReplaceListing(replaceListing);

  if (!dir->m_listItems->HasSortDetails())
    dir->m_listItems->AddSortMethod(SortByNone, 552, LABEL_MASKS("%L", "%D"));

  // set the event to mark that we're done
  dir->m_fetchComplete.Set();
}

void CPluginDirectory::AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &label2Mask)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;

  //! @todo Add all sort methods and fix which labels go on the right or left
  switch(sortMethod)
  {
    case SORT_METHOD_LABEL:
    case SORT_METHOD_LABEL_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByLabel, 551, LABEL_MASKS("%T", label2Mask), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_TITLE:
    case SORT_METHOD_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByTitle, 556, LABEL_MASKS("%T", label2Mask), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_ARTIST:
    case SORT_METHOD_ARTIST_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByArtist, 557, LABEL_MASKS("%T", "%A", "%T", "%A"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_ALBUM:
    case SORT_METHOD_ALBUM_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByAlbum, 558, LABEL_MASKS("%T", "%B", "%T", "%B"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_DATE:
      {
        dir->m_listItems->AddSortMethod(SortByDate, 552, LABEL_MASKS("%T", "%J", "%T", "%J"));
        break;
      }
    case SORT_METHOD_BITRATE:
      {
        dir->m_listItems->AddSortMethod(SortByBitrate, 623, LABEL_MASKS("%T", "%X", "%T", "%X"));
        break;
      }
    case SORT_METHOD_SIZE:
      {
        dir->m_listItems->AddSortMethod(SortBySize, 553, LABEL_MASKS("%T", "%I", "%T", "%I"));
        break;
      }
    case SORT_METHOD_FILE:
      {
        dir->m_listItems->AddSortMethod(SortByFile, 561, LABEL_MASKS("%T", label2Mask, "%T", label2Mask));
        break;
      }
    case SORT_METHOD_TRACKNUM:
      {
        dir->m_listItems->AddSortMethod(SortByTrackNumber, 554, LABEL_MASKS("[%N. ]%T", label2Mask, "[%N. ]%T", label2Mask));
        break;
      }
    case SORT_METHOD_DURATION:
    case SORT_METHOD_VIDEO_RUNTIME:
      {
        dir->m_listItems->AddSortMethod(SortByTime, 180, LABEL_MASKS("%T", "%D", "%T", "%D"));
        break;
      }
    case SORT_METHOD_VIDEO_RATING:
    case SORT_METHOD_SONG_RATING:
      {
        dir->m_listItems->AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));
        break;
      }
    case SORT_METHOD_YEAR:
      {
        dir->m_listItems->AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));
        break;
      }
    case SORT_METHOD_GENRE:
      {
        dir->m_listItems->AddSortMethod(SortByGenre, 515, LABEL_MASKS("%T", "%G", "%T", "%G"));
        break;
      }
    case SORT_METHOD_COUNTRY:
      {
        dir->m_listItems->AddSortMethod(SortByCountry, 574, LABEL_MASKS("%T", "%G", "%T", "%G"));
        break;
      }
    case SORT_METHOD_VIDEO_TITLE:
      {
        dir->m_listItems->AddSortMethod(SortByTitle, 369, LABEL_MASKS("%T", "%M", "%T", "%M"));
        break;
      }
    case SORT_METHOD_VIDEO_SORT_TITLE:
    case SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortBySortTitle, 556, LABEL_MASKS("%T", "%M", "%T", "%M"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_MPAA_RATING:
      {
        dir->m_listItems->AddSortMethod(SortByMPAA, 20074, LABEL_MASKS("%T", "%O", "%T", "%O"));
        break;
      }
    case SORT_METHOD_STUDIO:
    case SORT_METHOD_STUDIO_IGNORE_THE:
      {
        dir->m_listItems->AddSortMethod(SortByStudio, 572, LABEL_MASKS("%T", "%U", "%T", "%U"), CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
        break;
      }
    case SORT_METHOD_PROGRAM_COUNT:
      {
        dir->m_listItems->AddSortMethod(SortByProgramCount, 567, LABEL_MASKS("%T", "%C", "%T", "%C"));
        break;
      }
    case SORT_METHOD_UNSORTED:
      {
        dir->m_listItems->AddSortMethod(SortByNone, 571, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_NONE:
      {
        dir->m_listItems->AddSortMethod(SortByNone, 552, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_DRIVE_TYPE:
      {
        dir->m_listItems->AddSortMethod(SortByDriveType, 564, LABEL_MASKS()); // Preformatted
        break;
      }
    case SORT_METHOD_PLAYLIST_ORDER:
      {
        std::string strTrack=CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_MUSICFILES_TRACKFORMAT);
        dir->m_listItems->AddSortMethod(SortByPlaylistOrder, 559, LABEL_MASKS(strTrack, "%D"));
        break;
      }
    case SORT_METHOD_EPISODE:
      {
        dir->m_listItems->AddSortMethod(SortByEpisodeNumber, 20359, LABEL_MASKS("%H. %T", "%R", "%H. %T", "%R"));
        break;
      }
    case SORT_METHOD_PRODUCTIONCODE:
      {
        //dir->m_listItems.AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));
        dir->m_listItems->AddSortMethod(SortByProductionCode, 20368, LABEL_MASKS("%H. %T", "%P", "%H. %T", "%P"));
        break;
      }
    case SORT_METHOD_LISTENERS:
      {
       dir->m_listItems->AddSortMethod(SortByListeners, 20455, LABEL_MASKS("%T", "%W"));
       break;
      }
    case SORT_METHOD_DATEADDED:
      {
        dir->m_listItems->AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a"));
        break;
      }
    case SORT_METHOD_FULLPATH:
      {
        dir->m_listItems->AddSortMethod(SortByPath, 573, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_LABEL_IGNORE_FOLDERS:
      {
        dir->m_listItems->AddSortMethod(SortByLabel, SortAttributeIgnoreFolders, 551, LABEL_MASKS("%T", label2Mask));
        break;
      }
    case SORT_METHOD_LASTPLAYED:
      {
        dir->m_listItems->AddSortMethod(SortByLastPlayed, 568, LABEL_MASKS("%T", "%G"));
        break;
      }
    case SORT_METHOD_PLAYCOUNT:
      {
        dir->m_listItems->AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V", "%T", "%V"));
        break;
      }
    case SORT_METHOD_CHANNEL:
      {
        dir->m_listItems->AddSortMethod(SortByChannel, 19029, LABEL_MASKS("%T", label2Mask));
        break;
      }

    default:
      break;
  }
}

bool CPluginDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  const std::string pathToUrl(url.Get());
  bool success = ExecuteScriptAndWaitOnResult(pathToUrl, false);

  // append the items to the list
  items.Assign(*m_listItems, true); // true to keep the current items
  m_listItems->Clear();
  return success;
}

bool CPluginDirectory::RunScriptWithParams(const std::string& strPath, bool resume)
{
  CURL url(strPath);
  if (url.GetHostName().empty()) // called with no script - should never happen
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), addon, ADDON_PLUGIN) && !CAddonInstaller::GetInstance().InstallModal(url.GetHostName(), addon))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return false;
  }

  // options
  std::string options = url.GetOptions();
  url.SetOptions(""); // do this because we can then use the url to generate the basepath
                      // which is passed to the plugin (and represents the share)

  std::string basePath(url.Get());

  // setup our parameters to send the script
  std::string strHandle = StringUtils::Format("%i", -1);
  std::vector<std::string> argv;
  argv.push_back(basePath);
  argv.push_back(strHandle);
  argv.push_back(options);

  std::string strResume = "resume:false";
  if (resume)
    strResume = "resume:true";
  argv.push_back(strResume);

  // run the script
  CLog::Log(LOGDEBUG, "%s - calling plugin %s('%s','%s','%s','%s')", __FUNCTION__, addon->Name().c_str(), argv[0].c_str(), argv[1].c_str(), argv[2].c_str(), argv[3].c_str());
  if (CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon, argv) >= 0)
    return true;
  else
    CLog::Log(LOGERROR, "Unable to run plugin %s", addon->Name().c_str());

  return false;
}

void CPluginDirectory::WaitForScriptToFinish(const SCRIPT_EXECUTION_INFO& scriptExecutionInfo,
                                             const int mSecs,
                                             const bool bForceStop)
{
  // Give the script mSecs miliseconds to exit before we attempt to stop it
  XbmcThreads::EndTime timer(mSecs);
  while (!timer.IsTimePast() &&
         CScriptInvocationManager::GetInstance().IsRunning(scriptExecutionInfo.Id) &&
         !m_fetchComplete.WaitMSec(20))
    ;

  if (bForceStop)
  {
    // cancel our script
    ForceStopRunningScript(scriptExecutionInfo);
  }
}

void CPluginDirectory::ForceStopRunningScript(const SCRIPT_EXECUTION_INFO& scriptExecutionInfo)
{
  if (scriptExecutionInfo.Id != -1 &&
      CScriptInvocationManager::GetInstance().IsRunning(scriptExecutionInfo.Id))
  {
    CLog::Log(LOGDEBUG, "%s- cancelling plugin %s (id=%d)", __FUNCTION__, m_addon->Name().c_str(),
              scriptExecutionInfo.Id);
    CScriptInvocationManager::GetInstance().Stop(scriptExecutionInfo.Id);
  }
}

void CPluginDirectory::SetResolvedUrl(int handle, bool success, const CFileItem *resultItem)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;

  dir->m_success = success;
  *dir->m_fileResult = *resultItem;

  // set the event to mark that we're done
  dir->m_fetchComplete.Set();
}

std::string CPluginDirectory::GetSetting(int handle, const std::string &strID)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if(dir && dir->m_addon)
    return dir->m_addon->GetSetting(strID);
  else
    return "";
}

void CPluginDirectory::SetSetting(int handle, const std::string &strID, const std::string &value)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if(dir && dir->m_addon)
    dir->m_addon->UpdateSetting(strID, value);
}

void CPluginDirectory::SetContent(int handle, const std::string &strContent)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (dir)
    dir->m_listItems->SetContent(strContent);
}

void CPluginDirectory::SetProperty(int handle, const std::string &strProperty, const std::string &strValue)
{
  CSingleLock lock(m_handleLock);
  CPluginDirectory *dir = dirFromHandle(handle);
  if (!dir)
    return;
  if (strProperty == "fanart_image")
    dir->m_listItems->SetArt("fanart", strValue);
  else
    dir->m_listItems->SetProperty(strProperty, strValue);
}

void CPluginDirectory::CancelDirectory()
{
  m_cancelled = true;
  m_fetchComplete.Set();
}

float CPluginDirectory::GetProgress() const
{
  if (m_totalItems > 0)
    return (m_listItems->Size() * 100.0f) / m_totalItems;
  return 0.0f;
}

bool CPluginDirectory::IsMediaLibraryScanningAllowed(const std::string& content, const std::string& strPath)
{
  if (content.empty())
    return false;

  CURL url(strPath);
  if (url.GetHostName().empty())
    return false;
  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), addon, ADDON_PLUGIN))
  {
    CLog::Log(LOGERROR, "Unable to find plugin %s", url.GetHostName().c_str());
    return false;
  }
  CPluginSource* plugin = dynamic_cast<CPluginSource*>(addon.get());
  if (!plugin)
    return false;

  auto& paths = plugin->MediaLibraryScanPaths();
  if (paths.empty())
    return false;
  auto it = paths.find(content);
  if (it == paths.end())
    return false;
  std::string path = url.GetFileName();
  for (const auto& p : it->second)
    if (p.empty() || p == "/" || URIUtils::PathHasParent(path, p))
      return true;
  return false;
}

bool CPluginDirectory::CheckExists(const std::string& content, const std::string& strPath)
{
  if (!IsMediaLibraryScanningAllowed(content, strPath))
    return false;
  // call the plugin at specified path with option "kodi_action=check_exists"
  // url exists if the plugin returns any fileitem with setResolvedUrl
  CURL url(strPath);
  url.SetOption("kodi_action", "check_exists");
  CFileItem item;
  return AsyncGetPluginResultAction(url.Get(), item, false).ExecuteSynchronously();
}
