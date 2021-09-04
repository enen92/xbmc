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
  Dispose();
}

void CDebugRenderer::Initialize()
{
  if (m_isInitialized)
    return;

  m_adapter = new CSubtitlesAdapter();

  for (int i = 0; i < 6; i++)
  {
    m_strDebug[i] = " ";
  }

  m_isInitialized = m_adapter->Initialize();
  if (!m_isInitialized)
    CLog::Log(LOGERROR, "{} - Failed to configure OSD info debug renderer", __FUNCTION__);

  // We create only a single overlay with a fixed PTS for each rendered frame
  m_overlayRenderer.AddOverlay(m_adapter->CreateOverlay(), 1000000., 0);
}

void CDebugRenderer::Dispose()
{
  m_isInitialized = false;
  m_overlayRenderer.Flush();
  if (m_adapter)
  {
    delete m_adapter;
    m_adapter = nullptr;
  }
}

void CDebugRenderer::SetInfo(DEBUG_INFO_PLAYER& info)
{
  if (!m_isInitialized)
    return;

  // FIXME: We currently force ASS_Event's and the current PTS
  // of rendering with fixed values to allow perpetual
  // display of on-screen text. It would be appropriate for Libass
  // provide a way to allow fixed on-screen text display
  // without use all these fixed values.
  m_adapter->FlushSubtitles();
  m_adapter->AddSubtitle(info.audio.c_str(), 0., 5000000.);
  m_adapter->AddSubtitle(info.video.c_str(), 0., 5000000.);
  m_adapter->AddSubtitle(info.player.c_str(), 0., 5000000.);
  m_adapter->AddSubtitle(info.vsync.c_str(), 0., 5000000.);
}

void CDebugRenderer::SetInfo(DEBUG_INFO_VIDEO& video, DEBUG_INFO_RENDER& render)
{
  if (!m_isInitialized)
    return;

  // FIXME: We currently force ASS_Event's and the current PTS
  // of rendering with fixed values to allow perpetual
  // display of on-screen text. It would be appropriate for Libass
  // provide a way to allow fixed on-screen text display
  // without use all these fixed values.

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
}

void CDebugRenderer::Render(CRect& src, CRect& dst, CRect& view)
{
  if (!m_isInitialized)
    return;

  m_overlayRenderer.SetVideoRect(src, dst, view);
  m_overlayRenderer.Render(0);
}

void CDebugRenderer::Flush()
{
  if (!m_isInitialized)
    return;

  m_adapter->FlushSubtitles();

  for (int i = 0; i < 6; i++)
  {
    m_strDebug[i] = " ";
  }
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
      bool updateStyle = !m_debugOverlayStyle;
      if (updateStyle)
        CreateSubtitlesStyle();

      o = ConvertLibass(ovAss, it->pts, updateStyle, m_debugOverlayStyle);

      if (!o)
        continue;
    }

    OVERLAY::CRenderer::Render(o);
  }
}

void CDebugRenderer::CRenderer::CreateSubtitlesStyle()
{
  m_debugOverlayStyle = std::make_shared<KODI::SUBTITLES::subtitlesStyle>();
  m_debugOverlayStyle->fontName = "arial.ttf";
  m_debugOverlayStyle->fontSize = 16.0;
}
