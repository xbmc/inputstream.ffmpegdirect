/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "FFmpegStream.h"

// #include <iostream>
// #include <map>
// #include <string.h>
// #include <sstream>
// #include <librtmp/log.h>
// #include <librtmp/rtmp.h>

#include "threads/SingleLock.h"
#include "url/URL.h"
#include "FFmpegLog.h"
#include "../utils/Log.h"

#include "IManageDemuxPacket.h"


// #include <kodi/addon-instance/Inputstream.h>

#include <chrono>
#include <ctime>

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef TARGET_POSIX
#include <stdint.h>
#endif

extern "C" {
#include <libavutil/dict.h>
#include <libavutil/opt.h>
}

//#include "platform/posix/XTimeUtils.h"

#include <p8-platform/util/StringUtils.h>

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

struct StereoModeConversionMap
{
  const char*          name;
  const char*          mode;
};

// we internally use the matroska string representation of stereoscopic modes.
// This struct is a conversion map to convert stereoscopic mode values
// from asf/wmv to the internally used matroska ones
static const struct StereoModeConversionMap WmvToInternalStereoModeMap[] =
{
  { "SideBySideRF",             "right_left" },
  { "SideBySideLF",             "left_right" },
  { "OverUnderRT",              "bottom_top" },
  { "OverUnderLT",              "top_bottom" },
  {}
};

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - AV_INPUT_BUFFER_PADDING_SIZE)

static int interrupt_cb(void* ctx)
{
  FFmpegStream* demuxer = static_cast<FFmpegStream*>(ctx);
  if (demuxer && demuxer->Aborted())
    return 1;
  return 0;
}

FFmpegStream::FFmpegStream(IManageDemuxPacket* demuxPacketManager)
  : BaseStream(demuxPacketManager),
//    m_session(nullptr),
    m_paused(false)
{
  m_pFormatContext = NULL;
  //m_pInput = NULL;
  m_ioContext = NULL;
  m_currentPts = DVD_NOPTS_VALUE;
  m_bMatroska = false;
  m_bAVI = false;
  m_bSup = false;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_program = UINT_MAX;
  m_pkt.result = -1;
  memset(&m_pkt.pkt, 0, sizeof(AVPacket));
  m_streaminfo = true; /* set to true if we want to look for streams before playback */
  m_checkvideo = false;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;

  CFFmpegLog::SetLogLevel(-100);
  av_log_set_callback(ff_avutil_log);
}

FFmpegStream::~FFmpegStream()
{
  Dispose();
  ff_flush_avutil_log_buffers();
}

bool FFmpegStream::Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty)
{
  Log(LOGLEVEL_DEBUG, "inputstream.ffmpegdirect: OpenStream()");

  m_streamUrl = streamUrl;
  m_mimeType = mimeType;
  m_isRealTimeStream = isRealTimeStream;
  m_programProperty = programProperty;

  // we never want to check stream info as we want fast switching
  bool checkStreamInfo = false;

  // if (mimeType == "video/mp2t")
  // {
  //   checkStreamInfo = false;
  // }

  m_opened = Open(checkStreamInfo);

  return m_opened;
}

void FFmpegStream::Close()
{
  // if (m_session)
  // {
  //   // RTMP_Close(m_session);
  //   // RTMP_Free(m_session);
  // }
  // m_session = nullptr;
  m_paused = false;
  m_opened = false;
}

void FFmpegStream::GetCapabilities(INPUTSTREAM_CAPABILITIES &caps)
{
  Log(LOGLEVEL_DEBUG, "GetCapabilities()");
  caps.m_mask = INPUTSTREAM_CAPABILITIES::SUPPORTS_IDEMUX |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_IDISPLAYTIME |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_ITIME |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_IPOSTIME |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_SEEK |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_PAUSE;
    INPUTSTREAM_CAPABILITIES::SUPPORTS_ICHAPTER;
}

INPUTSTREAM_IDS FFmpegStream::GetStreamIds()
{
  Log(LOGLEVEL_DEBUG, "GetStreamIds()");
  INPUTSTREAM_IDS iids;

  if(m_opened)
  {
    //int chapter = m_session->GetChapter();
    iids.m_streamCount = 0;

    for (const auto& streamPair : m_streams)
    {
      if (iids.m_streamCount < INPUTSTREAM_IDS::MAX_STREAM_COUNT)
        iids.m_streamIds[iids.m_streamCount++] = streamPair.second->uniqueId;
      else
        Log(LOGLEVEL_ERROR, "Too many streams, only %u supported", INPUTSTREAM_IDS::MAX_STREAM_COUNT);
    }
  }

  return iids;
}

INPUTSTREAM_INFO FFmpegStream::GetStream(int streamid)
{
  static struct INPUTSTREAM_INFO dummy_info = {
    INPUTSTREAM_INFO::TYPE_NONE, 0, 0, "", "", "", STREAMCODEC_PROFILE::CodecProfileUnknown, 0, 0, 0, "",
    0, 0, 0, 0, 0.0f,
    0, 0, 0, 0, 0,
    CRYPTO_INFO::CRYPTO_KEY_SYSTEM_NONE ,0 ,0 ,0};

  Log(LOGLEVEL_DEBUG, "GetStream(%d)", streamid);

  DemuxStream* stream = nullptr;
  auto streamPair = m_streams.find(streamid);
  if (streamPair != m_streams.end())
    stream = streamPair->second;

  if (stream)
  {
    INPUTSTREAM_INFO info;

    stream->GetInformation(info);

    return info;
  }
  return dummy_info;
}

void FFmpegStream::EnableStream(int streamid, bool enable)
{
}

bool FFmpegStream::OpenStream(int streamid)
{
  return true;
}

void FFmpegStream::DemuxReset()
{
  m_demuxResetOpenSuccess = false;
  Dispose();
  m_opened = false;
  m_demuxResetOpenSuccess = Open(m_streaminfo);
}

void FFmpegStream::DemuxAbort()
{
  m_timeout.SetExpired();
}

void FFmpegStream::DemuxFlush()
{
  if (m_pFormatContext)
  {
    if (m_pFormatContext->pb)
      avio_flush(m_pFormatContext->pb);
    avformat_flush(m_pFormatContext);
  }

  m_currentPts = DVD_NOPTS_VALUE;

  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
  m_seekToKeyFrame = false;
}

