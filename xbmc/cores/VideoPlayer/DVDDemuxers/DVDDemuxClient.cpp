/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxClient.h"

#include "DVDDemuxUtils.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "cores/FFmpeg.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

extern "C" {
#include <libavcodec/bsf.h>
}

#include <utility>

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - AV_INPUT_BUFFER_PADDING_SIZE)

class CDemuxStreamClientInternal
{
public:
  ~CDemuxStreamClientInternal()
  {
    DisposeParser();
  }

  void DisposeParser()
  {
    if (m_parser)
    {
      av_parser_close(m_parser);
      m_parser = nullptr;
    }
    if (m_context)
    {
      avcodec_free_context(&m_context);
      m_context = nullptr;
    }
  }

  AVCodecParserContext *m_parser = nullptr;
  AVCodecContext *m_context = nullptr;
  bool m_parser_split = false;
};

template <class T>
class CDemuxStreamClientInternalTpl : public CDemuxStreamClientInternal, public T
{
};

CDVDDemuxClient::CDVDDemuxClient() : CDVDDemux()
{
  m_streams.clear();
}

CDVDDemuxClient::~CDVDDemuxClient()
{
  Dispose();
}

bool CDVDDemuxClient::Open(std::shared_ptr<CDVDInputStream> pInput)
{
  Abort();

  m_pInput = std::move(pInput);
  m_IDemux = std::dynamic_pointer_cast<CDVDInputStream::IDemux>(m_pInput);
  if (!m_IDemux)
    return false;

  if (!m_IDemux->OpenDemux())
    return false;

  RequestStreams();

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
  return true;
}

void CDVDDemuxClient::Dispose()
{
  DisposeStreams();

  m_pInput = nullptr;
  m_IDemux = nullptr;
}

void CDVDDemuxClient::DisposeStreams()
{
  m_streams.clear();
  m_videoStreamPlaying = -1;
}

bool CDVDDemuxClient::Reset()
{
  std::shared_ptr<CDVDInputStream> pInputStream = m_pInput;
  Dispose();
  return Open(pInputStream);
}

void CDVDDemuxClient::Abort()
{
  if (m_IDemux)
    m_IDemux->AbortDemux();
}

void CDVDDemuxClient::Flush()
{
  if (m_IDemux)
    m_IDemux->FlushDemux();

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
}

