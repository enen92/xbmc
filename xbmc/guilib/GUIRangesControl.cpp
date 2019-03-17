/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRangesControl.h"

#include <cmath>

CGUIRangesControl::CGUIRange::CGUIRange(const CGUITexture& lowerTexture, const CGUITexture& fillTexture,
                                        const CGUITexture& upperTexture, const std::pair<float, float>& percentages)
: m_guiLowerTexture(lowerTexture),
  m_guiFillTexture(fillTexture),
  m_guiUpperTexture(upperTexture),
  m_percentValues(percentages)
{
}

void CGUIRangesControl::CGUIRange::AllocResources()
{
  m_guiFillTexture.AllocResources();
  m_guiUpperTexture.AllocResources();
  m_guiLowerTexture.AllocResources();

  m_guiFillTexture.SetHeight(20);
  m_guiUpperTexture.SetHeight(20);
  m_guiLowerTexture.SetHeight(20);
}

void CGUIRangesControl::CGUIRange::FreeResources(bool immediately)
{
  m_guiFillTexture.FreeResources(immediately);
  m_guiUpperTexture.FreeResources(immediately);
  m_guiLowerTexture.FreeResources(immediately);
}

void CGUIRangesControl::CGUIRange::DynamicResourceAlloc(bool bOnOff)
{
  m_guiFillTexture.DynamicResourceAlloc(bOnOff);
  m_guiUpperTexture.DynamicResourceAlloc(bOnOff);
  m_guiLowerTexture.DynamicResourceAlloc(bOnOff);
}

void CGUIRangesControl::CGUIRange::SetInvalid()
{
  m_guiFillTexture.SetInvalid();
  m_guiUpperTexture.SetInvalid();
  m_guiLowerTexture.SetInvalid();
}

bool CGUIRangesControl::CGUIRange::SetDiffuseColor(const KODI::GUILIB::GUIINFO::CGUIInfoColor& color)
{
  bool changed = false;
  changed |= m_guiFillTexture.SetDiffuseColor(color);
  changed |= m_guiUpperTexture.SetDiffuseColor(color);
  changed |= m_guiLowerTexture.SetDiffuseColor(color);
  return changed;
}

bool CGUIRangesControl::CGUIRange::Process(unsigned int currentTime)
{
  bool changed = false;
  changed |= m_guiFillTexture.Process(currentTime);
  changed |= m_guiUpperTexture.Process(currentTime);
  changed |= m_guiLowerTexture.Process(currentTime);
  return changed;
}

void CGUIRangesControl::CGUIRange::Render()
{
  if (m_guiLowerTexture.GetFileName().empty() && m_guiUpperTexture.GetFileName().empty())
  {
    if (m_guiFillTexture.GetWidth() > 0)
      m_guiFillTexture.Render();
  }
  else
  {
    m_guiLowerTexture.Render();

    if (m_guiFillTexture.GetWidth() > 0)
      m_guiFillTexture.Render();

    m_guiUpperTexture.Render();
  }
}

bool CGUIRangesControl::CGUIRange::UpdateLayout(float fBackgroundTextureHeight,
                                                float fPosX, float fPosY, float fWidth,
                                                float fScaleX, float fScaleY)
{
  bool bChanged = false;

  if (m_guiLowerTexture.GetFileName().empty() && m_guiUpperTexture.GetFileName().empty())
  {
    // rendering without left and right image - fill the mid image completely
    float width = (m_percentValues.second - m_percentValues.first) * fWidth * 0.01f;
    float offsetX = m_percentValues.first * fWidth * 0.01f;
    float offsetY = std::fabs(fScaleY * 0.5f * (m_guiFillTexture.GetTextureHeight() - fBackgroundTextureHeight));
    bChanged |= m_guiFillTexture.SetPosition(fPosX + (offsetX > 0 ? offsetX : 0), fPosY + (offsetY > 0 ? offsetY : 0));
    bChanged |= m_guiFillTexture.SetHeight(fScaleY * m_guiFillTexture.GetTextureHeight());
    bChanged |= m_guiFillTexture.SetWidth(width);
  }
  else
  {
    float offsetX = m_percentValues.first * fWidth * 0.01f;
    float offsetY = std::fabs(fScaleY * 0.5f * (m_guiLowerTexture.GetTextureHeight() - fBackgroundTextureHeight));
    bChanged |= m_guiLowerTexture.SetPosition(fPosX + (offsetX > 0 ? offsetX : 0), fPosY + (offsetY > 0 ? offsetY : 0));
    bChanged |= m_guiLowerTexture.SetHeight(fScaleY * m_guiLowerTexture.GetTextureHeight());
    bChanged |= m_guiLowerTexture.SetWidth(m_guiLowerTexture.GetTextureWidth());

    if (m_percentValues.first != m_percentValues.second)
    {
      float width = (m_percentValues.second - m_percentValues.first) * fWidth * 0.01f - m_guiLowerTexture.GetTextureWidth() - m_guiUpperTexture.GetTextureWidth();

      fPosX += m_guiLowerTexture.GetTextureWidth();
      offsetY = std::fabs(fScaleY * 0.5f * (m_guiFillTexture.GetTextureHeight() - fBackgroundTextureHeight));
      bChanged |= m_guiFillTexture.SetPosition(fPosX + offsetX, fPosY + (offsetY > 0 ? offsetY : 0));
      bChanged |= m_guiFillTexture.SetHeight(fScaleY * m_guiFillTexture.GetTextureHeight());
      bChanged |= m_guiFillTexture.SetWidth(width);

      fPosX += width;
      offsetY = std::fabs(fScaleY * 0.5f * (m_guiUpperTexture.GetTextureHeight() - fBackgroundTextureHeight));
      bChanged |= m_guiUpperTexture.SetPosition(fPosX + offsetX, fPosY + (offsetY > 0 ? offsetY : 0));
      bChanged |= m_guiUpperTexture.SetHeight(fScaleY * m_guiUpperTexture.GetTextureHeight());
      bChanged |= m_guiUpperTexture.SetWidth(m_guiUpperTexture.GetTextureWidth());
    }
    else
    {
      // render only lower texture if range is zero
      bChanged |= m_guiFillTexture.SetVisible(false);
      bChanged |= m_guiUpperTexture.SetVisible(false);
    }
  }
  return bChanged;
}