DemuxPacket* FFmpegStream::DemuxRead()
{
  DemuxPacket* pPacket = NULL;
  // on some cases where the received packet is invalid we will need to return an empty packet (0 length) otherwise the main loop (in CVideoPlayer)
  // would consider this the end of stream and stop.
  bool bReturnEmpty = false;
  { CSingleLock lock(m_critSection); // open lock scope
  if (m_pFormatContext)
  {
    // assume we are not eof
    if (m_pFormatContext->pb)
      m_pFormatContext->pb->eof_reached = 0;

    // check for saved packet after a program change
    if (m_pkt.result < 0)
    {
      // keep track if ffmpeg doesn't always set these
      m_pkt.pkt.size = 0;
      m_pkt.pkt.data = NULL;

      // timeout reads after 100ms
      m_timeout.Set(20000);
      m_pkt.result = av_read_frame(m_pFormatContext, &m_pkt.pkt);
      m_timeout.SetInfinite();
    }

    if (m_pkt.result == AVERROR(EINTR) || m_pkt.result == AVERROR(EAGAIN))
    {
      // timeout, probably no real error, return empty packet
      bReturnEmpty = true;
    }
    else if (m_pkt.result == AVERROR_EOF)
    {
    }
    else if (m_pkt.result < 0)
    {
      DemuxFlush();
    }
    // check size and stream index for being in a valid range
    else if (m_pkt.pkt.size < 0 ||
             m_pkt.pkt.stream_index < 0 ||
             m_pkt.pkt.stream_index >= (int)m_pFormatContext->nb_streams)
    {
      // XXX, in some cases ffmpeg returns a negative packet size
      if (m_pFormatContext->pb && !m_pFormatContext->pb->eof_reached)
      {
        Log(LOGLEVEL_ERROR, "CDVDDemuxFFmpeg::Read() no valid packet");
        bReturnEmpty = true;
        DemuxFlush();
      }
      else
        Log(LOGLEVEL_ERROR, "CDVDDemuxFFmpeg::Read() returned invalid packet and eof reached");

      m_pkt.result = -1;
      av_packet_unref(&m_pkt.pkt);
    }
    else
    {
      ParsePacket(&m_pkt.pkt);

      if (IsProgramChange())
      {
        av_dump_format(m_pFormatContext, 0, CURL::GetRedacted(m_streamUrl).c_str(), 0);

        // update streams
        CreateStreams(m_program);

        pPacket = m_demuxPacketMamnager->AllocateDemuxPacketFromInputStreamAPI(0);
        pPacket->iStreamId = DMX_SPECIALID_STREAMCHANGE;
        pPacket->demuxerId = m_demuxerId;

        return pPacket;
      }

      AVStream* stream = m_pFormatContext->streams[m_pkt.pkt.stream_index];

      if (IsVideoReady())
      {
        if (m_program != UINT_MAX)
        {
          /* check so packet belongs to selected program */
          for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
          {
            if (m_pkt.pkt.stream_index == (int)m_pFormatContext->programs[m_program]->stream_index[i])
            {
              pPacket = m_demuxPacketMamnager->AllocateDemuxPacketFromInputStreamAPI(m_pkt.pkt.size);
              break;
            }
          }

          if (!pPacket)
            bReturnEmpty = true;
        }
        else
          pPacket = m_demuxPacketMamnager->AllocateDemuxPacketFromInputStreamAPI(m_pkt.pkt.size);
      }
      else
        bReturnEmpty = true;

      if (pPacket)
      {
        if (m_bAVI && stream->codecpar && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
          // AVI's always have borked pts, specially if m_pFormatContext->flags includes
          // AVFMT_FLAG_GENPTS so always use dts
          m_pkt.pkt.pts = AV_NOPTS_VALUE;
        }

        // copy contents into our own packet
        pPacket->iSize = m_pkt.pkt.size;

        // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
        if (m_pkt.pkt.data)
          memcpy(pPacket->pData, m_pkt.pkt.data, pPacket->iSize);

        pPacket->pts = ConvertTimestamp(m_pkt.pkt.pts, stream->time_base.den, stream->time_base.num);
        pPacket->dts = ConvertTimestamp(m_pkt.pkt.dts, stream->time_base.den, stream->time_base.num);
        pPacket->duration =  DVD_SEC_TO_TIME((double)m_pkt.pkt.duration * stream->time_base.num / stream->time_base.den);

        StoreSideData(pPacket, &m_pkt.pkt);

        // TODO check this
        //CDVDInputStream::IDisplayTime* inputStream = m_pInput->GetIDisplayTime();
        // if (inputStream)
        // {
          int dispTime = GetTime();
          if (m_displayTime != dispTime)
          {
            m_displayTime = dispTime;
            if (pPacket->dts != DVD_NOPTS_VALUE)
            {
              m_dtsAtDisplayTime = pPacket->dts;
            }
          }
          if (m_dtsAtDisplayTime != DVD_NOPTS_VALUE && pPacket->dts != DVD_NOPTS_VALUE)
          {
            pPacket->dispTime = m_displayTime;
            pPacket->dispTime += DVD_TIME_TO_MSEC(pPacket->dts - m_dtsAtDisplayTime);
          }
        // }

        // used to guess streamlength
        if (pPacket->dts != DVD_NOPTS_VALUE && (pPacket->dts > m_currentPts || m_currentPts == DVD_NOPTS_VALUE))
          m_currentPts = pPacket->dts;

        // store internal id until we know the continuous id presented to player
        // the stream might not have been created yet
        pPacket->iStreamId = m_pkt.pkt.stream_index;
      }
      m_pkt.result = -1;
      av_packet_unref(&m_pkt.pkt);
    }
  }
  } // end of lock scope
  if (bReturnEmpty && !pPacket)
    pPacket = m_demuxPacketMamnager->AllocateDemuxPacketFromInputStreamAPI(0);

  if (!pPacket)
    return nullptr;

  // check streams, can we make this a bit more simple?
  if (pPacket && pPacket->iStreamId >= 0)
  {
    DemuxStream* stream = GetDemuxStream(pPacket->iStreamId);
    if (!stream ||
        stream->pPrivate != m_pFormatContext->streams[pPacket->iStreamId] ||
        stream->codec != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->codec_id)
    {
      // content has changed, or stream did not yet exist
      stream = AddStream(pPacket->iStreamId);
    }
    // we already check for a valid m_streams[pPacket->iStreamId] above
    else if (stream->type == INPUTSTREAM_INFO::STREAM_TYPE::TYPE_AUDIO)
    {
      if (static_cast<DemuxStreamAudio*>(stream)->iChannels != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->channels ||
          static_cast<DemuxStreamAudio*>(stream)->iSampleRate != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->sample_rate)
      {
        // content has changed
        stream = AddStream(pPacket->iStreamId);
      }
    }
    else if (stream->type == INPUTSTREAM_INFO::STREAM_TYPE::TYPE_VIDEO)
    {
      if (static_cast<DemuxStreamVideo*>(stream)->iWidth != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->width ||
          static_cast<DemuxStreamVideo*>(stream)->iHeight != m_pFormatContext->streams[pPacket->iStreamId]->codecpar->height)
      {
        // content has changed
        stream = AddStream(pPacket->iStreamId);
      }
      if (stream && stream->codec == AV_CODEC_ID_H264)
        pPacket->recoveryPoint = m_seekToKeyFrame;
      m_seekToKeyFrame = false;
    }
    if (!stream)
    {
      m_demuxPacketMamnager->FreeDemuxPacketFromInputStreamAPI(pPacket);
      pPacket = m_demuxPacketMamnager->AllocateDemuxPacketFromInputStreamAPI(0);
      return pPacket;
    }

    pPacket->iStreamId = stream->uniqueId;
    pPacket->demuxerId = m_demuxerId;
  }
  return pPacket;
}

bool FFmpegStream::DemuxSeekTime(double time, bool backwards, double& startpts)
{
  return SeekTime(time, backwards, &startpts);
}