int CDVDDemuxClient::GetPacketExtradata(const DemuxPacket* pkt,
                                        const AVCodecParserContext* parserCtx,
                                        AVCodecContext* codecCtx,
                                        uint8_t** p_extradata)
{
  int extradata_size = 0;

  if (!pkt || !p_extradata)
    return 0;

  *p_extradata = nullptr;

#if LIBAVFORMAT_BUILD >= AV_VERSION_INT(59, 0, 100)
  AVBSFContext* bsf = nullptr;
  AVPacket* dst_pkt = nullptr;
  const AVBitStreamFilter* f;
  AVPacket* pkt_ref = nullptr;
  AVPacket* src_pkt = nullptr;
  int ret = 0;
  uint8_t* ret_extradata = nullptr;
  size_t ret_extradata_size = 0;

  f = av_bsf_get_by_name("extract_extradata");
  if (!f)
    return 0;

  bsf = nullptr;
  ret = av_bsf_alloc(f, &bsf);
  if (ret < 0)
    return 0;

  bsf->par_in->codec_id = codecCtx->codec_id;

  ret = av_bsf_init(bsf);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    return 0;
  }

  src_pkt = av_packet_alloc();
  if (!src_pkt)
  {
    av_bsf_free(&bsf);
    return 0;
  }
  src_pkt->data = pkt->pData;
  src_pkt->size = pkt->iSize;
  src_pkt->dts = src_pkt->pts = AV_NOPTS_VALUE;

  dst_pkt = av_packet_alloc();
  pkt_ref = dst_pkt;

  ret = av_packet_ref(pkt_ref, src_pkt);
  if (ret < 0)
  {
    av_bsf_free(&bsf);
    av_packet_free(&dst_pkt);
    av_packet_free(&src_pkt);
    return 0;
  }

  ret = av_bsf_send_packet(bsf, pkt_ref);
  if (ret < 0)
  {
    av_packet_unref(pkt_ref);
    av_bsf_free(&bsf);
    av_packet_free(&dst_pkt);
    av_packet_free(&src_pkt);
    return 0;
  }

  ret = 0;
  while (ret >= 0)
  {
    ret = av_bsf_receive_packet(bsf, pkt_ref);
    if (ret < 0)
    {
      if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        break;

      continue;
    }

    ret_extradata =
        av_packet_get_side_data(pkt_ref, AV_PKT_DATA_NEW_EXTRADATA, &ret_extradata_size);
    if (ret_extradata && ret_extradata_size > 0 && ret_extradata_size < FF_MAX_EXTRADATA_SIZE)
    {
      *p_extradata = (uint8_t*)av_malloc(ret_extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
      if (!*p_extradata)
      {
        CLog::Log(LOGERROR, "{} - failed to allocate {} bytes for extradata", __FUNCTION__,
                  ret_extradata_size);

        av_packet_unref(pkt_ref);
        av_bsf_free(&bsf);
        av_packet_free(&dst_pkt);
        av_packet_free(&src_pkt);
        return 0;
      }

      CLog::Log(LOGDEBUG, "{} - fetching extradata, extradata_size({})", __FUNCTION__,
                ret_extradata_size);

      memcpy(*p_extradata, ret_extradata, ret_extradata_size);
      memset(*p_extradata + ret_extradata_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
      extradata_size = ret_extradata_size;

      av_packet_unref(pkt_ref);
      break;
    }

    av_packet_unref(pkt_ref);
  }

  av_bsf_free(&bsf);
  av_packet_free(&dst_pkt);
  av_packet_free(&src_pkt);
#else
  if (codecCtx && parserCtx && parserCtx->parser && parserCtx->parser->split)
    extradata_size = parserCtx->parser->split(codecCtx, pkt->pData, pkt->iSize);

  if (extradata_size <= 0 && extradata_size >= FF_MAX_EXTRADATA_SIZE)
  {
    CLog::Log(LOGDEBUG, "{} - fetched extradata of weird size {}", __FUNCTION__, extradata_size);
    return 0;
  }

  *p_extradata = (uint8_t*)av_malloc(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
  if (!*p_extradata)
  {
    CLog::Log(LOGERROR, "{} - failed to allocate {} bytes for extradata", __FUNCTION__,
              extradata_size);
    return 0;
  }

  CLog::Log(LOGDEBUG, "{} - fetching extradata, extradata_size({})", __FUNCTION__, extradata_size);

  memcpy(*p_extradata, pkt->pData, extradata_size);
  memset(*p_extradata + extradata_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
#endif

  return extradata_size;
}

bool CDVDDemuxClient::ParsePacket(DemuxPacket* pkt)
{
  bool change = false;

  CDemuxStream* st = GetStream(pkt->iStreamId);
  if (st == nullptr || st->changes < 0 || st->ExtraSize || !CodecHasExtraData(st->codec))
    return change;

  CDemuxStreamClientInternal* stream = dynamic_cast<CDemuxStreamClientInternal*>(st);

  if (stream == nullptr ||
     stream->m_parser == nullptr)
    return change;

  if (stream->m_context == nullptr)
  {
    FFMPEG_FMT_CONST AVCodec* codec = avcodec_find_decoder(st->codec);
    if (codec == nullptr)
    {
      CLog::Log(LOGERROR, "{} - can't find decoder", __FUNCTION__);
      stream->DisposeParser();
      return change;
    }

    stream->m_context = avcodec_alloc_context3(codec);
    if (stream->m_context == nullptr)
    {
      CLog::Log(LOGERROR, "{} - can't allocate context", __FUNCTION__);
      stream->DisposeParser();
      return change;
    }
    stream->m_context->time_base.num = 1;
    stream->m_context->time_base.den = DVD_TIME_BASE;
  }

  if (stream->m_parser_split && stream->m_parser && stream->m_parser->parser)
  {
    uint8_t* old_extradata = st->ExtraData;
    int len = GetPacketExtradata(pkt, stream->m_parser, stream->m_context,
                                 &st->ExtraData);
    if (len > 0)
    {
      if (old_extradata)
        av_free(old_extradata);
      st->changes++;
      st->disabled = false;
      st->ExtraSize = len;
      stream->m_parser_split = false;
      change = true;
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - split extradata");

      // Allow ffmpeg to transport codec information to stream->m_context
      if (!avcodec_open2(stream->m_context, stream->m_context->codec, nullptr))
      {
        AVPacket* avpkt = av_packet_alloc();
        if (!avpkt)
        {
          CLog::Log(LOGERROR, "CDVDDemuxClient::{} - av_packet_alloc failed: {}", __FUNCTION__,
                    strerror(errno));
          return false;
        }
        avpkt->data = pkt->pData;
        avpkt->size = pkt->iSize;
        avpkt->dts = avpkt->pts = AV_NOPTS_VALUE;
        avcodec_send_packet(stream->m_context, avpkt);
        avcodec_close(stream->m_context);
        av_packet_free(&avpkt);
      }
    }
  }

  uint8_t *outbuf = nullptr;
  int outbuf_size = 0;
  int len = av_parser_parse2(stream->m_parser,
                             stream->m_context, &outbuf, &outbuf_size,
                             pkt->pData, pkt->iSize,
                             (int64_t)(pkt->pts * DVD_TIME_BASE),
                             (int64_t)(pkt->dts * DVD_TIME_BASE),
                             0);

  // our parse is setup to parse complete frames, so we don't care about outbufs
  if (len >= 0)
  {
    if (stream->m_context->profile != st->profile &&
        stream->m_context->profile != FF_PROFILE_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) profile changed from {} to {}", st->uniqueId, st->profile, stream->m_context->profile);
      st->profile = stream->m_context->profile;
      st->changes++;
      st->disabled = false;
    }

    if (stream->m_context->level != st->level &&
        stream->m_context->level != FF_LEVEL_UNKNOWN)
    {
      CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) level changed from {} to {}", st->uniqueId, st->level, stream->m_context->level);
      st->level = stream->m_context->level;
      st->changes++;
      st->disabled = false;
    }

    switch (st->type)
    {
      case STREAM_AUDIO:
      {
        CDemuxStreamClientInternalTpl<CDemuxStreamAudio>* sta = static_cast<CDemuxStreamClientInternalTpl<CDemuxStreamAudio>*>(st);
#if LIBAVCODEC_BUILD >= AV_VERSION_INT(59, 37, 100) && \
    LIBAVUTIL_BUILD >= AV_VERSION_INT(57, 28, 100)
        int stream_channels = stream->m_context->ch_layout.nb_channels;
#else
        int stream_channels = stream->m_context->channels;
#endif
        if (stream_channels != sta->iChannels && stream_channels != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) channels changed from {} to {}",
                    st->uniqueId, sta->iChannels, stream_channels);
          sta->iChannels = stream_channels;
          sta->changes++;
          sta->disabled = false;
        }
        if (stream->m_context->sample_rate != sta->iSampleRate &&
            stream->m_context->sample_rate != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) samplerate changed from {} to {}", st->uniqueId, sta->iSampleRate, stream->m_context->sample_rate);
          sta->iSampleRate = stream->m_context->sample_rate;
          sta->changes++;
          sta->disabled = false;
        }
        if (stream_channels)
          st->changes = -1; // stop parsing
        break;
      }
      case STREAM_VIDEO:
      {
        CDemuxStreamClientInternalTpl<CDemuxStreamVideo>* stv = static_cast<CDemuxStreamClientInternalTpl<CDemuxStreamVideo>*>(st);
        if (stream->m_parser->width != stv->iWidth && stream->m_parser->width != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) width changed from {} to {}",
                    st->uniqueId, stv->iWidth, stream->m_parser->width);
          stv->iWidth = stream->m_parser->width;
          stv->changes++;
          stv->disabled = false;
        }
        if (stream->m_parser->height != stv->iHeight && stream->m_parser->height != 0)
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) height changed from {} to {}",
                    st->uniqueId, stv->iHeight, stream->m_parser->height);
          stv->iHeight = stream->m_parser->height;
          stv->changes++;
          stv->disabled = false;
        }
        if (stream->m_context->sample_aspect_ratio.num && stream->m_context->height)
        {
          double fAspect =
              (av_q2d(stream->m_context->sample_aspect_ratio) * stream->m_context->width) /
              stream->m_context->height;
          if (abs(fAspect - stv->fAspect) > 0.001 && fAspect >= 0.001)
          {
            CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) aspect changed from {} to {}",
                      st->uniqueId, stv->fAspect, fAspect);
            stv->fAspect = fAspect;
            stv->changes++;
            stv->disabled = false;
          }
        }
        if (stream->m_context->framerate.num &&
            (stv->iFpsRate != stream->m_context->framerate.num ||
             stv->iFpsScale != stream->m_context->framerate.den))
        {
          CLog::Log(LOGDEBUG, "CDVDDemuxClient::ParsePacket - ({}) fps changed from {}/{} to {}/{}",
                    st->uniqueId, stv->iFpsRate, stv->iFpsScale, stream->m_context->framerate.num,
                    stream->m_context->framerate.den);
          stv->iFpsRate = stream->m_context->framerate.num;
          stv->iFpsScale = stream->m_context->framerate.den;
          stv->changes++;
          stv->disabled = false;
        }

        break;
      }

      default:
        break;
    }
  }
  else
    CLog::Log(LOGDEBUG, "{} - parser returned error {}", __FUNCTION__, len);

  return change;
}

