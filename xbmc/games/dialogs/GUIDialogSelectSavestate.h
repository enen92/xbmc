/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

namespace KODI
{
namespace GAME
{
class CDialogGameSaves;

/*!
 * \ingroup games
 */
class CGUIDialogSelectSavestate
{
public:
  static bool ShowAndGetSavestate(const std::string& gamePath, std::string& savestatePath);

private:
  static std::shared_ptr<CDialogGameSaves> GetDialog();
};
} // namespace GAME
} // namespace KODI
