/*
 *      Copyright (C) 2005-2018 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Color.h"
#include "ColorUtils.h"
#include "math.h"

UTILS::Color ColorUtils::ChangeOpacity(const UTILS::Color color, const float opacity)
{
  int alpha = (color >> 24) & 0xff;
  int r = (color >> 16) & 0xff;
  int g = (color >> 8) & 0xff;
  int b = color & 0xff;
    
  int newAlpha = ceil(alpha * opacity);
    
  UTILS::Color newColor = r << 16;
  newColor += g << 8;
  newColor += b;
  newColor += (newAlpha << 24);
  return newColor;
};
