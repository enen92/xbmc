/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscDriveHandlerPosix.h"
#include "storage/MediaManager.h"
#include "storage/cdioSupport.h"
#include "utils/log.h"

using namespace MEDIA_DETECT;
using namespace MEDIA_DETECT::STORAGE;

IDiscDriveHandler* IDiscDriveHandler::CreateInstance()
{
  return new DiscDriveHandlerPosix();
}

DriveState DiscDriveHandlerPosix::GetDriveStatus(const std::string& devicePath)
{
  DriveState driveStatus = DriveState::DRIVE_NOT_READY;
  std::shared_ptr<CLibcdio> c_cdio = CLibcdio::GetInstance();
  if (c_cdio)
  {
    CdIo_t* cdio = c_cdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
    int status = c_cdio->mmc_get_tray_status(cdio);

    switch(status)
    {
    case 0: //closed
      driveStatus = DriveState::DRIVE_CLOSED_MEDIA_UNDEFINED;
      break;

    case 1: //open
      driveStatus = DriveState::DRIVE_OPEN;
      break;
    }
    c_cdio->cdio_destroy(cdio);
  }
  return driveStatus;
}

TrayState DiscDriveHandlerPosix::GetTrayStatus(const std::string& devicePath)
{
  TrayState trayStatus = TrayState::UNDEFINED;
  discmode_t discmode = CDIO_DISC_MODE_NO_INFO;
  std::shared_ptr<CLibcdio> c_cdio = CLibcdio::GetInstance();
  if (c_cdio)
  {
    CdIo_t* cdio = c_cdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
    discmode = c_cdio->cdio_get_discmode(cdio);
    if (discmode == CDIO_DISC_MODE_NO_INFO)
      trayStatus = TrayState::TRAY_CLOSED_NO_MEDIA;
    else if (discmode == CDIO_DISC_MODE_ERROR)
      trayStatus = TrayState::UNDEFINED;
    else
      trayStatus = TrayState::TRAY_CLOSED_MEDIA_PRESENT;
    c_cdio->cdio_destroy(cdio);
  }
  return trayStatus;
}

void DiscDriveHandlerPosix::EjectDriveTray(const std::string& devicePath)
{
  std::shared_ptr<CLibcdio> c_cdio = CLibcdio::GetInstance();
  if (c_cdio)
  {
    int retries=3;
    while (retries-- > 0)
    {
      CdIo_t* cdio = c_cdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
      if (cdio)
      {
        c_cdio->cdio_eject_media(&cdio);
        c_cdio->cdio_destroy(cdio);
      }
      else
      {
        break;
      }
    }
  }
}

void DiscDriveHandlerPosix::CloseDriveTray(const std::string& devicePath)
{
  std::shared_ptr<CLibcdio> c_cdio = CLibcdio::GetInstance();
  if (c_cdio)
  {
    driver_return_code_t ret = c_cdio->cdio_close_tray(devicePath.c_str(), nullptr);
    if (ret != DRIVER_OP_SUCCESS)
    {
      CLog::LogF(LOGERROR, "Closing tray failed for device {}: {}", devicePath, c_cdio->cdio_driver_errmsg(ret));
    }
  }
}
