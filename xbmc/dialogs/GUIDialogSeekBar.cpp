/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSeekBar.h"

#include <math.h>

#include "Application.h"
#include "GUIInfoManager.h"
#include "SeekHandler.h"
#include "ServiceBroker.h"
#include "cores/Cut.h"
#include "cores/DataCacheCore.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIRangesControl.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

#define POPUP_SEEK_PROGRESS           401
#define POPUP_SEEK_EPG_EVENT_PROGRESS 402
#define POPUP_SEEK_TIMESHIFT_PROGRESS 403
#define POPUP_VIEW_EDL_RANGES         404

CGUIDialogSeekBar::CGUIDialogSeekBar(void)
  : CGUIDialog(WINDOW_DIALOG_SEEK_BAR, "DialogSeekBar.xml", DialogModalityType::MODELESS)
{
  m_loadType = LOAD_ON_GUI_INIT;    // the application class handles our resources
}

CGUIDialogSeekBar::~CGUIDialogSeekBar(void) = default;

bool CGUIDialogSeekBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    InitEDL();
    return CGUIDialog::OnMessage(message);
  case GUI_MSG_WINDOW_DEINIT:
    DeinitEDL();
    return CGUIDialog::OnMessage(message);
  case GUI_MSG_ITEM_SELECT:
    if (message.GetSenderId() == GetID() &&
        (message.GetControlId() == POPUP_SEEK_PROGRESS ||
         message.GetControlId() == POPUP_SEEK_EPG_EVENT_PROGRESS ||
         message.GetControlId() == POPUP_SEEK_TIMESHIFT_PROGRESS))
      return CGUIDialog::OnMessage(message);
    break;
  case GUI_MSG_REFRESH_TIMER:
    return CGUIDialog::OnMessage(message);
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogSeekBar::FrameMove()
{
  if (!g_application.GetAppPlayer().HasPlayer())
  {
    Close(true);
    return;
  }

  int progress = GetProgress();
  if (progress != m_lastProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_PROGRESS, m_lastProgress = progress);

  int epgEventProgress = GetEpgEventProgress();
  if (epgEventProgress != m_lastEpgEventProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_EPG_EVENT_PROGRESS, m_lastEpgEventProgress = epgEventProgress);

  int timeshiftProgress = GetTimeshiftProgress();
  if (timeshiftProgress != m_lastTimeshiftProgress)
    CONTROL_SELECT_ITEM(POPUP_SEEK_TIMESHIFT_PROGRESS, m_lastTimeshiftProgress = timeshiftProgress);

  CGUIDialog::FrameMove();
}

int CGUIDialogSeekBar::GetProgress() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;

  if (g_application.GetAppPlayer().GetSeekHandler().GetSeekSize() != 0)
    infoMgr.GetInt(progress, PLAYER_SEEKBAR);
  else
    infoMgr.GetInt(progress, PLAYER_PROGRESS);

  return progress;
}

int CGUIDialogSeekBar::GetEpgEventProgress() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;
  infoMgr.GetInt(progress, PVR_EPG_EVENT_PROGRESS);

  int seekSize = g_application.GetAppPlayer().GetSeekHandler().GetSeekSize();
  if (seekSize != 0)
  {
    int total = 0;
    infoMgr.GetInt(total, PVR_EPG_EVENT_DURATION);

    float totalTime = static_cast<float>(total);
    if (totalTime == 0.0f)
      return 0;

    float percentPerSecond = 100.0f / totalTime;
    float percent = progress + percentPerSecond * seekSize;
    percent = std::max(0.0f, std::min(percent, 100.0f));
    return std::lrintf(percent);
  }

  return progress;
}

int CGUIDialogSeekBar::GetTimeshiftProgress() const
{
  CGUIInfoManager& infoMgr = CServiceBroker::GetGUI()->GetInfoManager();

  int progress = 0;
  infoMgr.GetInt(progress, PVR_TIMESHIFT_PROGRESS_PLAY_POS);

  int seekSize = g_application.GetAppPlayer().GetSeekHandler().GetSeekSize();
  if (seekSize != 0)
  {
    int total = 0;
    infoMgr.GetInt(total, PVR_TIMESHIFT_PROGRESS_DURATION);

    float totalTime = static_cast<float>(total);
    if (totalTime == 0.0f)
      return 0;

    float percentPerSecond = 100.0f / totalTime;
    float percent = progress + percentPerSecond * seekSize;
    percent = std::max(0.0f, std::min(percent, 100.0f));
    return std::lrintf(percent);
  }

  return progress;
}

void CGUIDialogSeekBar::InitEDL()
{
  CGUIRangesControl* rangesControl = dynamic_cast<CGUIRangesControl*>(GetControl(POPUP_VIEW_EDL_RANGES));
  if (!rangesControl)
    return;

  CDataCacheCore& data = CServiceBroker::GetDataCacheCore();
  std::vector<std::pair<float, float>> ranges;

  time_t start;
  int64_t current;
  int64_t min;
  int64_t max;
  data.GetPlayTimes(start, current, min, max);

  time_t duration = max - start * 1000;
  if (duration > 0)
  {
    // cut list
    const std::vector<EDL::Cut> cuts = data.GetCutList();
    for (const auto& cut : cuts)
    {
      if (cut.action != EDL::Action::CUT &&
          cut.action != EDL::Action::COMM_BREAK)
        continue;

      float cutStart = cut.start * 100.0f / duration;
      float cutEnd = cut.end * 100.0f / duration;
      ranges.emplace_back(std::make_pair(cutStart, cutEnd));
    }

    // chapter markers
    const std::vector<std::pair<std::string, int64_t>> chapters = data.GetChapters();
    for (const auto& chapter : chapters)
    {
      float marker = chapter.second * 1000 * 100.0f / duration;
      if (marker == 0.0f || marker == 100.0f)
        continue;

      ranges.emplace_back(std::make_pair(marker, marker));
    }
  }
  rangesControl->SetRanges(ranges);
}

void CGUIDialogSeekBar::DeinitEDL()
{
  CGUIRangesControl* rangesControl = dynamic_cast<CGUIRangesControl*>(GetControl(POPUP_VIEW_EDL_RANGES));
  if (!rangesControl)
    return;

  rangesControl->ClearRanges();
}