void FFmpegStream::DemuxSetSpeed(int speed)
{
  if (!m_pFormatContext)
    return;

  if (m_speed == speed)
    return;

  if (m_speed != DVD_PLAYSPEED_PAUSE && speed == DVD_PLAYSPEED_PAUSE)
  {
    //m_pInput->Pause(m_currentPts);
    av_read_pause(m_pFormatContext);
  }
  else if (m_speed == DVD_PLAYSPEED_PAUSE && speed != DVD_PLAYSPEED_PAUSE)
  {
    //m_pInput->Pause(m_currentPts);
    av_read_play(m_pFormatContext);
  }
  m_speed = speed;

  AVDiscard discard = AVDISCARD_NONE;
  if (m_speed > 4 * DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_NONKEY;
  else if (m_speed > 2 * DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_BIDIR;
  else if (m_speed < DVD_PLAYSPEED_PAUSE)
    discard = AVDISCARD_NONKEY;


  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    if (m_pFormatContext->streams[i])
    {
      if (m_pFormatContext->streams[i]->discard != AVDISCARD_ALL)
        m_pFormatContext->streams[i]->discard = discard;
    }
  }
}

void FFmpegStream::SetVideoResolution(int width, int height)
{

}

int FFmpegStream::GetTotalTime()
{
  if (m_pFormatContext->duration)
    return static_cast<int>(m_pFormatContext->duration / AV_TIME_BASE * 1000);
  else
    return std::time(nullptr) - static_cast<int>(m_startTime);
}

int FFmpegStream::GetTime()
{
  return static_cast<int>(m_currentPts / DVD_TIME_BASE * 1000);
}

bool FFmpegStream::GetTimes(INPUTSTREAM_TIMES& times)
{
  return false;
}

bool FFmpegStream::PosTime(int ms)
{
  return SeekTime(static_cast<double>(ms) * 0.001f);
}

int FFmpegStream::ReadStream(uint8_t* buf, unsigned int size)
{
  return -1;
}

int64_t FFmpegStream::SeekStream(int64_t position, int whence /* SEEK_SET */)
{
  return -1;
}

int64_t FFmpegStream::PositionStream()
{
  return -1;
}

int64_t FFmpegStream::LengthStream()
{
  return -1;
}

bool FFmpegStream::IsRealTimeStream()
{
  return m_isRealTimeStream;
}

void FFmpegStream::Dispose()
{
  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  if (m_pFormatContext)
  {
    if (m_ioContext && m_pFormatContext->pb && m_pFormatContext->pb != m_ioContext)
    {
      Log(LOGLEVEL_WARNING, "CDVDDemuxFFmpeg::Dispose - demuxer changed our byte context behind our back, possible memleak");
      m_ioContext = m_pFormatContext->pb;
    }
    avformat_close_input(&m_pFormatContext);
  }

  if (m_ioContext)
  {
    av_free(m_ioContext->buffer);
    av_free(m_ioContext);
  }

  m_ioContext = NULL;
  m_pFormatContext = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  DisposeStreams();

  // TODO
  //m_pInput = NULL;
}

void FFmpegStream::DisposeStreams()
{
  // std::map<int, DemuxStream*>::iterator it;
  // for(it = m_streams.begin(); it != m_streams.end(); ++it)
  //   delete it->second;
  // m_streams.clear();
  // m_parsers.clear();
}

bool FFmpegStream::Aborted()
{
  if (m_timeout.IsTimePast())
    return true;

  // std::shared_ptr<CDVDInputStreamFFmpeg> input = std::dynamic_pointer_cast<CDVDInputStreamFFmpeg>(m_pInput);
  // if (input && input->Aborted())
  //   return true;

  return false;
}

bool FFmpegStream::Open(bool streaminfo /* true */, bool fileinfo /* false */)
{
  AVInputFormat* iformat = NULL;
  std::string strFile;
  m_streaminfo = streaminfo;
  m_currentPts = DVD_NOPTS_VALUE;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_program = UINT_MAX;
  m_seekToKeyFrame = false;

  const AVIOInterruptCB int_cb = { interrupt_cb, this };

  if (m_streamUrl.empty())
    return false;

  //m_pInput = streamUrl;
  strFile = m_streamUrl;//m_pInput->GetFileName();

  if (m_mimeType.length() > 0)
  {
    std::string content = m_mimeType;
    StringUtils::ToLower(content);

    /* check if we can get a hint from content */
    if (content.compare("video/x-vobsub") == 0)
      iformat = av_find_input_format("mpeg");
    else if (content.compare("video/x-dvd-mpeg") == 0)
      iformat = av_find_input_format("mpeg");
    else if (content.compare("video/mp2t") == 0)
      iformat = av_find_input_format("mpegts");
    else if (content.compare("multipart/x-mixed-replace") == 0)
      iformat = av_find_input_format("mjpeg");
  }

  // open the demuxer
  m_pFormatContext = avformat_alloc_context();
  m_pFormatContext->interrupt_callback = int_cb;

  // try to abort after 30 seconds
  m_timeout.Set(30000);

  if (!OpenWithAVFormat(iformat, int_cb))
    return false;

  // Avoid detecting framerate if advancedsettings.xml says so
  // if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoFpsDetect == 0)
  //     m_pFormatContext->fps_probe_size = 0;

  // analyse very short to speed up mjpeg playback start
  // if (iformat && (strcmp(iformat->name, "mjpeg") == 0) && m_ioContext->seekable == 0)
  //   av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);

  bool skipCreateStreams = false;
  bool isBluray = false;//pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY);
  if (iformat && (strcmp(iformat->name, "mpegts") == 0) && !fileinfo && !isBluray)
  {
    av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);
    m_checkvideo = true;
    skipCreateStreams = true;
  }
  else if (!iformat || (strcmp(iformat->name, "mpegts") != 0))
  {
    m_streaminfo = true;
  }

  if (iformat && (strcmp(iformat->name, "mov,mp4,m4a,3gp,3g2,mj2") == 0))
  {
    CURL url(m_streamUrl);
    //if (URIUtils::IsRemote(strFile))
    if (!url.GetProtocol().empty() && !url.IsProtocol("file"))
      m_pFormatContext->iformat->flags |= AVFMT_NOGENSEARCH;
  }

  // we need to know if this is matroska, avi or sup later
  m_bMatroska = strncmp(m_pFormatContext->iformat->name, "matroska", 8) == 0;	// for "matroska.webm"
  m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;
  m_bSup = strcmp(m_pFormatContext->iformat->name, "sup") == 0;

  if (m_streaminfo)
  {
    /* to speed up dvd switches, only analyse very short */
    // if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    //   av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);

    Log(LOGLEVEL_DEBUG, "%s - avformat_find_stream_info starting", __FUNCTION__);
    int iErr = avformat_find_stream_info(m_pFormatContext, NULL);
    if (iErr < 0)
    {
      Log(LOGLEVEL_WARNING,"could not find codec parameters for %s", CURL::GetRedacted(strFile).c_str());
      if ((m_pFormatContext->nb_streams == 1 &&
           m_pFormatContext->streams[0]->codecpar->codec_id == AV_CODEC_ID_AC3) ||
          m_checkvideo)
      {
        // special case, our codecs can still handle it.
      }
      else
      {
        Dispose();
        return false;
      }
    }
    Log(LOGLEVEL_DEBUG, "%s - av_find_stream_info finished", __FUNCTION__);

    // print some extra information
    av_dump_format(m_pFormatContext, 0, CURL::GetRedacted(strFile).c_str(), 0);

    if (m_checkvideo)
    {
      // make sure we start video with an i-frame
      ResetVideoStreams();
    }
  }
  else
  {
    m_program = 0;
    m_checkvideo = true;
    skipCreateStreams = true;
  }

  // reset any timeout
  m_timeout.SetInfinite();

  // if format can be nonblocking, let's use that
  m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

  // deprecated, will be always set in future versions
  m_pFormatContext->flags |= AVFMT_FLAG_KEEP_SIDE_DATA;

  UpdateCurrentPTS();

  // select the correct program if requested
  m_initialProgramNumber = UINT_MAX;
  CVariant programProp(m_programProperty);
  if (!programProp.isNull())
    m_initialProgramNumber = static_cast<int>(programProp.asInteger());

  // in case of mpegts and we have not seen pat/pmt, defer creation of streams
  if (!skipCreateStreams || m_pFormatContext->nb_programs > 0)
  {
    unsigned int nProgram = UINT_MAX;
    if (m_pFormatContext->nb_programs > 0)
    {
      // select the correct program if requested
      if (m_initialProgramNumber != UINT_MAX)
      {
        for (unsigned int i = 0; i < m_pFormatContext->nb_programs; ++i)
        {
          if (m_pFormatContext->programs[i]->program_num == static_cast<int>(m_initialProgramNumber))
          {
            nProgram = i;
            m_initialProgramNumber = UINT_MAX;
            break;
          }
        }
      }
      else if (m_pFormatContext->iformat && strcmp(m_pFormatContext->iformat->name, "hls,applehttp") == 0)
      {
        nProgram = HLSSelectProgram();
      }
      else
      {
        // skip programs without or empty audio/video streams
        for (unsigned int i = 0; nProgram == UINT_MAX && i < m_pFormatContext->nb_programs; i++)
        {
          for (unsigned int j = 0; j < m_pFormatContext->programs[i]->nb_stream_indexes; j++)
          {
            int idx = m_pFormatContext->programs[i]->stream_index[j];
            AVStream* st = m_pFormatContext->streams[idx];
            if ((st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && st->codec_info_nb_frames > 0) ||
                (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && st->codecpar->sample_rate > 0))
            {
              nProgram = i;
              break;
            }
          }
        }
      }
    }
    CreateStreams(nProgram);
  }

  m_newProgram = m_program;

  // allow IsProgramChange to return true
  if (skipCreateStreams && GetNrOfStreams() == 0)
    m_program = 0;

  m_displayTime = 0;
  m_dtsAtDisplayTime = DVD_NOPTS_VALUE;
  m_startTime = 0;
  m_seekStream = -1;

  if (m_checkvideo && m_streaminfo)
  {
    int64_t duration = m_pFormatContext->duration;
    Dispose();
    if (!Open(false))
      return false;
    m_pFormatContext->duration = duration;
  }

  // seems to be a bug in ffmpeg, hls jumps back to start after a couple of seconds
  // this cures the issue
  if (m_pFormatContext->iformat && strcmp(m_pFormatContext->iformat->name, "hls,applehttp") == 0)
  {
    SeekTime(0);
  }

  return true;
}