DemuxPacket* CDVDDemuxClient::Read()
{
  if (!m_IDemux)
    return nullptr;

  if (m_packet)
    return m_packet.release();

  m_packet.reset(m_IDemux->ReadDemux());
  if (!m_packet)
  {
    return nullptr;
  }

  if (m_packet->iStreamId == DMX_SPECIALID_STREAMINFO)
  {
    RequestStreams();
    CDVDDemuxUtils::FreeDemuxPacket(m_packet.release());
    return CDVDDemuxUtils::AllocateDemuxPacket(0);
  }
  else if (m_packet->iStreamId == DMX_SPECIALID_STREAMCHANGE)
  {
    RequestStreams();
  }
  else if (m_packet->iStreamId >= 0 && m_streams.count(m_packet->iStreamId) > 0)
  {
    if (ParsePacket(m_packet.get()))
    {
      RequestStreams();
      DemuxPacket *pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
      pPacket->iStreamId = DMX_SPECIALID_STREAMCHANGE;
      pPacket->demuxerId = m_demuxerId;
      return pPacket;
    }
  }

  if (!IsVideoReady())
  {
    CDVDDemuxUtils::FreeDemuxPacket(m_packet.release());
    DemuxPacket *pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
    pPacket->demuxerId = m_demuxerId;
    return pPacket;
  }

  //! @todo drop this block
  CDVDInputStream::IDisplayTime *inputStream = m_pInput->GetIDisplayTime();
  if (inputStream)
  {
    int dispTime = inputStream->GetTime();
    if (m_displayTime != dispTime)
    {
      m_displayTime = dispTime;
      if (m_packet->dts != DVD_NOPTS_VALUE)
      {
        m_dtsAtDisplayTime = m_packet->dts;
      }
    }
    if (m_dtsAtDisplayTime != DVD_NOPTS_VALUE && m_packet->dts != DVD_NOPTS_VALUE)
    {
      m_packet->dispTime = m_displayTime;
      m_packet->dispTime += DVD_TIME_TO_MSEC(m_packet->dts - m_dtsAtDisplayTime);
    }
  }

  return m_packet.release();
}

