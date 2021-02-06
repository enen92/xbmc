/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDCodecs/Overlay/DVDOverlaySSA.h"

class TextToASSAdapter
{
public:
  TextToASSAdapter();
  
  ~TextToASSAdapter();
  
  CDVDOverlaySSA* CreateAssOverlay(const double startPts, const double endPts,
                                          const std::string& text);
  
  void AppendTextToLastOverlay(const std::string& text);
  
  void ReplaceTextInLastOverlay(const std::string& text);
  
private:
  CDVDSubtitlesLibass* m_libass;
  int m_defaultStyleId = ASS_INVALID_STYLE;
};
