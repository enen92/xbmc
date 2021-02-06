/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDSubtitleParserSubrip.h"

#include "DVDCodecs/Overlay/DVDOverlayText.h"
#include "DVDSubtitleTagSami.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/StringUtils.h"

CDVDSubtitleParserSubrip::CDVDSubtitleParserSubrip(std::unique_ptr<CDVDSubtitleStream> && pStream, const std::string& strFile)
    : CDVDSubtitleParserText(std::move(pStream), strFile)
{
}

CDVDSubtitleParserSubrip::~CDVDSubtitleParserSubrip()
{
  Dispose();
}

bool CDVDSubtitleParserSubrip::Open(CDVDStreamInfo &hints)
{
  if (!CDVDSubtitleParserText::Open())
    return false;

  CDVDSubtitleTagSami TagConv;
  if (!TagConv.Init())
    return false;

  char line[1024];
  std::string strLine;

  // last processed subtitle entry
  CDVDOverlay* lastOverlay = nullptr;

  while (m_pStream->ReadLine(line, sizeof(line)))
  {
    strLine = line;
    StringUtils::Trim(strLine);

    if (!strLine.empty())
    {
      char sep;
      int hh1, mm1, ss1, ms1, hh2, mm2, ss2, ms2;
      int c = sscanf(strLine.c_str(), "%d%c%d%c%d%c%d --> %d%c%d%c%d%c%d\n",
                     &hh1, &sep, &mm1, &sep, &ss1, &sep, &ms1,
                     &hh2, &sep, &mm2, &sep, &ss2, &sep, &ms2);

      if (c == 14) // time info
      {
        double startPTS = (static_cast<double>(((hh1 * 60 + mm1) * 60) + ss1) * 1000 + ms1) * (DVD_TIME_BASE / 1000);
        auto stopPTS  = (static_cast<double>(((hh2 * 60 + mm2) * 60) + ss2) * 1000 + ms2) * (DVD_TIME_BASE / 1000);

        std::string text;
        while (m_pStream->ReadLine(line, sizeof(line)))
        {
          strLine = line;
          StringUtils::Trim(strLine);

          // empty line, next subtitle is about to start
          if (strLine.length() <= 0) break;
          
          // append text to the subtitle until the buffer ends
          if (text.empty())
          {
            text = strLine;
          }
          else
          {
            text += "\n" + strLine;
          }
        }

        // todo
        //TagConv.ConvertLine(pOverlay, strLine.c_str(), strLine.length());

        // The subtitle file may have stacked subtitles (i.e, multiple text entries for the
        // same start and stop pts) in such cases append a the new line to the previous event
        if (lastOverlay != nullptr &&
            lastOverlay->iPTSStartTime == startPTS && lastOverlay->iPTSStopTime == stopPTS)
        {
          AppendTextToLastOverlay(text);
        }
        else
        {
          auto overlay = CreateAssOverlay(startPTS, stopPTS, text);
          lastOverlay = overlay;
          overlay->Acquire();
          m_collection.Add(overlay);
        }
      }
    }
  }
  m_collection.Sort();
  return true;
}
