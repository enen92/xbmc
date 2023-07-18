/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscsUtils.h"

#include "FileItem.h"
//! @todo it's wrong to include videoplayer scoped files, refactor
// dvd inputstream so they can be used by other components. Or just use libdvdnav directly.
#include "cores/VideoPlayer/DVDInputStreams/DVDInputStreamNavigator.h"
#ifdef HAVE_LIBBLURAY
//! @todo it's wrong to include vfs scoped files in a utils class, refactor
// to use libbluray directly.
#include <libbluray/bluray.h>
#include <libbluray/bluray-version.h>
#include "filesystem/BlurayDirectory.h"
#endif

bool UTILS::DISCS::GetDiscInfo(UTILS::DISCS::DiscInfo& info, const std::string& mediaPath)
{
  // try to probe as a DVD
  info = ProbeDVDDiscInfo(mediaPath);
  if (!info.empty())
    return true;

  // try to probe as Blu-ray
  info = ProbeBlurayDiscInfo(mediaPath);
  if (!info.empty())
    return true;

  return false;
}

UTILS::DISCS::DiscInfo UTILS::DISCS::ProbeDVDDiscInfo(const std::string& mediaPath)
{
  DiscInfo info;
  CFileItem item{mediaPath, false};
  CDVDInputStreamNavigator dvdNavigator{nullptr, item};
  if (dvdNavigator.Open())
  {
    info.type = DiscType::DVD;
    info.name = dvdNavigator.GetDVDTitleString();
    // fallback to DVD volume id
    if (info.name.empty())
    {
      info.name = dvdNavigator.GetDVDVolIdString();
    }
    info.serial = dvdNavigator.GetDVDSerialString();
  }
  return info;
}

UTILS::DISCS::DiscInfo UTILS::DISCS::ProbeBlurayDiscInfo(const std::string& mediaPath)
{
  DiscInfo info;
#ifdef HAVE_LIBBLURAY
  auto bd = bd_init();
  auto coiso = bd_open_disc(bd, mediaPath.c_str(), nullptr);
  const BLURAY_DISC_INFO* disc_info = bd_get_disc_info(bd);
  if (!disc_info || !disc_info->bluray_detected)
    return info;

  info.type = DiscType::BLURAY;
  #if (BLURAY_VERSION > BLURAY_VERSION_CODE(1,0,0))
    info.name = disc_info->disc_name ? disc_info->disc_name : "";
    info.serial = disc_info->udf_volume_id ? disc_info->udf_volume_id : "";
  #endif
  if (info.name.empty())
  {
    info.name = "Blu-ray";
  }
#endif
  return info;
}