bool FFmpegStream::OpenWithAVFormat(AVInputFormat* iformat, const AVIOInterruptCB& int_cb)
{
  // special stream type that makes avformat handle file opening
  // allows internal ffmpeg protocols to be used
  AVDictionary* options = GetFFMpegOptionsFromInput();

  CURL url;
  url.Parse(m_streamUrl);
  url.SetProtocolOptions("");
  std::string strFile = url.Get();

  int result = -1;
  if (url.IsProtocol("mms"))
  {
    // try mmsh, then mmst
    url.SetProtocol("mmsh");
    url.SetProtocolOptions("");
    result = avformat_open_input(&m_pFormatContext, url.Get().c_str(), iformat, &options);
    if (result < 0)
    {
      url.SetProtocol("mmst");
      strFile = url.Get();
    }
  }
  else if (url.IsProtocol("udp") || url.IsProtocol("rtp"))
  {
    std::string strURL = url.Get();
    Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::Open() UDP/RTP Original URL '%s'", strURL.c_str());
    size_t found = strURL.find("://");
    if (found != std::string::npos)
    {
      size_t start = found + 3;
      found = strURL.find("@");

      if (found != std::string::npos && found > start)
      {
        // sourceip found
        std::string strSourceIp = strURL.substr(start, found - start);

        strFile = strURL.substr(0, start);
        strFile += strURL.substr(found);
        if(strFile.back() == '/')
          strFile.pop_back();
        strFile += "?sources=";
        strFile += strSourceIp;
        Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::Open() UDP/RTP URL '%s'", strFile.c_str());
      }
    }
  }
  if (result < 0)
  {
    m_pFormatContext->flags |= AVFMT_FLAG_PRIV_OPT;
    if (avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0)
    {
      Log(LOGLEVEL_DEBUG, "Error, could not open file %s", CURL::GetRedacted(strFile).c_str());
      Dispose();
      av_dict_free(&options);
      return false;
    }

    av_dict_free(&options);
    avformat_close_input(&m_pFormatContext);
    m_pFormatContext = avformat_alloc_context();
    m_pFormatContext->interrupt_callback = int_cb;
    m_pFormatContext->flags &= ~AVFMT_FLAG_PRIV_OPT;
    options = GetFFMpegOptionsFromInput();
    av_dict_set_int(&options, "load_all_variants", 0, AV_OPT_SEARCH_CHILDREN);

    if (avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0)
    {
      Log(LOGLEVEL_DEBUG, "Error, could not open file (2) %s", CURL::GetRedacted(strFile).c_str());
      Dispose();
      av_dict_free(&options);
      return false;
    }
  }

  av_dict_free(&options);

  return true;
}

void FFmpegStream::ResetVideoStreams()
{
  AVStream* st;
  for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    st = m_pFormatContext->streams[i];
    if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      av_freep(&st->codecpar->extradata);
      st->codecpar->extradata_size = 0;
    }
  }
}

void FFmpegStream::UpdateCurrentPTS()
{
  m_currentPts = DVD_NOPTS_VALUE;

  int idx = av_find_default_stream_index(m_pFormatContext);
  if (idx >= 0)
  {
    AVStream* stream = m_pFormatContext->streams[idx];
    if (stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      m_currentPts = ts;
    }
  }
}

double FFmpegStream::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completely exact timestamp anyway
  double timestamp = (double)pts * num / den;
  double starttime = 0.0f;

  //std::shared_ptr<CDVDInputStream::IMenus> menu = std::dynamic_pointer_cast<CDVDInputStream::IMenus>(m_pInput);
  //if (!menu && m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if (m_checkvideo)
    starttime = m_startTime;

  if (!m_bSup)
  {
    if (timestamp > starttime || m_checkvideo)
      timestamp -= starttime;
    // allow for largest possible difference in pts and dts for a single packet
    else if (timestamp + 0.5f > starttime)
      timestamp = 0;
  }

  return timestamp * DVD_TIME_BASE;
}

bool FFmpegStream::IsProgramChange()
{
  if (m_program == UINT_MAX)
    return false;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return false;

  if (m_initialProgramNumber != UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; ++i)
    {
      if (m_pFormatContext->programs[i]->program_num == static_cast<int>(m_initialProgramNumber))
      {
        m_newProgram = i;
        m_initialProgramNumber = UINT_MAX;
        break;
      }
    }
    if (m_initialProgramNumber != UINT_MAX)
      return false;
  }

  if (m_program != m_newProgram)
  {
    m_program = m_newProgram;
    return true;
  }

  if (m_pFormatContext->programs[m_program]->nb_stream_indexes != m_streamsInProgram)
    return true;

  if (m_program >= m_pFormatContext->nb_programs)
    return true;

  for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
  {
    int idx = m_pFormatContext->programs[m_program]->stream_index[i];
    if (m_pFormatContext->streams[idx]->discard >= AVDISCARD_ALL)
      continue;
    DemuxStream* stream = GetDemuxStream(idx);
    if (!stream)
      return true;
    if (m_pFormatContext->streams[idx]->codecpar->codec_id != stream->codec)
      return true;
    if (m_pFormatContext->streams[idx]->codecpar->extradata_size != static_cast<int>(stream->ExtraSize))
      return true;
  }
  return false;
}

unsigned int FFmpegStream::HLSSelectProgram()
{
  unsigned int prog = UINT_MAX;

  int bandwidth = 0;//CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_NETWORK_BANDWIDTH) * 1000;
  if (bandwidth <= 0)
    bandwidth = INT_MAX;

  int selectedBitrate = 0;
  int selectedRes = 0;
  for (unsigned int i = 0; i < m_pFormatContext->nb_programs; ++i)
  {
    int strBitrate = 0;
    AVDictionaryEntry* tag = av_dict_get(m_pFormatContext->programs[i]->metadata, "variant_bitrate", NULL, 0);
    if (tag)
      strBitrate = atoi(tag->value);
    else
      continue;

    int strRes = 0;
    for (unsigned int j = 0; j < m_pFormatContext->programs[i]->nb_stream_indexes; j++)
    {
      int idx = m_pFormatContext->programs[i]->stream_index[j];
      AVStream* pStream = m_pFormatContext->streams[idx];
      if (pStream && pStream->codecpar &&
          pStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        strRes = pStream->codecpar->width * pStream->codecpar->height;
      }
    }

    if ((strRes && strRes < selectedRes) && selectedBitrate < bandwidth)
      continue;

    bool want = false;

    if (strBitrate <= bandwidth)
    {
      if (strBitrate > selectedBitrate || strRes > selectedRes)
        want = true;
    }
    else
    {
      if (strBitrate < selectedBitrate)
        want = true;
    }

    if (want)
    {
      selectedRes = strRes;
      selectedBitrate = strBitrate;
      prog = i;
    }
  }
  return prog;
}

/**
 * @brief Finds stream based on unique id
 */
DemuxStream* FFmpegStream::GetDemuxStream(int iStreamId) const
{
  auto it = m_streams.find(iStreamId);
  if (it != m_streams.end())
    return it->second;

  return nullptr;
}

std::vector<DemuxStream*> FFmpegStream::GetDemuxStreams() const
{
  std::vector<DemuxStream*> streams;

  for (auto& iter : m_streams)
    streams.push_back(iter.second);

  return streams;
}

int FFmpegStream::GetNrOfStreams() const
{
  return m_streams.size();
}

int FFmpegStream::GetNrOfStreams(INPUTSTREAM_INFO::STREAM_TYPE streamType)
{
  int iCounter = 0;

  for (auto pStream : GetDemuxStreams())
  {
    if (pStream && pStream->type == streamType) iCounter++;
  }

  return iCounter;
}

int FFmpegStream::GetNrOfSubtitleStreams()
{
  return GetNrOfStreams(INPUTSTREAM_INFO::STREAM_TYPE::TYPE_SUBTITLE);
}

