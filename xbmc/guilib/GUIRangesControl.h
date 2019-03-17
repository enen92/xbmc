/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRangesControl.h
\brief
*/

#include <utility>
#include <vector>

#include "GUIControl.h"
#include "GUITexture.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIRangesControl : public CGUIControl
{
  class CGUIRange
  {
  public:
    CGUIRange(const CGUITexture& lowerTexture, const CGUITexture& fillTexture,
              const CGUITexture& upperTexture, const std::pair<float, float>& percentages);

    void AllocResources();
    void FreeResources(bool immediately);
    void DynamicResourceAlloc(bool bOnOff);
    void SetInvalid();
    bool SetDiffuseColor(const KODI::GUILIB::GUIINFO::CGUIInfoColor& color);

    bool Process(unsigned int currentTime);
    void Render();
    bool UpdateLayout(float fBackgroundTextureHeight, float fPosX, float fPosY, float fWidth, float fScaleX, float fScaleY);

  private:
    CGUITexture m_guiLowerTexture;
    CGUITexture m_guiFillTexture;
    CGUITexture m_guiUpperTexture;
    std::pair<float,float> m_percentValues;
  };

public:
  CGUIRangesControl(int parentID, int controlID, float posX, float posY,
                    float width, float height, const CTextureInfo& backGroundTexture,
                    const CTextureInfo& leftTexture, const CTextureInfo& midTexture,
                    const CTextureInfo& rightTexture, const CTextureInfo& overlayTexture);
  ~CGUIRangesControl() override;
  CGUIRangesControl* Clone() const override { return new CGUIRangesControl(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float posX, float posY) override;

  void SetRanges(const std::vector<std::pair<float, float>>& ranges);
  void ClearRanges();

protected:
  bool UpdateColors() override;
  bool UpdateLayout();

  CGUITexture m_guiBackground;
  CGUITexture m_guiOverlay;
  const CGUITexture m_guiLowerTexture;
  const CGUITexture m_guiFillTexture;
  const CGUITexture m_guiUpperTexture;
  std::vector<CGUIRange> m_ranges;
};
