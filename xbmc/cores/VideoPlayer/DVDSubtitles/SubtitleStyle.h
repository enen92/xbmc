/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Color.h"
#include <string>

namespace KODI
{
namespace SUBTITLE
{
enum BorderStyle
{
  NONE = 0,
  OUTLINE,
  BOX
};

enum FontStyle
{
  NORMAL = 0,
  BOLD,
  ITALICS,
  BOLD_ITALICS
};

struct STYLE {
  // font
  std::string font;
  int fontSize;
  UTILS::Color fontColor;
  int fontColorOpacity;
  FontStyle fontStyle;
  // border
  BorderStyle borderStyle;
  UTILS::Color borderColor;
  int borderColorOpacity;
  int borderSize;
  // shadow
  //UTILS::Color shadowColor;
  //int shadowColorOpacity;
  //int shadowSize;
};

}
}
