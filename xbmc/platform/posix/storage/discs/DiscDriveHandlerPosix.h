
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

class DiscDriveHandlerPosix : public AbstractDiscDriveHandler
{
public:
  /*! \brief Posix DiscDriveHandler constructor
  */
  DiscDriveHandlerPosix() = default;

  /*! \brief Get the optical drive state provided its device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual DriveState GetDriveState(const std::string& devicePath) override;

  /*! \brief Get the optical drive tray state provided the drive device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual TrayState GetTrayState(const std::string& devicePath) override;

  /*! \brief Eject the provided drive device
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void EjectDriveTray(const std::string& devicePath) override;

  /*! \brief Close the provided drive device
  * \note Some drives support closing appart from opening/eject
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void CloseDriveTray(const std::string& devicePath) override;
};