double FFmpegStream::SelectAspect(AVStream* st, bool& forced)
{
  // trust matroska container
  if (m_bMatroska && st->sample_aspect_ratio.num != 0)
  {
    forced = true;
    double dar = av_q2d(st->sample_aspect_ratio);
    // for stereo modes, use codec aspect ratio
    AVDictionaryEntry* entry = av_dict_get(st->metadata, "stereo_mode", NULL, 0);
    if (entry)
    {
      if (strcmp(entry->value, "left_right") == 0 || strcmp(entry->value, "right_left") == 0)
        dar /= 2;
      else if (strcmp(entry->value, "top_bottom") == 0 || strcmp(entry->value, "bottom_top") == 0)
        dar *= 2;
    }
    return dar;
  }

  /* if stream aspect is 1:1 or 0:0 use codec aspect */
  if ((st->sample_aspect_ratio.den == 1 || st->sample_aspect_ratio.den == 0) &&
     (st->sample_aspect_ratio.num == 1 || st->sample_aspect_ratio.num == 0) &&
      st->codecpar->sample_aspect_ratio.num != 0)
  {
    forced = false;
    return av_q2d(st->codecpar->sample_aspect_ratio);
  }

  if (st->sample_aspect_ratio.num != 0)
  {
    forced = true;
    return av_q2d(st->sample_aspect_ratio);
  }

  forced = false;
  return 0.0;
}

std::string FFmpegStream::GetStereoModeFromMetadata(AVDictionary* pMetadata)
{
  std::string stereoMode;
  AVDictionaryEntry* tag = NULL;

  // matroska
  tag = av_dict_get(pMetadata, "stereo_mode", NULL, 0);
  if (tag && tag->value)
    stereoMode = tag->value;

  // asf / wmv
  if (stereoMode.empty())
  {
    tag = av_dict_get(pMetadata, "Stereoscopic", NULL, 0);
    if (tag && tag->value)
    {
      tag = av_dict_get(pMetadata, "StereoscopicLayout", NULL, 0);
      if (tag && tag->value)
        stereoMode = ConvertCodecToInternalStereoMode(tag->value, WmvToInternalStereoModeMap);
    }
  }

  return stereoMode;
}

std::string FFmpegStream::ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap* conversionMap)
{
  size_t i = 0;
  while (conversionMap[i].name)
  {
    if (mode == conversionMap[i].name)
      return conversionMap[i].mode;
    i++;
  }
  return "";
}

void FFmpegStream::StoreSideData(DemuxPacket *pkt, AVPacket *src)
{
  AVPacket avPkt;
  av_init_packet(&avPkt);
  av_packet_copy_props(&avPkt, src);
  pkt->pSideData = avPkt.side_data;
  pkt->iSideDataElems = avPkt.side_data_elems;
}

bool FFmpegStream::SeekTime(double time, bool backwards, double* startpts)
{
  bool hitEnd = false;

  if (!StreamsOpened())
    return false;

  if (time < 0)
  {
    time = 0;
    hitEnd = true;
  }

  m_pkt.result = -1;
  av_packet_unref(&m_pkt.pkt);

  int64_t seek_pts = (int64_t)time * (AV_TIME_BASE / 1000);
  bool ismp3 = m_pFormatContext->iformat && (strcmp(m_pFormatContext->iformat->name, "mp3") == 0);

  if (m_checkvideo)
  {
    XbmcThreads::EndTime timer(1000);

    while (!IsVideoReady())
    {
      DemuxPacket* pkt = DemuxRead();
      if (pkt)
        m_demuxPacketMamnager->FreeDemuxPacketFromInputStreamAPI(pkt);
      else
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      m_pkt.result = -1;
      av_packet_unref(&m_pkt.pkt);

      if (timer.IsTimePast())
      {
        Log(LOGLEVEL_ERROR, "CDVDDemuxFFmpeg::%s - Timed out waiting for video to be ready", __FUNCTION__);
        return false;
      }
    }

    AVStream* st = m_pFormatContext->streams[m_seekStream];
    seek_pts = av_rescale(m_startTime + time / 1000, st->time_base.den, st->time_base.num);
  }
  else if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE && !ismp3 && !m_bSup)
    seek_pts += m_pFormatContext->start_time;

  int ret;
  {
    CSingleLock lock(m_critSection);
    ret = av_seek_frame(m_pFormatContext, m_seekStream, seek_pts, backwards ? AVSEEK_FLAG_BACKWARD : 0);

    if (ret < 0)
    {
      int64_t starttime = m_pFormatContext->start_time;
      if (m_checkvideo)
      {
        AVStream* st = m_pFormatContext->streams[m_seekStream];
        starttime = av_rescale(m_startTime, st->time_base.num, st->time_base.den);
      }

      // demuxer can return failure, if seeking behind eof
      if (m_pFormatContext->duration &&
          seek_pts >= (m_pFormatContext->duration + starttime))
      {
        // force eof
        // files of realtime streams may grow
        if (!IsRealTimeStream())
          Close();
        else
          ret = 0;
      }
      else if (Aborted()) // TODO Was pInput->IsEOF();
        ret = 0;
    }

    if (ret >= 0)
    {
      if (m_pFormatContext->iformat->read_seek)
        m_seekToKeyFrame = true;

      UpdateCurrentPTS();
    }
  }

  if (m_currentPts == DVD_NOPTS_VALUE)
    Log(LOGLEVEL_DEBUG, "%s - unknown position after seek", __FUNCTION__);
  else
    Log(LOGLEVEL_DEBUG, "%s - seek ended up on time %d", __FUNCTION__, (int)(m_currentPts / DVD_TIME_BASE * 1000));

  // in this case the start time is requested time
  if (startpts)
    *startpts = DVD_MSEC_TO_TIME(time);

  if (ret >= 0)
  {
    if (!hitEnd)
      return true;
    else
      return false;
  }
  else
    return false;
}

void FFmpegStream::ParsePacket(AVPacket* pkt)
{
  AVStream* st = m_pFormatContext->streams[pkt->stream_index];

  if (st && st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
  {
    auto parser = m_parsers.find(st->index);
    if (parser == m_parsers.end())
    {
      m_parsers.insert(std::make_pair(st->index,
                                      std::unique_ptr<DemuxParserFFmpeg>(new DemuxParserFFmpeg())));
      parser = m_parsers.find(st->index);

      parser->second->m_parserCtx = av_parser_init(st->codecpar->codec_id);

      AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
      if (codec == nullptr)
      {
        Log(LOGLEVEL_ERROR, "%s - can't find decoder", __FUNCTION__);
        m_parsers.erase(parser);
        return;
      }
      parser->second->m_codecCtx = avcodec_alloc_context3(codec);
    }

    DemuxStream* stream = GetDemuxStream(st->index);
    if (!stream)
      return;

    if (parser->second->m_parserCtx &&
        parser->second->m_parserCtx->parser &&
        parser->second->m_parserCtx->parser->split &&
        !st->codecpar->extradata)
    {
      int i = parser->second->m_parserCtx->parser->split(parser->second->m_codecCtx, pkt->data, pkt->size);
      if (i > 0 && i < FF_MAX_EXTRADATA_SIZE)
      {
        st->codecpar->extradata = (uint8_t*)av_malloc(i + AV_INPUT_BUFFER_PADDING_SIZE);
        if (st->codecpar->extradata)
        {
          Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::ParsePacket() fetching extradata, extradata_size(%d)", i);
          st->codecpar->extradata_size = i;
          memcpy(st->codecpar->extradata, pkt->data, i);
          memset(st->codecpar->extradata + i, 0, AV_INPUT_BUFFER_PADDING_SIZE);

          if (parser->second->m_parserCtx->parser->parser_parse)
          {
            parser->second->m_codecCtx->extradata = st->codecpar->extradata;
            parser->second->m_codecCtx->extradata_size = st->codecpar->extradata_size;
            const uint8_t* outbufptr;
            int bufSize;
            parser->second->m_parserCtx->flags |= PARSER_FLAG_COMPLETE_FRAMES;
            parser->second->m_parserCtx->parser->parser_parse(parser->second->m_parserCtx,
                                                              parser->second->m_codecCtx,
                                                              &outbufptr, &bufSize,
                                                              pkt->data, pkt->size);
            parser->second->m_codecCtx->extradata = nullptr;
            parser->second->m_codecCtx->extradata_size = 0;

            if (parser->second->m_parserCtx->width != 0)
            {
              st->codecpar->width = parser->second->m_parserCtx->width;
              st->codecpar->height = parser->second->m_parserCtx->height;
            }
            else
            {
              Log(LOGLEVEL_ERROR, "CDVDDemuxFFmpeg::ParsePacket() invalid width/height");
            }
          }
        }
      }
    }
  }
}

bool FFmpegStream::IsVideoReady()
{
  AVStream* st;
  bool hasVideo = false;

  if (!m_checkvideo)
    return true;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return false;

  if (m_program != UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
    {
      int idx = m_pFormatContext->programs[m_program]->stream_index[i];
      st = m_pFormatContext->streams[idx];
      if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if (st->codecpar->extradata)
        {
          if (!m_startTime)
          {
            m_startTime = av_rescale(st->cur_dts, st->time_base.num, st->time_base.den) - 0.000001;
            m_seekStream = i;
          }
          return true;
        }
        hasVideo = true;
      }
    }
    // Workaround for live audio-only MPEG-TS streams: If there are no elementary video streams
    // present attempt to set the start time from the first available elementary audio stream instead
    if (!hasVideo && !m_startTime)
    {
      for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
      {
        int idx = m_pFormatContext->programs[m_program]->stream_index[i];
        st = m_pFormatContext->streams[idx];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
          m_startTime = av_rescale(st->cur_dts, st->time_base.num, st->time_base.den) - 0.000001;
          m_seekStream = i;
          break;
        }
      }
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      st = m_pFormatContext->streams[i];
      if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if (st->codecpar->extradata)
        {
          if (!m_startTime)
          {
            m_startTime = av_rescale(st->cur_dts, st->time_base.num, st->time_base.den) - 0.000001;
            m_seekStream = i;
          }
          return true;
        }
        hasVideo = true;
      }
    }
    // Workaround for live audio-only MPEG-TS streams: If there are no elementary video streams
    // present attempt to set the start time from the first available elementary audio stream instead
    if (!hasVideo && !m_startTime)
    {
      for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      {
        st = m_pFormatContext->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
          m_startTime = av_rescale(st->cur_dts, st->time_base.num, st->time_base.den) - 0.000001;
          m_seekStream = i;
          break;
        }
      }
    }
  }
  return !hasVideo;
}