CDemuxStream* CDVDDemuxClient::GetStream(int iStreamId) const
{
  auto stream = m_streams.find(iStreamId);
  if (stream == m_streams.end())
    return nullptr;

  return stream->second.get();
}

std::vector<CDemuxStream*> CDVDDemuxClient::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  streams.reserve(m_streams.size());
  for (auto &st : m_streams)
    streams.push_back(st.second.get());

  return streams;
}

void CDVDDemuxClient::RequestStreams()
{
  std::map<int, std::shared_ptr<CDemuxStream>> newStreamMap;
  for (auto stream : m_IDemux->GetStreams())
    SetStreamProps(stream, newStreamMap, false);
  m_streams = newStreamMap;
}

void CDVDDemuxClient::SetStreamProps(CDemuxStream *stream, std::map<int, std::shared_ptr<CDemuxStream>> &map, bool forceInit)
{
  if (!stream)
  {
    CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid stream");
    DisposeStreams();
    return;
  }

  std::shared_ptr<CDemuxStream> currentStream(GetStreamInternal(stream->uniqueId));
  std::shared_ptr<CDemuxStream> toStream;

  if (stream->type == STREAM_AUDIO)
  {
    CDemuxStreamAudio *source = dynamic_cast<CDemuxStreamAudio*>(stream);
    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid audio stream with id {}",
                stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamAudio>> streamAudio;
    if (currentStream)
      streamAudio = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamAudio>>(currentStream);
    if (forceInit || !streamAudio || streamAudio->codec != source->codec)
    {
      streamAudio.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamAudio>());
      streamAudio->m_parser = av_parser_init(source->codec);
      if (streamAudio->m_parser)
        streamAudio->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      streamAudio->iSampleRate = source->iSampleRate;
      streamAudio->iChannels = source->iChannels;
    }

    streamAudio->iBlockAlign     = source->iBlockAlign;
    streamAudio->iBitRate        = source->iBitRate;
    streamAudio->iBitsPerSample  = source->iBitsPerSample;
    if (source->ExtraSize > 0 && source->ExtraData)
    {
      delete[] streamAudio->ExtraData;
      streamAudio->ExtraData = new uint8_t[source->ExtraSize];
      streamAudio->ExtraSize = source->ExtraSize;
      for (unsigned int j=0; j<source->ExtraSize; j++)
        streamAudio->ExtraData[j] = source->ExtraData[j];
    }
    streamAudio->m_parser_split = true;
    streamAudio->changes++;
    map[stream->uniqueId] = streamAudio;
    toStream = streamAudio;
  }
  else if (stream->type == STREAM_VIDEO)
  {
    CDemuxStreamVideo *source = dynamic_cast<CDemuxStreamVideo*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid video stream with id {}",
                stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamVideo>> streamVideo;
    if (currentStream)
      streamVideo = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamVideo>>(currentStream);
    if (forceInit || !streamVideo || streamVideo->codec != source->codec)
    {
      streamVideo.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamVideo>());
      streamVideo->m_parser = av_parser_init(source->codec);
      if (streamVideo->m_parser)
        streamVideo->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
      streamVideo->iHeight = source->iHeight;
      streamVideo->iWidth = source->iWidth;
      streamVideo->fAspect = source->fAspect;
      streamVideo->iFpsScale = source->iFpsScale;
      streamVideo->iFpsRate = source->iFpsRate;
    }
    streamVideo->iBitRate = source->iBitRate;
    if (source->ExtraSize > 0 && source->ExtraData)
    {
      delete[] streamVideo->ExtraData;
      streamVideo->ExtraData = new uint8_t[source->ExtraSize];
      streamVideo->ExtraSize = source->ExtraSize;
      for (unsigned int j=0; j<source->ExtraSize; j++)
        streamVideo->ExtraData[j] = source->ExtraData[j];
    }
    streamVideo->colorPrimaries = source->colorPrimaries;
    streamVideo->colorRange = source->colorRange;
    streamVideo->colorSpace = source->colorSpace;
    streamVideo->colorTransferCharacteristic = source->colorTransferCharacteristic;
    streamVideo->masteringMetaData = source->masteringMetaData;
    streamVideo->contentLightMetaData = source->contentLightMetaData;

    streamVideo->m_parser_split = true;
    streamVideo->changes++;
    map[stream->uniqueId] = streamVideo;
    toStream = streamVideo;
  }
  else if (stream->type == STREAM_SUBTITLE)
  {
    CDemuxStreamSubtitle *source = dynamic_cast<CDemuxStreamSubtitle*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid subtitle stream with id {}",
                stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamSubtitle>> streamSubtitle;
    if (currentStream)
      streamSubtitle = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamSubtitle>>(currentStream);
    if (!streamSubtitle || streamSubtitle->codec != source->codec)
    {
      streamSubtitle.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamSubtitle>());
      streamSubtitle->m_parser = av_parser_init(source->codec);
      if (streamSubtitle->m_parser)
        streamSubtitle->m_parser->flags |= PARSER_FLAG_COMPLETE_FRAMES;
    }

    if (source->ExtraSize == 4)
    {
      delete[] streamSubtitle->ExtraData;
      streamSubtitle->ExtraData = new uint8_t[4];
      streamSubtitle->ExtraSize = 4;
      for (int j=0; j<4; j++)
        streamSubtitle->ExtraData[j] = source->ExtraData[j];
    }
    map[stream->uniqueId] = streamSubtitle;
    toStream = streamSubtitle;
  }
  else if (stream->type == STREAM_TELETEXT)
  {
    CDemuxStreamTeletext *source = dynamic_cast<CDemuxStreamTeletext*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid teletext stream with id {}",
                stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamTeletext>> streamTeletext;
    if (currentStream)
       streamTeletext = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamTeletext>>(currentStream);
    if (!streamTeletext || streamTeletext->codec != source->codec)
    {
      streamTeletext.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamTeletext>());
    }

    map[stream->uniqueId] = streamTeletext;
    toStream = streamTeletext;
  }
  else if (stream->type == STREAM_RADIO_RDS)
  {
    CDemuxStreamRadioRDS *source = dynamic_cast<CDemuxStreamRadioRDS*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid radio-rds stream with id {}",
                stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamRadioRDS>> streamRDS;
    if (currentStream)
      streamRDS = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamRadioRDS>>(currentStream);
    if (!streamRDS || streamRDS->codec != source->codec)
    {
      streamRDS.reset(new CDemuxStreamClientInternalTpl<CDemuxStreamRadioRDS>());
    }

    map[stream->uniqueId] = streamRDS;
    toStream = streamRDS;
  }
  else if (stream->type == STREAM_AUDIO_ID3)
  {
    CDemuxStreamAudioID3* source = dynamic_cast<CDemuxStreamAudioID3*>(stream);

    if (!source)
    {
      CLog::Log(LOGERROR, "CDVDDemuxClient::RequestStream - invalid audio ID3 stream with id {}",
                stream->uniqueId);
      DisposeStreams();
      return;
    }

    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStreamAudioID3>> streamID3;
    if (currentStream)
      streamID3 = std::dynamic_pointer_cast<CDemuxStreamClientInternalTpl<CDemuxStreamAudioID3>>(
          currentStream);
    if (!streamID3 || streamID3->codec != source->codec)
    {
      streamID3 = std::make_shared<CDemuxStreamClientInternalTpl<CDemuxStreamAudioID3>>();
    }

    map[stream->uniqueId] = streamID3;
    toStream = streamID3;
  }
  else
  {
    std::shared_ptr<CDemuxStreamClientInternalTpl<CDemuxStream>> streamGen;
    streamGen = std::make_shared<CDemuxStreamClientInternalTpl<CDemuxStream>>();
    map[stream->uniqueId] = streamGen;
    toStream = streamGen;
  }

  // only update profile / level if we create a new stream
  // existing streams may be corrected by ParsePacket
  if (!currentStream || !CodecHasExtraData(stream->codec))
  {
    toStream->profile = stream->profile;
    toStream->level = stream->level;
  }

  toStream->uniqueId = stream->uniqueId;
  toStream->codec = stream->codec;
  toStream->codecName = stream->codecName;
  toStream->codec_fourcc = stream->codec_fourcc;
  toStream->flags = stream->flags;
  toStream->cryptoSession = stream->cryptoSession;
  toStream->externalInterfaces = stream->externalInterfaces;
  toStream->language = stream->language;
  toStream->name = stream->name;

  CLog::Log(LOGDEBUG, "CDVDDemuxClient::RequestStream(): added/updated stream {} with codec_id {}",
            toStream->uniqueId, toStream->codec);
}

