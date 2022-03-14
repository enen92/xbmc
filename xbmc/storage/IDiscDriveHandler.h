/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include <string>

enum class DriveState : unsigned int {
    DRIVE_OPEN = 0,  // Open...
    DRIVE_NOT_READY = 1, // Opening.. Closing...
    DRIVE_READY = 2,
    DRIVE_CLOSED_NO_MEDIA = 3, // CLOSED...but no media in drive
    DRIVE_CLOSED_MEDIA_PRESENT = 4, // Will be send once when the drive just have closed
    DRIVE_NONE = 5, // system doesn't have an optical drive
    DRIVE_CLOSED_MEDIA_UNDEFINED = 6
};

enum class TrayState: unsigned int {
    UNDEFINED = 0,
    TRAY_OPEN = 16,
    TRAY_CLOSED_NO_MEDIA = 64,
    TRAY_CLOSED_MEDIA_PRESENT = 96
};

class IDiscDriveHandler
{
public:

  virtual ~IDiscDriveHandler() = default;

  virtual DriveState GetDriveStatus(const std::string& devicePath) = 0;
  virtual TrayState GetTrayStatus(const std::string& devicePath) = 0;
  virtual void EjectDriveTray(const std::string& devicePath) = 0;
  virtual void CloseDriveTray(const std::string& devicePath) = 0;
  virtual void ToggleDriveTray(const std::string& devicePath) = 0;

  /**\brief Called by media manager to create platform storage provider
  *
  * This method used to create platform specified storage provider
  */
  static IDiscDriveHandler* CreateInstance();
};
