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

/*! \brief Implementation of an abstract DiscDriveHandler
* \sa IDiscDriveHandler
*/
class AbstractDiscDriveHandler : public IDiscDriveHandler
{
public:
  /*! \brief Get the optical drive state provided its device path
  * \sa IDiscDriveHandler
  * \note Subclasses must implement this method, AbstractDiscDriveHandler itself will throw an exception
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual DriveState GetDriveState(const std::string& devicePath) = 0;

  /*! \brief Get the optical drive tray state provided the drive device path
  * \sa IDiscDriveHandler
  * \note Subclasses must implement this method, AbstractDiscDriveHandler itself will throw an exception
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual TrayState GetTrayState(const std::string& devicePath) = 0;

  /*! \brief Eject the provided drive device
  * \sa IDiscDriveHandler
  * \note Subclasses must implement this method, AbstractDiscDriveHandler itself will throw an exception
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void EjectDriveTray(const std::string& devicePath) = 0;

  /*! \brief Close the provided drive device
  * \sa IDiscDriveHandler
  * \note Subclasses must implement this method, AbstractDiscDriveHandler itself will throw an exception
  * \note Some drives support closing appart from opening/eject
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void CloseDriveTray(const std::string& devicePath) = 0;

  /*! \brief Toggle the state of a given drive device
  * \sa IDiscDriveHandler
  * Will internally call EjectDriveTray or CloseDriveTray depending on
  * the internal state of the drive (i.e. if open -> CloseDriveTray /
  * if closed -> EjectDriveTray)
  *
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void ToggleDriveTray(const std::string& devicePath) override;
};
