/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "IDiscDriveHandler.h"
#include <string>

class AbstractDiscDriveHandler: public IDiscDriveHandler {
   public:
      virtual DriveState GetDriveStatus(const std::string& devicePath) = 0;
      virtual TrayState GetTrayStatus(const std::string& devicePath) = 0;
      virtual void EjectDriveTray(const std::string& devicePath) = 0;
      virtual void CloseDriveTray(const std::string& devicePath) = 0;
      virtual void ToggleDriveTray(const std::string& devicePath) override;
};
