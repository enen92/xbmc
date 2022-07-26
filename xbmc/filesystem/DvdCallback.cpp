/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DvdCallback.h"

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

struct SDirState
{
  CFileItemList list;
  int curr = 0;
};

void CDVDCallback::dvd_logger(void* priv, dvdnav_logger_level_t level, const char* fmt, va_list va)
{
  const std::string message = StringUtils::FormatV(fmt, va);
  auto logLevel = LOGDEBUG;
  switch (level)
  {
    case DVDNAV_LOGGER_LEVEL_INFO:
      logLevel = LOGINFO;
      break;
    case DVDNAV_LOGGER_LEVEL_ERROR:
      logLevel = LOGERROR;
      break;
    case DVDNAV_LOGGER_LEVEL_WARN:
      logLevel = LOGWARNING;
      break;
    case DVDNAV_LOGGER_LEVEL_DEBUG:
      logLevel = LOGDEBUG;
      break;
    default:
      break;
  };
  CLog::Log(logLevel, "Libdvd: {}", message);
}

void CDVDCallback::dir_close(dvd_dir_h *dir)
{
  if (dir)
  {
    CLog::Log(LOGDEBUG, "CDVDCallback - Closed dir ({})", fmt::ptr(dir));
    delete static_cast<SDirState*>(dir->internal);
    delete dir;
  }
}

dvd_dir_h* CDVDCallback::dir_open(const char* strDirname)
{
  CLog::Log(LOGDEBUG, "CDVDCallback - Opening dir {}", CURL::GetRedacted(strDirname));

  SDirState *st = new SDirState();
  if (!CDirectory::GetDirectory(strDirname, st->list, "", DIR_FLAG_DEFAULTS))
  {
    if (!CFile::Exists(strDirname))
      CLog::Log(LOGDEBUG, "CDVDCallback - Error opening dir! ({})",
                CURL::GetRedacted(strDirname));
    delete st;
    return nullptr;
  }

  dvd_dir_h* dir = new dvd_dir_h;
  dir->close = dir_close;
  dir->read = dir_read;
  dir->internal = (void*)st;

  return dir;
}

int CDVDCallback::dir_read(dvd_dir_h *dir, dvd_dirent_t *entry)
{
  SDirState* state = static_cast<SDirState*>(dir->internal);

  if (state->curr >= state->list.Size())
    return 1;

  strncpy(entry->d_name, state->list[state->curr]->GetLabel().c_str(), sizeof(entry->d_name));
  entry->d_name[sizeof(entry->d_name) - 1] = 0;
  state->curr++;

  return 0;
}

int64_t CDVDCallback::file_close(dvd_file_h *file)
{
  if (file)
  {
    delete static_cast<CFile*>(file->internal);
    delete file;
  }
  return 0;
}

dvd_file_h * CDVDCallback::file_open(const char *filename, const char *cmode)
{
  dvd_file_h* file = new dvd_file_h;

  file->close = file_close;
  file->seek = file_seek;
  file->read = file_read;

  CFile* fp = new CFile();
  if (fp->Open(filename))
  {
    file->internal = (void*)fp;
    return file;
  }

  CLog::Log(LOGDEBUG, "CDVDCallback - Error opening file! ({})", CURL::GetRedacted(filename));

  delete fp;
  delete file;

  return nullptr;
}

int64_t CDVDCallback::file_seek(dvd_file_h *file, int64_t offset, int32_t origin)
{
  return static_cast<CFile*>(file->internal)->Seek(offset, origin);
}

int64_t CDVDCallback::file_read(dvd_file_h *file, char *buf, int64_t size)
{
  return static_cast<int64_t>(static_cast<CFile*>(file->internal)->Read(buf, static_cast<size_t>(size)));
}


  int CDVDCallback::stat(const char *path, dvdstat_t* statbuff)
  {
    struct __stat64 tStat;
    int result = CFile::Stat(path, &tStat);
    statbuff->size = tStat.st_size;
    statbuff->is_blk = S_ISBLK(tStat.st_mode);
    statbuff->is_chr = S_ISCHR(tStat.st_mode);
    statbuff->is_dir = S_ISDIR(tStat.st_mode);
    statbuff->is_reg = S_ISREG(tStat.st_mode);
    return result;
  }