std::shared_ptr<CDemuxStream> CDVDDemuxClient::GetStreamInternal(int iStreamId)
{
  auto stream = m_streams.find(iStreamId);
  if (stream != m_streams.end())
  {
    return stream->second;
  }
  else
    return nullptr;
}

int CDVDDemuxClient::GetNrOfStreams() const
{
  return m_streams.size();
}

bool CDVDDemuxClient::IsVideoReady()
{
  for (const auto& stream : m_streams)
  {
    if (stream.first == m_videoStreamPlaying &&
        stream.second->type == STREAM_VIDEO &&
        CodecHasExtraData(stream.second->codec) &&
        stream.second->ExtraData == nullptr)
      return false;
  }
  return true;
}

std::string CDVDDemuxClient::GetFileName()
{
  if (m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

std::string CDVDDemuxClient::GetStreamCodecName(int iStreamId)
{
  CDemuxStream *stream = GetStream(iStreamId);
  std::string strName;
  if (stream)
  {
    if (stream->codec == AV_CODEC_ID_AC3)
      strName = "ac3";
    else if (stream->codec == AV_CODEC_ID_MP2)
      strName = "mp2";
    else if (stream->codec == AV_CODEC_ID_AAC)
      strName = "aac";
    else if (stream->codec == AV_CODEC_ID_DTS)
      strName = "dca";
    else if (stream->codec == AV_CODEC_ID_MPEG2VIDEO)
      strName = "mpeg2video";
    else if (stream->codec == AV_CODEC_ID_H264)
      strName = "h264";
    else if (stream->codec == AV_CODEC_ID_EAC3)
      strName = "eac3";
    else if (stream->codec == AV_CODEC_ID_VP8)
      strName = "vp8";
    else if (stream->codec == AV_CODEC_ID_VP9)
      strName = "vp9";
    else if (stream->codec == AV_CODEC_ID_HEVC)
      strName = "hevc";
    else if (stream->codec == AV_CODEC_ID_AV1)
      strName = "av1";
  }
  return strName;
}

bool CDVDDemuxClient::SeekTime(double timems, bool backwards, double *startpts)
{
  if (m_IDemux)
  {
    m_displayTime = 0;
    m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
    return m_IDemux->SeekTime(timems, backwards, startpts);
  }
  return false;
}

void CDVDDemuxClient::SetSpeed (int speed)
{
  if (m_IDemux)
  {
    m_IDemux->SetSpeed(speed);
  }
}

void CDVDDemuxClient::FillBuffer(bool mode)
{
  if (m_IDemux)
  {
    m_IDemux->FillBuffer(mode);
  }
}

void CDVDDemuxClient::EnableStream(int id, bool enable)
{
  if (m_IDemux)
  {
    m_IDemux->EnableStream(id, enable);
  }
}

void CDVDDemuxClient::OpenStream(int id)
{
  // OpenStream may change some parameters
  // in this case we need to reset our stream properties
  if (m_IDemux)
  {
    bool bOpenStream = m_IDemux->OpenStream(id);

    CDemuxStream *stream(m_IDemux->GetStream(id));
    if (stream && stream->type == STREAM_VIDEO)
      m_videoStreamPlaying = id;

    if (bOpenStream)
      SetStreamProps(stream, m_streams, true);
  }
}

void CDVDDemuxClient::SetVideoResolution(unsigned int width, unsigned int height)
{
  if (m_IDemux)
  {
    m_IDemux->SetVideoResolution(width, height, width, height);
  }
}

bool CDVDDemuxClient::CodecHasExtraData(AVCodecID id)
{
  switch (id)
  {
  case AV_CODEC_ID_VP9:
    return false;
  default:
    return true;
  }
}
