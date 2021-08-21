/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DebugRenderer.h"

#include "cores/VideoPlayer/DVDCodecs/Overlay/DVDOverlayText.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

using namespace OVERLAY;

CDebugRenderer::CDebugRenderer()
{
}

CDebugRenderer::~CDebugRenderer()
{
  if (!m_isConfigured)
    return;

  m_adapter->FlushSubtitles();

  delete m_adapter;
}

void CDebugRenderer::Configure()
{
  if (m_isConfigured)
    return;

  m_adapter = new CSubtitlesAdapter();

  for (int i = 0; i < 6; i++)
  {
    m_strDebug[i] = " ";
  }

  m_isConfigured = m_adapter->Initialize();
  if (!m_isConfigured)
    CLog::Log(LOGERROR, "{} - Failed to configure OSD info debug renderer", __FUNCTION__);
}

void CDebugRenderer::SetInfo(DEBUG_INFO_PLAYER& info)
{
  if (!m_isConfigured)
    return;

  // FIXME: We currently force ASS_Event's and the current PTS
  // of rendering with fixed values to allow perpetual
  // display of on-screen text. It would be appropriate for Libass
  // provide a way to allow fixed on-screen text display
  // without use all these fixed values.

  m_overlayRenderer.Release(0);

  if (info.audio != m_strDebug[0])
  {
    m_strDebug[0] = info.audio;
    m_adapter->AddSubtitle(m_strDebug[0].c_str(), 0., 5000000.);
  }
  if (info.video != m_strDebug[1])
  {
    m_strDebug[1] = info.video;
    m_adapter->AddSubtitle(m_strDebug[1].c_str(), 0., 5000000.);
  }
  if (info.player != m_strDebug[2])
  {
    m_strDebug[2] = info.player;
    m_adapter->AddSubtitle(m_strDebug[2].c_str(), 0., 5000000.);
  }
  if (info.vsync != m_strDebug[3])
  {
    m_strDebug[3] = info.vsync;
    m_adapter->AddSubtitle(m_strDebug[3].c_str(), 0., 5000000.);
  }

  auto overlay = m_adapter->CreateOverlay();
  m_overlayRenderer.AddOverlay(overlay, 1000000., 0);
}

void CDebugRenderer::SetInfo(DEBUG_INFO_VIDEO& video, DEBUG_INFO_RENDER& render)
{
  if (!m_isConfigured)
    return;

  // FIXME: We currently force ASS_Event's and the current PTS
  // of rendering with fixed values to allow perpetual
  // display of on-screen text. It would be appropriate for Libass
  // provide a way to allow fixed on-screen text display
  // without use all these fixed values.

  m_overlayRenderer.Release(0);

  if (video.videoSource != m_strDebug[0])
  {
    m_strDebug[0] = video.videoSource;
    m_adapter->AddSubtitle(m_strDebug[0].c_str(), 0., 5000000.);
  }
  if (video.metaPrim != m_strDebug[1])
  {
    m_strDebug[1] = video.metaPrim;
    m_adapter->AddSubtitle(m_strDebug[1].c_str(), 0., 5000000.);
  }
  if (video.metaLight != m_strDebug[2])
  {
    m_strDebug[2] = video.metaLight;
    m_adapter->AddSubtitle(m_strDebug[2].c_str(), 0., 5000000.);
  }
  if (video.shader != m_strDebug[3])
  {
    m_strDebug[3] = video.shader;
    m_adapter->AddSubtitle(m_strDebug[3].c_str(), 0., 5000000.);
  }
  if (render.renderFlags != m_strDebug[4])
  {
    m_strDebug[4] = render.renderFlags;
    m_adapter->AddSubtitle(m_strDebug[4].c_str(), 0., 5000000.);
  }
  if (render.videoOutput != m_strDebug[5])
  {
    m_strDebug[5] = render.videoOutput;
    m_adapter->AddSubtitle(m_strDebug[5].c_str(), 0., 5000000.);
  }

  auto overlay = m_adapter->CreateOverlay();
  m_overlayRenderer.AddOverlay(overlay, 1000000., 0);
}

void CDebugRenderer::Render(CRect& src, CRect& dst, CRect& view)
{
  if (!m_isConfigured)
    return;

  m_overlayRenderer.SetVideoRect(src, dst, view);
  m_overlayRenderer.Render(0);
}

void CDebugRenderer::Flush()
{
  if (!m_isConfigured)
    return;

  m_overlayRenderer.Flush();
}

CDebugRenderer::CRenderer::CRenderer() : OVERLAY::CRenderer()
{
}

void CDebugRenderer::CRenderer::Render(int idx)
{
  std::vector<SElement>& list = m_buffers[idx];
  for (std::vector<SElement>::iterator it = list.begin(); it != list.end(); ++it)
  {
    COverlay* o = nullptr;

    if (it->overlay_dvd)
    {
      CDVDOverlayLibass* ovAss = static_cast<CDVDOverlayLibass*>(it->overlay_dvd);
      if (!ovAss || !ovAss->GetLibassHandler())
        continue;

      KODI::SUBTITLES::subtitlesStyle subStyle;
      bool updateStyle = !ovAss->GetLibassHandler()->IsStyleInitialized();
      if (updateStyle)
        subStyle = CreateSubtitlesStyle();

      o = ConvertLibass(ovAss, it->pts, updateStyle, subStyle);

      if (!o)
        continue;
    }

    OVERLAY::CRenderer::Render(o);
  }

  ReleaseUnused();
}

KODI::SUBTITLES::subtitlesStyle CDebugRenderer::CRenderer::CreateSubtitlesStyle()
{
  KODI::SUBTITLES::subtitlesStyle subStyle;
  subStyle.fontName = "Arial";
  subStyle.fontSize = 16.0;
  return subStyle;
}