void FFmpegStream::CreateStreams(unsigned int program)
{
  DisposeStreams();

  // add the ffmpeg streams to our own stream map
  if (m_pFormatContext->nb_programs)
  {
    // check if desired program is available
    if (program < m_pFormatContext->nb_programs)
    {
      m_program = program;
      m_streamsInProgram = m_pFormatContext->programs[program]->nb_stream_indexes;
      m_pFormatContext->programs[program]->discard = AVDISCARD_NONE;
    }
    else
      m_program = UINT_MAX;

    // look for first non empty stream and discard nonselected programs
    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      if (m_program == UINT_MAX && m_pFormatContext->programs[i]->nb_stream_indexes > 0)
      {
        m_program = i;
      }

      if (i != m_program)
        m_pFormatContext->programs[i]->discard = AVDISCARD_ALL;
    }
    if (m_program != UINT_MAX)
    {
      m_pFormatContext->programs[m_program]->discard = AVDISCARD_NONE;

      // add streams from selected program
      for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
      {
        int streamIdx = m_pFormatContext->programs[m_program]->stream_index[i];
        m_pFormatContext->streams[streamIdx]->discard = AVDISCARD_NONE;
        AddStream(streamIdx);
      }

      // discard all unneeded streams
      for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      {
        m_pFormatContext->streams[i]->discard = AVDISCARD_NONE;
        if (GetDemuxStream(i) == nullptr)
          m_pFormatContext->streams[i]->discard = AVDISCARD_ALL;
      }
    }
  }
  else
    m_program = UINT_MAX;

  // if there were no programs or they were all empty, add all streams
  if (m_program == UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      AddStream(i);
  }
}

