
/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "storage/discs/IDiscDriveHandler.h"

#include <string>

enum class Win32DiscDriveState : int
{
  DRIVE_ERROR = -1,
  NO_MEDIA = 0,
  TRAY_OPEN = 1,
  MEDIA_ACCESSIBLE = 2,
};

class Win32DiscDriveHandler : public IDiscDriveHandler
{
public:
  /*! \brief Win32 DiscDriveHandler constructor
  */
  Win32DiscDriveHandler() = default;

  /*! \brief Get the optical drive state provided its device path
  * \param devicePath the path for the device drive (e.g. D\://)
  * \return The drive state
  */
  virtual DriveState GetDriveState(const std::string& devicePath) override;

  /*! \brief Get the optical drive tray state provided the drive device path
  * \param devicePath the path for the device drive (e.g. D\://)
  * \return The drive state
  */
  virtual TrayState GetTrayState(const std::string& devicePath) override;

  /*! \brief Eject the provided drive device
  * \param devicePath the path for the device drive (e.g. D\://)
  */
  virtual void EjectDriveTray(const std::string& devicePath) override;

  /*! \brief Close the provided drive device
  * \note Some drives support closing appart from opening/eject
  * \param devicePath the path for the device drive (e.g. D\://)
  */
  virtual void CloseDriveTray(const std::string& devicePath) override;

  /*! \brief Toggle the state of a given drive device
  *
  * \param devicePath the path for the device drive (e.g. D\://)
  */
  virtual void ToggleDriveTray(const std::string& devicePath) override;
};
