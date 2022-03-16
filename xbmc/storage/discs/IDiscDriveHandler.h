/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

/*! \brief Represents the state of a disc (optical) drive
* @todo those unsigned ints are defined by licdio, we'll need some sort of map for translation in the future
* so they can be defined in libcdio access header instead and removed from platform agnostic code
*/
enum class DriveState : unsigned int
{
  DRIVE_OPEN = 0, ///< The drive is open
  DRIVE_NOT_READY = 1, ///< The drive is not ready (happens when openning or closing)
  DRIVE_READY = 2, ///< The drive is ready
  DRIVE_CLOSED_NO_MEDIA = 3, ///< The drive is closed but no media could be detected in the drive
  DRIVE_CLOSED_MEDIA_PRESENT = 4, ///< The drive is closed and there is media in the drive
  DRIVE_NONE = 5, ///< The system does not have an optical drive
  DRIVE_CLOSED_MEDIA_UNDEFINED =
      6 ///< The drive is closed but we don't know yet if there's media there
};

/*! \brief Represents the state of the drive tray
* @todo those unsigned ints are defined by licdio, we'll need some sort of map for translation in the future
* so they can be defined in libcdio access header instead and removed from platform agnostic code
*/
enum class TrayState : unsigned int
{
  UNDEFINED = 0, ///< The tray is in an undefined state, we don't know yet
  TRAY_OPEN = 16, ///< The tray is open
  TRAY_CLOSED_NO_MEDIA = 64, ///< The tray is closed and doesn't have any optical media
  TRAY_CLOSED_MEDIA_PRESENT = 96 ///< The tray is closed and contains optical media
};

/*! \brief Generic interface for platform disc drive handling
*/
class IDiscDriveHandler
{
public:
  virtual ~IDiscDriveHandler() = default;

  /*! \brief Get the optical drive state provided its device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual DriveState GetDriveState(const std::string& devicePath) = 0;

  /*! \brief Get the optical drive tray state provided the drive device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual TrayState GetTrayState(const std::string& devicePath) = 0;

  /*! \brief Eject the provided drive device
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void EjectDriveTray(const std::string& devicePath) = 0;

  /*! \brief Close the provided drive device
  * \note Some drives support closing appart from opening/eject
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void CloseDriveTray(const std::string& devicePath) = 0;

  /*! \brief Toggle the state of a given drive device
  *
  * Will internally call EjectDriveTray or CloseDriveTray depending on
  * the internal state of the drive (i.e. if open -> CloseDriveTray /
  * if closed -> EjectDriveTray)
  *
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void ToggleDriveTray(const std::string& devicePath) = 0;

  /*! \brief Called to create platform-specific disc drive handler
  *
  * This method is used to create platform-specific disc drive handler
  */
  static std::shared_ptr<IDiscDriveHandler> CreateInstance();
};
