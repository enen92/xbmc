/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscDriveHandlerBase.h"

void CDiscDriveHandlerBase::ToggleDriveTray(const std::string& devicePath)
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