CGUIRangesControl::CGUIRangesControl(int parentID, int controlID,
                                     float posX, float posY, float width, float height,
                                     const CTextureInfo& backGroundTexture,
                                     const CTextureInfo& lowerTexture,
                                     const CTextureInfo& fillTexture,
                                     const CTextureInfo& upperTexture,
                                     const CTextureInfo& overlayTexture)
: CGUIControl(parentID, controlID, posX, posY, width, height),
  m_guiBackground(posX, posY, width, height, backGroundTexture),
  m_guiOverlay(posX, posY, width, height, overlayTexture),
  m_guiLowerTexture(posX, posY, width, height, lowerTexture),
  m_guiFillTexture(posX, posY, width, height, fillTexture),
  m_guiUpperTexture(posX, posY, width, height, upperTexture)
{
  ControlType = GUICONTROL_RANGES;
}

CGUIRangesControl::~CGUIRangesControl() = default;

void CGUIRangesControl::SetPosition(float posX, float posY)
{
  // everything is positioned based on the background image position
  CGUIControl::SetPosition(posX, posY);
  m_guiBackground.SetPosition(posX, posY);
}

void CGUIRangesControl::Process(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  bool changed = false;

  if (!IsDisabled())
    changed |= UpdateLayout();

  changed |= m_guiBackground.Process(currentTime);
  changed |= m_guiOverlay.Process(currentTime);

  for (auto& range : m_ranges)
    changed |= range.Process(currentTime);

  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIRangesControl::Render()
{
  if (!IsDisabled())
  {
    m_guiBackground.Render();

    for (auto& range : m_ranges)
      range.Render();

    m_guiOverlay.Render();
  }

  CGUIControl::Render();
}


bool CGUIRangesControl::CanFocus() const
{
  return false;
}


void CGUIRangesControl::SetRanges(const std::vector<std::pair<float, float>>& ranges)
{
  m_ranges.clear();
  for (const auto& range : ranges)
    m_ranges.emplace_back(CGUIRange(m_guiLowerTexture, m_guiFillTexture, m_guiUpperTexture, range));
}

void CGUIRangesControl::ClearRanges()
{
  m_ranges.clear();
}

void CGUIRangesControl::FreeResources(bool immediately /* = false */)
{
  CGUIControl::FreeResources(immediately);

  m_guiBackground.FreeResources(immediately);
  m_guiOverlay.FreeResources(immediately);

  for (auto& range : m_ranges)
    range.FreeResources(immediately);
}

void CGUIRangesControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);

  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiOverlay.DynamicResourceAlloc(bOnOff);

  for (auto& range : m_ranges)
    range.DynamicResourceAlloc(bOnOff);
}

void CGUIRangesControl::AllocResources()
{
  CGUIControl::AllocResources();

  m_guiBackground.AllocResources();
  m_guiOverlay.AllocResources();

  m_guiBackground.SetHeight(25);
  m_guiOverlay.SetHeight(20);

  for (auto& range : m_ranges)
    range.AllocResources();
}

void CGUIRangesControl::SetInvalid()
{
  CGUIControl::SetInvalid();

  m_guiBackground.SetInvalid();
  m_guiOverlay.SetInvalid();

  for (auto& range : m_ranges)
    range.SetInvalid();
}

bool CGUIRangesControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();

  changed |= m_guiBackground.SetDiffuseColor(m_diffuseColor);
  changed |= m_guiOverlay.SetDiffuseColor(m_diffuseColor);

  for (auto& range : m_ranges)
    changed |= range.SetDiffuseColor(m_diffuseColor);

  return changed;
}

bool CGUIRangesControl::UpdateLayout()
{
  bool bChanged(false);

  if (m_width == 0)
    m_width = m_guiBackground.GetTextureWidth();

  if (m_height == 0)
    m_height = m_guiBackground.GetTextureHeight();

  bChanged |= m_guiBackground.SetHeight(m_height);
  bChanged |= m_guiBackground.SetWidth(m_width);

  float fScaleX = m_guiBackground.GetTextureWidth() ? m_width / m_guiBackground.GetTextureWidth() : 1.0f;
  float fScaleY = m_guiBackground.GetTextureHeight() ? m_height / m_guiBackground.GetTextureHeight() : 1.0f;

  float posX = m_guiBackground.GetXPosition();
  float posY = m_guiBackground.GetYPosition();

  for (auto& range : m_ranges)
    bChanged |= range.UpdateLayout(m_guiBackground.GetTextureHeight(), posX, posY, m_width, fScaleX, fScaleY);

  float offset = std::fabs(fScaleY * 0.5f * (m_guiOverlay.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
  if (offset > 0)  //  Center texture to the background if necessary
    bChanged |= m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition() + offset);
  else
    bChanged |= m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition());

  bChanged |= m_guiOverlay.SetHeight(fScaleY * m_guiOverlay.GetTextureHeight());
  bChanged |= m_guiOverlay.SetWidth(fScaleX * m_guiOverlay.GetTextureWidth());

  return bChanged;
}
