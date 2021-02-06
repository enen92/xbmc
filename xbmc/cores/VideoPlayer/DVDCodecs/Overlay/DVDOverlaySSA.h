/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../DVDSubtitles/DVDSubtitlesLibass.h"
#include "DVDOverlay.h"

#include "system.h" // for SAFE_RELEASE

class CDVDOverlaySSA : public CDVDOverlay
{
public:
  
  enum ASSSubType {
    NATIVE = 0,
    ADAPTED
  };
  
  explicit CDVDOverlaySSA(CDVDSubtitlesLibass* libass) : CDVDOverlay(DVDOVERLAY_TYPE_SSA)
  {
    replace = true;
    m_libass = libass;
    libass->Acquire();
  }

  CDVDOverlaySSA(CDVDOverlaySSA& src)
    : CDVDOverlay(src)
    , m_libass(src.m_libass), m_subType(src.m_subType)
  {
    m_libass->Acquire();
  }

  ~CDVDOverlaySSA() override
  {
    if(m_libass)
      SAFE_RELEASE(m_libass);
  }

  CDVDOverlaySSA* Clone() override
  {
    return new CDVDOverlaySSA(*this);
  }
  
  void setSubType(ASSSubType type)
  {
    m_subType = type;
  }
  
  const ASSSubType getSubType() const
  {
    return m_subType;
  }
  
  CDVDSubtitlesLibass* getLibassHandler()
  {
    return m_libass;
  }

private:
  CDVDSubtitlesLibass* m_libass;
  ASSSubType m_subType = ASSSubType::NATIVE;
};
