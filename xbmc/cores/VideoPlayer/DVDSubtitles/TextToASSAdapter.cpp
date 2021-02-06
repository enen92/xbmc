/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextToASSAdapter.h"
#include "utils/log.h"

TextToASSAdapter::TextToASSAdapter()
{
  m_libass = new CDVDSubtitlesLibass();

  if (m_libass->CreateEmptyTrack())
  {
    m_defaultStyleId = m_libass->CreateDefaultStyle();
  }
}

TextToASSAdapter::~TextToASSAdapter()
{
  if (m_libass != nullptr)
  {
    SAFE_RELEASE(m_libass);
  }
}

CDVDOverlaySSA* TextToASSAdapter::CreateAssOverlay(const double startPts,
                                                          const double endPts,
                                                          const std::string& text)
{
  if (m_libass == nullptr)
  {
    CLog::Log(LOGERROR, "TextToASSAdapter: No libass handler found", __FUNCTION__);
    return nullptr;
  }
  
  if (m_defaultStyleId == ASS_INVALID_STYLE)
  {
    CLog::Log(LOGERROR, "TextToASSAdapter: No default ASS style found", __FUNCTION__);
    return nullptr;
  }

  if (m_libass->CreateEvent(startPts, endPts, text.c_str(), m_defaultStyleId))
  {
    auto overlay = new CDVDOverlaySSA(m_libass);
    overlay->iPTSStartTime = startPts;
    overlay->iPTSStopTime = endPts;
    overlay->setSubType(CDVDOverlaySSA::ASSSubType::ADAPTED);
    return overlay;
  }
  return nullptr;
}

void TextToASSAdapter::AppendTextToLastOverlay(const std::string& text)
{
  if (m_libass == nullptr)
  {
    CLog::Log(LOGERROR, "TextToASSAdapter: No libass handler found", __FUNCTION__);
    return;
  }

  m_libass->AppendTextToLastEvent(text);
}

void ReplaceTextInLastOverlay(const std::string& text)
{
  // todo - when needed
}
