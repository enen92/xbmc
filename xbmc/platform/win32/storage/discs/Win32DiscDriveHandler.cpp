/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32DiscDriveHandler.h"

#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <memory>

std::shared_ptr<IDiscDriveHandler> IDiscDriveHandler::CreateInstance()
{
  return std::make_shared<CWin32DiscDriveHandler>();
}

DriveState CWin32DiscDriveHandler::GetDriveState(const std::string& devicePath)
{
  DriveState driveState = DriveState::DRIVE_NOT_READY;
  auto status = static_cast<CWin32DiscDriveState>(CWIN32Util::GetDriveStatus(devicePath));
  switch (status)
  {
    case CWin32DiscDriveState::DRIVE_ERROR:
      driveState = DriveState::DRIVE_NOT_READY;
      break;
    case CWin32DiscDriveState::NO_MEDIA:
      driveState = DriveState::DRIVE_CLOSED_NO_MEDIA;
      break;
    case CWin32DiscDriveState::TRAY_OPEN:
      driveState = DriveState::DRIVE_OPEN;
      break;
    case CWin32DiscDriveState::MEDIA_ACCESSIBLE:
      driveState = DriveState::DRIVE_CLOSED_MEDIA_PRESENT;
      break;
    default:
      CLog::LogF(LOGWARNING, "Unknown/unhandled drive state interpreted as DRIVE_NOT_READY");
      break;
  }
  return driveState;
}

TrayState CWin32DiscDriveHandler::GetTrayState(const std::string& devicePath)
{
  TrayState trayState = TrayState::UNDEFINED;
  DriveState driveState = GetDriveState(devicePath);
  switch (driveState)
  {
    case DriveState::DRIVE_OPEN:
      trayState = TrayState::TRAY_OPEN;
      break;
    case DriveState::DRIVE_CLOSED_NO_MEDIA:
      trayState = TrayState::TRAY_CLOSED_NO_MEDIA;
      break;
    case DriveState::DRIVE_CLOSED_MEDIA_PRESENT:
      trayState = TrayState::TRAY_CLOSED_MEDIA_PRESENT;
      break;
    default:
      CLog::LogF(LOGWARNING, "Unknown/unhandled tray state interpreted as TrayState::UNDEFINED");
      break;
  }
  return trayState;
}

void CWin32DiscDriveHandler::EjectDriveTray(const std::string& devicePath)
{
  if (devicePath.empty())
  {
    CLog::LogF(LOGERROR, "Invalid/Empty devicePath provided");
    return;
  }
  CWIN32Util::EjectTray(devicePath[0]);
}

void CWin32DiscDriveHandler::CloseDriveTray(const std::string& devicePath)
{
  if (devicePath.empty())
  {
    CLog::LogF(LOGERROR, "Invalid/Empty devicePath provided");
    return;
  }
  CWIN32Util::CloseTray(devicePath[0]);
}

void CWin32DiscDriveHandler::ToggleDriveTray(const std::string& devicePath)
{
  if (devicePath.empty())
  {
    CLog::LogF(LOGERROR, "Invalid/Empty devicePath provided");
    return;
  }
  CWIN32Util::ToggleTray(devicePath[0]);
}
