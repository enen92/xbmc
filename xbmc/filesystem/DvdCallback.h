/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <dvdread/dvd_filesystem.h>
#include <dvdnav/dvdnav.h>
#ifdef __cplusplus
}
#endif

class CDVDCallback
{
public:
  // logger implementation
  static void dvd_logger(void* priv, dvdnav_logger_level_t level, const char* fmt, va_list va);

  // filesystem close
  static void close(dvdnav_filesystem_h *fs) {};

  // dir
  static void dir_close(dvd_dir_h* dir);
  static dvd_dir_h* dir_open(dvdnav_filesystem_h *fs, const char* strDirname);
  static int dir_read(dvd_dir_h* dir, dvd_dirent_t* entry);

  // file
  static dvd_file_h* file_open(dvdnav_filesystem_h *fs, const char* filename);
  static int file_close(dvd_file_h* file);
  static ssize_t file_read(dvd_file_h* file, char* buf, size_t size);
  static int64_t file_seek(dvd_file_h* file, int64_t offset, int32_t origin);

  // stat
  static int stat(dvdnav_filesystem_h *fs, const char *path, dvdstat_t* statbuf);

private:
  CDVDCallback() = default;
  ~CDVDCallback() = default;
};