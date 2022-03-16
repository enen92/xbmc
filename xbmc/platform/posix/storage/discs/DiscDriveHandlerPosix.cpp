/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscDriveHandlerPosix.h"

#include "storage/cdioSupport.h"
#include "utils/log.h"

#include <memory>

namespace
{
constexpr int MaxOpenRetries = 3;
}
using namespace MEDIA_DETECT;


std::shared_ptr<IDiscDriveHandler> IDiscDriveHandler::CreateInstance()
{
  return std::make_shared<DiscDriveHandlerPosix>();
}

DriveState DiscDriveHandlerPosix::GetDriveState(const std::string& devicePath)
{
  std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return DriveState::DRIVE_NOT_READY;
  }

  DriveState driveStatus = DriveState::DRIVE_NOT_READY;
  CdIo_t* cdio = libCdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
  if (!cdio)
  {
    CLog::LogF(LOGERROR, "Failed to open device {} libcdio handler", devicePath);
    return DriveState::DRIVE_NOT_READY;
  }

  CdioTrayStatus status = static_cast<CdioTrayStatus>(libCdio->mmc_get_tray_status(cdio));

  switch (status)
  {
    case CdioTrayStatus::CLOSED:
      driveStatus = DriveState::DRIVE_CLOSED_MEDIA_UNDEFINED;
      break;

    case CdioTrayStatus::OPEN:
      driveStatus = DriveState::DRIVE_OPEN;
      break;

    default:
      break;
  }
  libCdio->cdio_destroy(cdio);

  return driveStatus;
}

TrayState DiscDriveHandlerPosix::GetTrayState(const std::string& devicePath)
{
  std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return TrayState::UNDEFINED;
  }

  TrayState trayStatus = TrayState::UNDEFINED;
  discmode_t discmode = CDIO_DISC_MODE_NO_INFO;
  CdIo_t* cdio = libCdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
  if (!cdio)
  {
    CLog::LogF(LOGERROR, "Failed to open device {} libcdio handler", devicePath);
    return TrayState::UNDEFINED;
  }

  discmode = libCdio->cdio_get_discmode(cdio);

  if (discmode == CDIO_DISC_MODE_NO_INFO)
  {
    trayStatus = TrayState::TRAY_CLOSED_NO_MEDIA;
  }
  else if (discmode == CDIO_DISC_MODE_ERROR)
  {
    trayStatus = TrayState::UNDEFINED;
  }
  else
  {
    trayStatus = TrayState::TRAY_CLOSED_MEDIA_PRESENT;
  }
  libCdio->cdio_destroy(cdio);

  return trayStatus;
}

void DiscDriveHandlerPosix::EjectDriveTray(const std::string& devicePath)
{
  std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return;
  }

  int retries = MaxOpenRetries;
  while (retries-- > 0)
  {
    CdIo_t* cdio = libCdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
    if (cdio)
    {
      driver_return_code_t ret = libCdio->cdio_eject_media(&cdio);
      if (ret == DRIVER_OP_SUCCESS)
      {
        libCdio->cdio_destroy(cdio);
        break;
      }
      libCdio->cdio_destroy(cdio);
    }
    else
    {
      break;
    }
  }
}

void DiscDriveHandlerPosix::CloseDriveTray(const std::string& devicePath)
{
  std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return;
  }

  driver_return_code_t ret = libCdio->cdio_close_tray(devicePath.c_str(), nullptr);
  if (ret != DRIVER_OP_SUCCESS)
  {
    CLog::LogF(LOGERROR, "Closing tray failed for device {}: {}", devicePath,
               libCdio->cdio_driver_errmsg(ret));
  }
}
