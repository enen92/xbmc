
/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "storage/discs/AbstractDiscDriveHandler.h"
#include <string>

class DiscDriveHandlerPosix : public AbstractDiscDriveHandler {
   public:
      DiscDriveHandlerPosix() = default;

      virtual DriveState GetDriveState(const std::string& devicePath) override;
      virtual TrayState GetTrayState(const std::string& devicePath) override;
      virtual void EjectDriveTray(const std::string& devicePath) override;
      virtual void CloseDriveTray(const std::string& devicePath) override;
};