DemuxStream* FFmpegStream::AddStream(int streamIdx)
{
  AVStream* pStream = m_pFormatContext->streams[streamIdx];
  if (pStream && pStream->discard != AVDISCARD_ALL)
  {
    // Video (mp4) from GoPro cameras can have a 'meta' track used for a file repair containing
    // 'fdsc' data, this is also called the SOS track.
    if (pStream->codecpar->codec_tag == MKTAG('f','d','s','c'))
    {
      Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::AddStream - discarding fdsc stream");
      pStream->discard = AVDISCARD_ALL;
      return nullptr;
    }

    DemuxStream* stream = nullptr;

    switch (pStream->codecpar->codec_type)
    {
      case AVMEDIA_TYPE_AUDIO:
      {
        DemuxStreamAudioFFmpeg* st = new DemuxStreamAudioFFmpeg(pStream);
        stream = st;
        st->iChannels = pStream->codecpar->channels;
        st->iSampleRate = pStream->codecpar->sample_rate;
        st->iBlockAlign = pStream->codecpar->block_align;
        st->iBitRate = static_cast<int>(pStream->codecpar->bit_rate);
        st->iBitsPerSample = pStream->codecpar->bits_per_raw_sample;
        st->iChannelLayout = pStream->codecpar->channel_layout;
        char buf[32] = { 0 };
        av_get_channel_layout_string(buf, 31, st->iChannels, st->iChannelLayout);
        st->m_channelLayoutName = buf;
        if (st->iBitsPerSample == 0)
          st->iBitsPerSample = pStream->codecpar->bits_per_coded_sample;

        if (av_dict_get(pStream->metadata, "title", NULL, 0))
          st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
      case AVMEDIA_TYPE_VIDEO:
      {
        DemuxStreamVideoFFmpeg* st = new DemuxStreamVideoFFmpeg(pStream);
        stream = st;
        if (strcmp(m_pFormatContext->iformat->name, "flv") == 0)
          st->bVFR = true;
        else
          st->bVFR = false;

        // never trust pts in avi files with h264.
        if (m_bAVI && pStream->codecpar->codec_id == AV_CODEC_ID_H264)
          st->bPTSInvalid = true;

        AVRational r_frame_rate = pStream->r_frame_rate;

        //average fps is more accurate for mkv files
        if (m_bMatroska && pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
        {
          st->iFpsRate = pStream->avg_frame_rate.num;
          st->iFpsScale = pStream->avg_frame_rate.den;
        }
        else if (r_frame_rate.den && r_frame_rate.num)
        {
          st->iFpsRate = r_frame_rate.num;
          st->iFpsScale = r_frame_rate.den;
        }
        else
        {
          st->iFpsRate  = 0;
          st->iFpsScale = 0;
        }

        // if (pStream->codec_info_nb_frames > 0 &&
        //     pStream->codec_info_nb_frames <= 2 &&
        //     m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
        // {
        //   Log(LOGLEVEL_DEBUG, "%s - fps may be unreliable since ffmpeg decoded only %d frame(s)", __FUNCTION__, pStream->codec_info_nb_frames);
        //   st->iFpsRate  = 0;
        //   st->iFpsScale = 0;
        // }

        st->iWidth = pStream->codecpar->width;
        st->iHeight = pStream->codecpar->height;
        st->fAspect = SelectAspect(pStream, st->bForcedAspect);
        if (pStream->codecpar->height)
          st->fAspect *= (double)pStream->codecpar->width / pStream->codecpar->height;
        st->iOrientation = 0;
        st->iBitsPerPixel = pStream->codecpar->bits_per_coded_sample;
        st->iBitRate = static_cast<int>(pStream->codecpar->bit_rate);

        AVDictionaryEntry* rtag = av_dict_get(pStream->metadata, "rotate", NULL, 0);
        if (rtag)
          st->iOrientation = atoi(rtag->value);

        // detect stereoscopic mode
        std::string stereoMode = GetStereoModeFromMetadata(pStream->metadata);
          // check for metadata in file if detection in stream failed
        if (stereoMode.empty())
          stereoMode = GetStereoModeFromMetadata(m_pFormatContext->metadata);
        if (!stereoMode.empty())
          st->stereo_mode = stereoMode;


        // if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
        // {
        //   if (pStream->codecpar->codec_id == AV_CODEC_ID_PROBE)
        //   {
        //     // fix MPEG-1/MPEG-2 video stream probe returning AV_CODEC_ID_PROBE for still frames.
        //     // ffmpeg issue 1871, regression from ffmpeg r22831.
        //     if ((pStream->id & 0xF0) == 0xE0)
        //     {
        //       pStream->codecpar->codec_id = AV_CODEC_ID_MPEG2VIDEO;
        //       pStream->codecpar->codec_tag = MKTAG('M','P','2','V');
        //       CLog::Log(LOGLEVEL_ERROR, "%s - AV_CODEC_ID_PROBE detected, forcing AV_CODEC_ID_MPEG2VIDEO", __FUNCTION__);
        //     }
        //   }
        // }
        if (av_dict_get(pStream->metadata, "title", NULL, 0))
          st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
      // case AVMEDIA_TYPE_DATA:
      // {
      //   stream = new DemuxStream();
      //   stream->type = STREAM_DATA;
      //   break;
      // }
      case AVMEDIA_TYPE_SUBTITLE:
      {
        if (pStream->codecpar->codec_id == AV_CODEC_ID_DVB_TELETEXT && true)//CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOPLAYER_TELETEXTENABLED))
        {
          DemuxStreamTeletext* st = new DemuxStreamTeletext();
          stream = st;
          break;
        }
        else
        {
          DemuxStreamSubtitleFFmpeg* st = new DemuxStreamSubtitleFFmpeg(pStream);
          stream = st;

          if (av_dict_get(pStream->metadata, "title", NULL, 0))
            st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

          break;
        }
      }
      // case AVMEDIA_TYPE_ATTACHMENT:
      // { //mkv attachments. Only bothering with fonts for now.
        // if (pStream->codecpar->codec_id == AV_CODEC_ID_TTF ||
        //     pStream->codecpar->codec_id == AV_CODEC_ID_OTF)
        // {
        //   std::string fileName = "special://temp/fonts/";
        //   XFILE::CDirectory::Create(fileName);
        //   AVDictionaryEntry* nameTag = av_dict_get(pStream->metadata, "filename", NULL, 0);
        //   if (!nameTag)
        //   {
        //     Log(LOGLEVEL_ERROR, "%s: TTF attachment has no name", __FUNCTION__);
        //   }
        //   else
        //   {
        //     fileName += nameTag->value;
        //     XFILE::CFile file;
        //     if (pStream->codecpar->extradata && file.OpenForWrite(fileName))
        //     {
        //       if (file.Write(pStream->codecpar->extradata, pStream->codecpar->extradata_size) !=
        //           pStream->codecpar->extradata_size)
        //       {
        //         file.Close();
        //         XFILE::CFile::Delete(fileName);
        //         Log(LOGLEVEL_DEBUG, "%s: Error saving font file \"%s\"", __FUNCTION__, fileName.c_str());
        //       }
        //     }
        //   }
        // }
        // stream = new DemuxStream();
        // stream->type = STREAM_NONE;
      //   break;
      // }
      default:
      {
        // if analyzing streams is skipped, unknown streams may become valid later
        if (m_streaminfo && IsVideoReady())
        {
          Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::AddStream - discarding unknown stream with id: %d", pStream->index);
          pStream->discard = AVDISCARD_ALL;
          return nullptr;
        }
        stream = new DemuxStream();
        stream->type = INPUTSTREAM_INFO::STREAM_TYPE::TYPE_NONE;
      }
    }

    // generic stuff
    if (pStream->duration != (int64_t)AV_NOPTS_VALUE)
      stream->iDuration = (int)((pStream->duration / AV_TIME_BASE) & 0xFFFFFFFF);

    stream->codec = pStream->codecpar->codec_id;
    stream->codec_fourcc = pStream->codecpar->codec_tag;
    stream->profile = pStream->codecpar->profile;
    stream->level = pStream->codecpar->level;

    //stream->source = STREAM_SOURCE_DEMUX;
    stream->pPrivate = pStream;
    stream->flags = (INPUTSTREAM_INFO::STREAM_FLAGS)pStream->disposition;

    AVDictionaryEntry* langTag = av_dict_get(pStream->metadata, "language", NULL, 0);
    if (!langTag)
    {
      // only for avi audio streams
      if ((strcmp(m_pFormatContext->iformat->name, "avi") == 0) && (pStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO))
      {
        // only defined for streams 1 to 9
        if ((streamIdx > 0) && (streamIdx < 10))
        {
          // search for language information in RIFF-Header ("IAS1": first language - "IAS9": ninth language)
          char riff_tag_string[5] = {'I', 'A', 'S', (char)(streamIdx + '0'), '\0'};
          langTag = av_dict_get(m_pFormatContext->metadata, riff_tag_string, NULL, 0);
          if (!langTag && (streamIdx == 1))
          {
            // search for language information in RIFF-Header ("ILNG": language)
            langTag = av_dict_get(m_pFormatContext->metadata, "language", NULL, 0);
          }
        }
      }
    }
    if (langTag)
      stream->language = std::string(langTag->value, 3);

    if (stream->type != INPUTSTREAM_INFO::STREAM_TYPE::TYPE_NONE && pStream->codecpar->extradata && pStream->codecpar->extradata_size > 0)
    {
      stream->ExtraSize = pStream->codecpar->extradata_size;
      stream->ExtraData = new uint8_t[pStream->codecpar->extradata_size];
      memcpy(stream->ExtraData, pStream->codecpar->extradata, pStream->codecpar->extradata_size);
    }

// #ifdef HAVE_LIBBLURAY
//     if (m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY))
//     {
//       stream->dvdNavId = pStream->id;

//       auto it = std::find_if(m_streams.begin(), m_streams.end(),
//         [&stream](const std::pair<int, DemuxStream*>& v)
//         {return (v.second->dvdNavId == stream->dvdNavId) && (v.second->type == stream->type); });

//       if (it != m_streams.end())
//       {
//         if (stream->codec == AV_CODEC_ID_AC3 && it->second->codec == AV_CODEC_ID_TRUEHD)
//           CLog::Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::AddStream - discarding duplicated bluray stream (truehd ac3 core)");
//         else
//           CLog::Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::AddStream - discarding duplicate bluray stream %s", stream->codecName);

//         pStream->discard = AVDISCARD_ALL;
//         delete stream;
//         return nullptr;
//       }
//       std::static_pointer_cast<CDVDInputStreamBluray>(m_pInput)->GetStreamInfo(pStream->id, stream->language);
//     }
// #endif
//     if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
//     {
//       // this stuff is really only valid for dvd's.
//       // this is so that the physicalid matches the
//       // id's reported from libdvdnav
//       switch (stream->codec)
//       {
//         case AV_CODEC_ID_AC3:
//           stream->dvdNavId = pStream->id - 128;
//           break;
//         case AV_CODEC_ID_DTS:
//           stream->dvdNavId = pStream->id - 136;
//           break;
//         case AV_CODEC_ID_MP2:
//           stream->dvdNavId = pStream->id - 448;
//           break;
//         case AV_CODEC_ID_PCM_S16BE:
//           stream->dvdNavId = pStream->id - 160;
//           break;
//         case AV_CODEC_ID_DVD_SUBTITLE:
//           stream->dvdNavId = pStream->id - 0x20;
//           break;
//         default:
//           stream->dvdNavId = pStream->id & 0x1f;
//           break;
//       }
//     }

    stream->uniqueId = pStream->index;
    stream->demuxerId = m_demuxerId;

    AddStream(stream->uniqueId, stream);
    return stream;
  }
  else
    return nullptr;
}

/**
 * @brief Adds or updates a demux stream based in ffmpeg id
 */
void FFmpegStream::AddStream(int streamIdx, DemuxStream* stream)
{
  std::pair<std::map<int, DemuxStream*>::iterator, bool> res;

  res = m_streams.insert(std::make_pair(streamIdx, stream));
  if (res.second)
  {
    /* was new stream */
    stream->uniqueId = streamIdx;
  }
  else
  {
    delete res.first->second;
    res.first->second = stream;
  }

  stream->codecName = GetStreamCodecName(stream->uniqueId);
  Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::AddStream ID: %d", streamIdx);
}

std::string FFmpegStream::GetStreamCodecName(int iStreamId)
{
  DemuxStream* stream = GetDemuxStream(iStreamId);
  std::string strName;
  if (stream)
  {
    /* use profile to determine the DTS type */
    if (stream->codec == AV_CODEC_ID_DTS)
    {
      if (stream->profile == FF_PROFILE_DTS_HD_MA)
        strName = "dtshd_ma";
      else if (stream->profile == FF_PROFILE_DTS_HD_HRA)
        strName = "dtshd_hra";
      else
        strName = "dca";

      return strName;
    }

    AVCodec* codec = avcodec_find_decoder(stream->codec);
    if (codec)
      strName = codec->name;
  }
  return strName;
}

AVDictionary* FFmpegStream::GetFFMpegOptionsFromInput()
{
  // const std::shared_ptr<CDVDInputStreamFFmpeg> input =
  //   std::dynamic_pointer_cast<CDVDInputStreamFFmpeg>(m_pInput);

  CURL url;
  url.Parse(m_streamUrl);//m_pInput->GetURL();
  AVDictionary* options = nullptr;

  // For a local file we need the following protocol whitelist
  if (url.GetProtocol().empty() || url.IsProtocol("file"))
    av_dict_set(&options, "protocol_whitelist", "file,http,https,tcp,tls,crypto", 0);

  if (url.IsProtocol("http") || url.IsProtocol("https"))
  {
    std::map<std::string, std::string> protocolOptions;
    url.GetProtocolOptions(protocolOptions);
    std::string headers;
    bool hasUserAgent = false;
    bool hasCookies = false;
    for(std::map<std::string, std::string>::const_iterator it = protocolOptions.begin(); it != protocolOptions.end(); ++it)
    {
      std::string name = it->first;
      StringUtils::ToLower(name);
      const std::string &value = it->second;

      // set any of these ffmpeg options
      if (name == "seekable" || name == "reconnect" || name == "reconnect_at_eof" ||
          name == "reconnect_streamed" || name == "reconnect_delay_max" ||
          name == "icy" || name == "icy_metadata_headers" || name == "icy_metadata_packet")
      {
        Log(LOGLEVEL_DEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg option '%s: %s'",
                  it->first.c_str(), value.c_str());
        av_dict_set(&options, name.c_str(), value.c_str(), 0);
      }
      // map some standard http headers to the ffmpeg related options
      else if (name == "user-agent")
      {
        av_dict_set(&options, "user_agent", value.c_str(), 0);
        Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg option 'user_agent: %s'", value.c_str());
        hasUserAgent = true;
      }
      else if (name == "cookies")
      {
        // in the plural option expect multiple Set-Cookie values. They are passed \n delimited to FFMPEG
        av_dict_set(&options, "cookies", value.c_str(), 0);
        Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg option 'cookies: %s'", value.c_str());
        hasCookies = true;
      }
      else if (name == "cookie")
      {
        Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding ffmpeg header value 'cookie: %s'", value.c_str());
        headers.append(it->first).append(": ").append(value).append("\r\n");
        hasCookies = true;
      }
      // other standard headers (see https://en.wikipedia.org/wiki/List_of_HTTP_header_fields) are appended as actual headers
      else if (name == "accept" || name == "accept-language" || name == "accept-datetime" ||
               name == "authorization" || name == "cache-control" || name == "connection" || name == "content-md5" ||
               name == "date" || name == "expect" || name == "forwarded" || name == "from" || name == "if-match" ||
               name == "if-modified-since" || name == "if-none-match" || name == "if-range" || name == "if-unmodified-since" ||
               name == "max-forwards" || name == "origin" || name == "pragma" || name == "range" || name == "referer" ||
               name == "te" || name == "upgrade" || name == "via" || name == "warning" || name == "x-requested-with" ||
               name == "dnt" || name == "x-forwarded-for" || name == "x-forwarded-host" || name == "x-forwarded-proto" ||
               name == "front-end-https" || name == "x-http-method-override" || name == "x-att-deviceid" ||
               name == "x-wap-profile" || name == "x-uidh" || name == "x-csrf-token" || name == "x-request-id" ||
               name == "x-correlation-id")
      {
        if (name == "authorization")
        {
          Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding custom header option '%s: ***********'", it->first.c_str());
        }
        else
        {
          Log(LOGLEVEL_DEBUG, "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding custom header option '%s: %s'", it->first.c_str(), value.c_str());
        }
        headers.append(it->first).append(": ").append(value).append("\r\n");
      }
      // Any other headers that need to be sent would be user defined and should be prefixed
      // by a `!`. We mask these values so we don't log anything we shouldn't
      else if (name.length() > 0 && name[0] == '!')
      {
        Log(LOGLEVEL_DEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() adding user custom header option "
                  "'%s: ***********'",
                  it->first.c_str());
        headers.append(it->first.substr(1)).append(": ").append(value).append("\r\n");
      }
      // for everything else we ignore the headers options if not specified above
      else
      {
        Log(LOGLEVEL_DEBUG,
                  "CDVDDemuxFFmpeg::GetFFMpegOptionsFromInput() ignoring header option '%s'",
                  it->first.c_str());
      }
    }
    if (!hasUserAgent)
    {
      // set default xbmc user-agent.
      // TODO:
      av_dict_set(&options, "user_agent", "Kodi", 0);//CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_userAgent.c_str(), 0);
    }

    if (!headers.empty())
      av_dict_set(&options, "headers", headers.c_str(), 0);

    if (!hasCookies)
    {
      std::string cookies;
      // TODO:
      // if (XFILE::CCurlFile::GetCookies(url, cookies))
      //   av_dict_set(&options, "cookies", cookies.c_str(), 0);
    }
  }

  // if (input)
  // {
  //   const std::string host = input->GetProxyHost();
  //   if (!host.empty() && input->GetProxyType() == "http")
  //   {
  //     std::ostringstream urlStream;

  //     const uint16_t port = input->GetProxyPort();
  //     const std::string user = input->GetProxyUser();
  //     const std::string password = input->GetProxyPassword();

  //     urlStream << "http://";

  //     if (!user.empty()) {
  //       urlStream << user;
  //       if (!password.empty())
  //         urlStream << ":" << password;
  //       urlStream << "@";
  //     }

  //     urlStream << host << ':' << port;

  //     av_dict_set(&options, "http_proxy", urlStream.str().c_str(), 0);
  //   }
  // }

  return options;
}

int FFmpegStream::GetChapterCount()
{
  if (m_pFormatContext == NULL)
    return 0;

  return m_pFormatContext->nb_chapters;
}

int FFmpegStream::GetChapter()
{
  if (m_pFormatContext == NULL || m_currentPts == DVD_NOPTS_VALUE)
    return -1;

  for(unsigned i = 0; i < m_pFormatContext->nb_chapters; i++)
  {
    AVChapter* chapter = m_pFormatContext->chapters[i];
    if (m_currentPts >= ConvertTimestamp(chapter->start, chapter->time_base.den, chapter->time_base.num) &&
        m_currentPts <  ConvertTimestamp(chapter->end,   chapter->time_base.den, chapter->time_base.num))
      return i + 1;
  }

  return -1;
}

const char* FFmpegStream::GetChapterName(int chapterIdx)
{
  if (chapterIdx <= 0 || chapterIdx > GetChapterCount())
    chapterIdx = GetChapter();

  if (chapterIdx <= 0)
    return nullptr;

  AVDictionaryEntry* titleTag = av_dict_get(m_pFormatContext->chapters[chapterIdx - 1]->metadata,
                                                        "title", NULL, 0);
  if (titleTag)
    return titleTag->value;

  return nullptr;
}

int64_t FFmpegStream::GetChapterPos(int chapterIdx)
{
  if (chapterIdx <= 0 || chapterIdx > GetChapterCount())
    chapterIdx = GetChapter();
  if (chapterIdx <= 0)
    return 0;

  return static_cast<int64_t>(m_pFormatContext->chapters[chapterIdx - 1]->start * av_q2d(m_pFormatContext->chapters[chapterIdx - 1]->time_base));
}

bool FFmpegStream::SeekChapter(int chapter)
{
  if (chapter < 1)
    chapter = 1;

  if (m_pFormatContext == NULL)
    return false;

  if (chapter < 1 || chapter > (int)m_pFormatContext->nb_chapters)
    return false;

  AVChapter* ch = m_pFormatContext->chapters[chapter - 1];
  double dts = ConvertTimestamp(ch->start, ch->time_base.den, ch->time_base.num);
  return SeekTime(DVD_TIME_TO_MSEC(dts), true);
}
