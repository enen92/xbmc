/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AbstractDiscDriveHandler.h"

#include <stdexcept>

DriveState AbstractDiscDriveHandler::GetDriveState(const std::string& devicePath)
{
  std::logic_error("GetDriveState: Method not allowed in abstract class");
  return DriveState::DRIVE_NOT_READY;
}

TrayState AbstractDiscDriveHandler::GetTrayState(const std::string& devicePath)
{
  std::logic_error("GetTrayState: Method not allowed in abstract class");
  return TrayState::UNDEFINED;
}

void AbstractDiscDriveHandler::EjectDriveTray(const std::string& devicePath)
{
  std::logic_error("EjectDriveTray: Method not allowed in abstract class");
}

void AbstractDiscDriveHandler::CloseDriveTray(const std::string& devicePath)
{
  std::logic_error("CloseDriveTray:: Method not allowed in abstract class");
}

void AbstractDiscDriveHandler::ToggleDriveTray(const std::string& devicePath)
{
  if (GetDriveState(devicePath) == DriveState::DRIVE_OPEN)
  {
    CloseDriveTray(devicePath);
  }
  else
  {
    EjectDriveTray(devicePath);
  }
}
