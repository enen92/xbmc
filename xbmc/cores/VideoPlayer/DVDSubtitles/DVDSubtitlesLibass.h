/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SubtitlesStyle.h"
#include "threads/CriticalSection.h"
#include "utils/ColorUtils.h"

#include <ass/ass.h>
#include <ass/ass_types.h>

/** Wrapper for Libass **/

static constexpr int ASS_NO_ID = -1;

enum ASSSubType
{
  NATIVE = 0,
  ADAPTED
};

class CDVDSubtitlesLibass
{
public:
  CDVDSubtitlesLibass();
  ~CDVDSubtitlesLibass();

  /*!
  * \brief Configure libass. This method groups any configurations
  * that might change throughout the lifecycle of libass (e.g. fonts)
  */
  void Configure();

  ASS_Image* RenderImage(double pts,
                         KODI::SUBTITLES::subtitleRenderOpts opts,
                         bool updateStyle,
                         KODI::SUBTITLES::subtitlesStyle subStyle,
                         int* changes = NULL);

  /*!
  * \brief Specifies whether the style has been initialised
  */
  bool IsStyleInitialized();

  /*!
  * \brief Translate and apply Kodi subtitles style to LibAss style
  */
  void ApplyStyle(KODI::SUBTITLES::subtitlesStyle subStyle);

  /*!
  * \brief Specify whether the subtitles are
  * native (loaded from ASS/SSA file or stream)
  * or adapted (converted from other types e.g. SubRip)
  */
  void SetSubtitleType(ASSSubType type) { m_subtitleType = type; };

  ASS_Event* GetEvents();

  /*!
  * \brief Get the number of events (subtitle entries) in the ASS track
  * \return The number of events in the ASS track
  */
  int GetNrOfEvents() const;

  /*!
  * \brief Decode Header of ASS/SSA, needed to properly decode
  * demux packets with DecodeDemuxPkt
  * \return True if success, false if error
  */
  bool DecodeHeader(char* data, int size);

  /*!
  * \brief Decode ASS/SSA demux packet (depends from DecodeHeader)
  * \return True if success, false if error
  */
  bool DecodeDemuxPkt(const char* data, int size, double start, double duration);

  /*!
  * \brief Create a new empty ASS track
  * \return True if success, false if error
  */
  bool CreateTrack();

  /*!
  * \brief Create a new ASS track based on an SSA buffer
  * \return True if success, false if error
  */
  bool CreateTrack(char* buf, size_t size);

  /*!
  * \brief Add an ASS event to show a subtitle on a specified time
  * \param startTime The PTS start time of the Event
  * \param stopTime The PTS stop time of the Event
  * \return Return the Event ID, otherwise ASS_NO_ID if fails
  */
  int AddEvent(const char* text, double startTime, double stopTime);

  /*!
  * \brief Append text to the specified event
  */
  void AppendTextToEvent(int eventId, const char* text);

  /*!
  * \brief Change the stop time of an Event with the specified time
  * \param eventId The ASS Event ID
  * \param stopTime The PTS stop time
  */
  void ChangeEventStopTime(int eventId, double stopTime);

  /*!
  * \brief Flush buffered events
  */
  void FlushEvents();

  /*!
  * \brief Delete old events only if the total number of events reaches the threshold
  * \param nEvents The number of events to delete
  * \param threshold Start deleting only when the number of events is reached
  * \return The updated ID of the last Event, otherwise ASS_NO_ID if error or no events
  */
  int DeleteEvents(int nEvents, int threshold);

private:
  void ConfigureAssOverride(KODI::SUBTITLES::subtitlesStyle& subStyle, ASS_Style* style);
  void ConfigureFont(bool overrideFont, std::string fontName);

  ASS_Library* m_library = nullptr;
  ASS_Track* m_track = nullptr;
  ASS_Renderer* m_renderer = nullptr;
  mutable CCriticalSection m_section;
  bool m_isConfigured;
  ASSSubType m_subtitleType;
  bool m_isStyleInitialized;
  int m_currentStyleId;
};
