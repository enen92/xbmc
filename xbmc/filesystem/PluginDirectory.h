/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "SortFileItem.h"
#include "addons/IAddon.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/IRunnable.h"
#include "threads/Thread.h"

#include <atomic>
#include <map>
#include <string>

#include "PlatformDefs.h"

class CURL;
class CFileItem;
class CFileItemList;

namespace XFILE
{

class CPluginDirectory : public IDirectory
{
public:
  CPluginDirectory();
  ~CPluginDirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool AllowAll() const override { return true; }
  bool Exists(const CURL& url) override { return true; }
  float GetProgress() const override;
  void CancelDirectory() override;
  static bool RunScriptWithParams(const std::string& strPath, bool resume);

  /*! \brief Check whether a plugin supports media library scanning.
  \param content content type - movies, tvshows, musicvideos.
  \param strPath full plugin url.
  \return true if scanning at specified url is allowed, false otherwise.
  */
  static bool IsMediaLibraryScanningAllowed(const std::string& content, const std::string& strPath);

  /*! \brief Check whether a plugin url exists by calling the plugin and checking result.
  Applies only to plugins that support media library scanning.
  \param content content type - movies, tvshows, musicvideos.
  \param strPath full plugin url.
  \return true if the plugin supports scanning and specified url exists, false otherwise.
  */
  static bool CheckExists(const std::string& content, const std::string& strPath);

  // callbacks from python
  static bool AddItem(int handle, const CFileItem *item, int totalItems);
  static bool AddItems(int handle, const CFileItemList *items, int totalItems);
  static void EndOfDirectory(int handle, bool success, bool replaceListing, bool cacheToDisc);
  static void AddSortMethod(int handle, SORT_METHOD sortMethod, const std::string &label2Mask);
  static std::string GetSetting(int handle, const std::string &key);
  static void SetSetting(int handle, const std::string &key, const std::string &value);
  static void SetContent(int handle, const std::string &strContent);
  static void SetProperty(int handle, const std::string &strProperty, const std::string &strValue);
  static void SetResolvedUrl(int handle, bool success, const CFileItem* resultItem);
  static void SetLabel2(int handle, const std::string& ident);

  /**
   * A structure holding data of a script execution
   * \sa CPluginDirectory_TriggerScriptExecution
   */
  struct SCRIPT_EXECUTION_INFO
  {
    int Id = -1; /**the script id */
    int Handle = -1; /**the script handle */
  };

private:
  ADDON::AddonPtr m_addon;

  /*!
  \brief Executes a given script asynchronously and returns the script execution info struct (id and
  handle) \anchor CPluginDirectory_TriggerScriptExecution \param strPath[in] full plugin url \param
  resume if resume should be passed to the script as an argument \return a stuct containing the id
  and handle of the script execution
  */
  const SCRIPT_EXECUTION_INFO TriggerScriptExecution(const std::string& strPath, const bool resume);

  /*!
  \brief Executes a given script and waits for execution to complete, returning the
   success of the operation
  \param strPath[in] full plugin url
  \param resume  if resume should be passed to the script as an argument
  \return true if the execution had success
  */
  bool ExecuteScriptAndWaitOnResult(const std::string& strPath, const bool resume);

  /*!
  \brief Wait mSecs for the script to finish. If mSecs is surpassed forceStop will force stop the
  script execution \param scriptExecutionInfo The struct containing the script execution info (id
  and handle) \param mSecs The time to wait for the script to finish (msec) \param forceStop If
  script should be stopped after mSecs have passed and the script is still running \return true if
  the script executed within the specified period
  */
  void WaitForScriptToFinish(const SCRIPT_EXECUTION_INFO& scriptExecutionInfo,
                             const int mSecs,
                             const bool bForceStop);

  /*!
  \brief Force stop a running script
  \param scriptExecutionInfo The struct containing the script execution info (id and handle)
  */
  void ForceStopRunningScript(const SCRIPT_EXECUTION_INFO& scriptExecutionInfo);

  static std::map<int,CPluginDirectory*> globalHandles;
  static int getNewHandle(CPluginDirectory *cp);
  static void reuseHandle(int handle, CPluginDirectory* cp);

  /*!
  \brief Updates an item (finalItem) with the properties and resolved path obtained from the script
  execution (resultItem) \param finalItem[in,out] The item destination \param resultItem[in] The
  CPluginDirectory item that resulted from the script execution
  */
  static void UpdateResultItem(CFileItem& finalItem, const CFileItem* resultItem);

  static void removeHandle(int handle);
  static CPluginDirectory *dirFromHandle(int handle);
  static CCriticalSection m_handleLock;
  static int handleCounter;

  CFileItemList* m_listItems;
  CFileItem*     m_fileResult;
  CEvent         m_fetchComplete;

  std::atomic<bool> m_cancelled;
  bool          m_success = false;      // set by script in EndOfDirectory
  int    m_totalItems = 0;   // set by script in AddDirectoryItem

  friend class AsyncGetPluginResultAction;

  class CScriptObserver : public CThread
  {
  public:
    CScriptObserver(int scriptId, CEvent &event);
    void Abort();
  protected:
    void Process() override;
    int m_scriptId;
    CEvent &m_event;
  };
};

/*!
 \brief AsyncGetPluginResultAction is an async action to resolve plugin paths (e.g. derive the
 dynpath playable path from the raw path of a plugin:// based fileitem). It is intended to be driven
 by busy loaders (e.g. CGUIDialogBusy) and can also be executed syncronously on the callee thread
 (see \ref AsyncGetPluginResultAction_ExecuteSynchronously "ExecuteSynchronously"). It is usefull
 since plugins are written in python and kodi has no control over the addon code or the time it
 takes to execute. Hence, if the core execution depends on a plugin call, this call should be
 wrapped in some busyloader that drives the render loop and allows the cancelation of its builtin
 execution
*/
class AsyncGetPluginResultAction : public IRunnable
{
public:
  /*!
  \brief No default constructor
  */
  AsyncGetPluginResultAction() = delete;

  /*!
  \brief Creates the plugin resolver action
  \param strPath[in] The plugin path
  \param resultItem[in,out] The CFIleitem that wll be filled with the result of the strPath
  execution \param resume[in] if resume should be passed to the script that will be executed as an
  argument
  */
  AsyncGetPluginResultAction(const std::string& strPath, CFileItem& resultItem, const bool resume);

  /*!
  \brief Checks if the async execution had success i.e., if the plugin (and ihnerently the script)
  executed sucessfully and the respective CFileItem is properly filled \return true if the execution
  was successfull
  */
  bool ExecutionHadSuccess() const;

  /*!
  \brief Cancels the async operation
  */
  void Cancel() override;

  /*!
  \brief Executes this async action syncronously, waiting for the result
  \anchor AsyncGetPluginResultAction_ExecuteSynchronously
  \return true if the execution was successfull
  */
  bool ExecuteSynchronously();

private:
  /*!
  \brief IRunnable implementation
  */
  void Run() override;

  /*!
  \brief Flags the async event, sets the success variables and updates the result item (called
  internally as part of Run)
  */
  void Finish();

  // propagated for script execution
  std::string m_path;
  CFileItem* m_item;
  bool m_resume = false;

  // part of future/async
  CEvent m_event;
  std::atomic<bool> m_success;
  std::atomic<bool> m_cancelled;

  // handler for CPluginDirectory static function calls
  XFILE::CPluginDirectory m_pluginDirHandler;
};
}
