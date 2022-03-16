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

/*! \brief Implementation of a generic/base DiscDriveHandler
* \sa IDiscDriveHandler
*/
class CDiscDriveHandlerBase : public IDiscDriveHandler
{
public:
  /*! \brief Toggle the state of a given drive device
  * \sa IDiscDriveHandler
  * Will internally call EjectDriveTray or CloseDriveTray depending on
  * the internal state of the drive (i.e. if open -> CloseDriveTray /
  * if closed -> EjectDriveTray)
  *
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  void ToggleDriveTray(const std::string& devicePath) override;
};
