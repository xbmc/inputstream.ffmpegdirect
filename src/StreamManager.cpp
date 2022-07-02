/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamManager.h"

#include "stream/FFmpegCatchupStream.h"
#include "stream/TimeshiftStream.h"
#include "stream/url/URL.h"
#include "utils/HttpProxy.h"
#include "utils/Log.h"

#include <kodi/tools/StringUtils.h>

using namespace ffmpegdirect;
using namespace kodi::tools;

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

void Log(const LogLevel logLevel, const char* format, ...)
{
  ADDON_LOG addonLevel;

  switch (logLevel)
  {
    case LogLevel::LOGLEVEL_FATAL:
      addonLevel = ADDON_LOG::ADDON_LOG_FATAL;
      break;
    case LogLevel::LOGLEVEL_ERROR:
      addonLevel = ADDON_LOG::ADDON_LOG_ERROR;
      break;
    case LogLevel::LOGLEVEL_WARNING:
      addonLevel = ADDON_LOG::ADDON_LOG_WARNING;
      break;
    case LogLevel::LOGLEVEL_INFO:
      addonLevel = ADDON_LOG::ADDON_LOG_INFO;
      break;
    default:
      addonLevel = ADDON_LOG::ADDON_LOG_DEBUG;
  }

  char buffer[16384];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  kodi::Log(addonLevel, buffer);
}

InputStreamFFmpegDirect::InputStreamFFmpegDirect(const kodi::addon::IInstanceInfo& instance)
  : CInstanceInputStream(instance)
{
}

InputStreamFFmpegDirect::~InputStreamFFmpegDirect()
{
}

bool InputStreamFFmpegDirect::Open(const kodi::addon::InputstreamProperty& props)
{
  Log(LOGLEVEL_INFO, "inputstream.ffmpegdirect: OpenStream() - Num Props: %d", props.GetPropertiesAmount());

  for (const auto& prop : props.GetProperties())
  {
    if (StringUtils::StartsWith(prop.second, "http://") || StringUtils::StartsWith(prop.second, "https://"))
      Log(LOGLEVEL_INFO, "inputstream.ffmpegdirect property: %s = %s", prop.first.c_str(), CURL::GetRedacted(prop.second).c_str());
    else
      Log(LOGLEVEL_INFO, "inputstream.ffmpegdirect property: %s = %s", prop.first.c_str(), prop.second.c_str());

    if (PROGRAM_NUMBER == prop.first)
    {
      m_properties.m_programProperty = prop.second;
    }
    else if (IS_REALTIME_STREAM == prop.first)
    {
      m_properties.m_isRealTimeStream = StringUtils::EqualsNoCase(prop.second, "true");
    }
    else if (STREAM_MODE == prop.first)
    {
      if (StringUtils::EqualsNoCase(prop.second, "catchup"))
        m_properties.m_streamMode = StreamMode::CATCHUP;
      else if (StringUtils::EqualsNoCase(prop.second, "timeshift"))
        m_properties.m_streamMode = StreamMode::TIMESHIFT;
    }
    else if (OPEN_MODE == prop.first)
    {
      if (StringUtils::EqualsNoCase(prop.second, "ffmpeg"))
        m_properties.m_openMode = OpenMode::FFMPEG;
      else if (StringUtils::EqualsNoCase(prop.second, "curl"))
        m_properties.m_openMode = OpenMode::CURL;
    }
    else if (MANIFEST_TYPE == prop.first)
    {
      m_properties.m_manifestType = prop.second;
    }
    else if (DEFAULT_URL == prop.first)
    {
      m_properties.m_defaultUrl = prop.second;
    }
    else if (PLAYBACK_AS_LIVE == prop.first)
    {
      m_properties.m_playbackAsLive = StringUtils::EqualsNoCase(prop.second, "true");
    }
    else if (PROGRAMME_START_TIME == prop.first)
    {
      m_properties.m_programmeStartTime = static_cast<time_t>(std::stoll(prop.second));
    }
    else if (PROGRAMME_END_TIME == prop.first)
    {
      m_properties.m_programmeEndTime = static_cast<time_t>(std::stoll(prop.second));
    }
    else if (CATCHUP_URL_FORMAT_STRING == prop.first)
    {
      m_properties.m_catchupUrlFormatString = prop.second;
    }
    else if (CATCHUP_URL_NEAR_LIVE_FORMAT_STRING == prop.first)
    {
      m_properties.m_catchupUrlNearLiveFormatString = prop.second;
    }
    else if (CATCHUP_BUFFER_START_TIME == prop.first)
    {
      m_properties.m_catchupBufferStartTime = static_cast<time_t>(std::stoll(prop.second));
    }
    else if (CATCHUP_BUFFER_END_TIME == prop.first)
    {
      m_properties.m_catchupBufferEndTime = static_cast<time_t>(std::stoll(prop.second));
    }
    else if (CATCHUP_BUFFER_OFFSET == prop.first)
    {
      m_properties.m_catchupBufferOffset = std::stoll(prop.second);
    }
    else if (CATCHUP_TERMINATES == prop.first)
    {
      m_properties.m_catchupTerminates = StringUtils::EqualsNoCase(prop.second, "true");
    }
    else if (CATCHUP_GRANULARITY == prop.first)
    {
      m_properties.m_catchupGranularity = std::stoi(prop.second);
    }
    else if (TIMEZONE_SHIFT == prop.first)
    {
      m_properties.m_timezoneShiftSecs = std::stoi(prop.second);
    }
    else if (DEFAULT_PROGRAMME_DURATION == prop.first)
    {
      m_properties.m_defaultProgrammeDurationSecs = std::stoi(prop.second);
    }
    else if (PROGRAMME_CATCHUP_ID == prop.first)
    {
      m_properties.m_programmeCatchupId = prop.second;
    }
  }

  m_streamUrl = props.GetURL();
  m_mimeType = props.GetMimeType();

  Log(LOGLEVEL_INFO, "Stream mimetype: %s", m_mimeType.c_str());

  const std::string& manifestType = m_properties.m_manifestType;
  if (m_properties.m_openMode == OpenMode::DEFAULT)
  {
    if (m_mimeType == "application/x-mpegURL" || // HLS
        m_mimeType == "application/vnd.apple.mpegurl" || //HLS
        m_mimeType == "application/xml+dash" ||
        manifestType == "hls" || // HLS
        manifestType == "mpd" || // DASH
        manifestType == "ism" || //Smooth Streaming
        StringUtils::StartsWithNoCase(m_streamUrl, "rtp://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtsp://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtsps://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "satip://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "sdp://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "udp://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "tcp://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "mms://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "mmst://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "mmsh://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtmp://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtmpt://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtmpe://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtmpte://") ||
        StringUtils::StartsWithNoCase(m_streamUrl, "rtmps://"))
      m_properties.m_openMode = OpenMode::FFMPEG;
    else
      m_properties.m_openMode = OpenMode::CURL;
  }

  HttpProxy httpProxy;

  bool useHttpProxy = kodi::addon::GetSettingBoolean("useHttpProxy");
  if (useHttpProxy)
  {
    httpProxy.SetProxyHost(kodi::addon::GetSettingString("httpProxyHost"));
    kodi::Log(ADDON_LOG_INFO, "HttpProxy host set: '%s'", httpProxy.GetProxyHost().c_str());

    httpProxy.SetProxyPort(static_cast<uint16_t>(kodi::addon::GetSettingInt("httpProxyPort")));
    kodi::Log(ADDON_LOG_INFO, "HttpProxy port set: %d", static_cast<int>(httpProxy.GetProxyPort()));

    httpProxy.SetProxyUser(kodi::addon::GetSettingString("httpProxyUser"));
    kodi::Log(ADDON_LOG_INFO, "HttpProxy user set: '%s'", httpProxy.GetProxyUser().c_str());

    httpProxy.SetProxyPassword(kodi::addon::GetSettingString("httpProxyPassword"));
  }

  if (m_properties.m_streamMode == StreamMode::CATCHUP)
    m_stream = std::make_shared<FFmpegCatchupStream>(static_cast<IManageDemuxPacket*>(this), m_properties, httpProxy);
  else if (m_properties.m_streamMode == StreamMode::TIMESHIFT)
    m_stream = std::make_shared<TimeshiftStream>(static_cast<IManageDemuxPacket*>(this), m_properties, httpProxy);
  else
    m_stream = std::make_shared<FFmpegStream>(static_cast<IManageDemuxPacket*>(this), m_properties, httpProxy);

  m_stream->SetVideoResolution(m_videoWidth, m_videoHeight);

  m_opened = m_stream->Open(m_streamUrl, m_mimeType, m_properties.m_isRealTimeStream, m_properties.m_programProperty);

  return m_opened;
}

void InputStreamFFmpegDirect::Close()
{
  m_opened = false;

  m_stream->Close();
}

void InputStreamFFmpegDirect::GetCapabilities(kodi::addon::InputstreamCapabilities &caps)
{
  Log(LOGLEVEL_DEBUG, "GetCapabilities()");
  m_stream->GetCapabilities(caps);
}

bool InputStreamFFmpegDirect::GetStreamIds(std::vector<unsigned int>& ids)
{
  Log(LOGLEVEL_DEBUG, "GetStreamIds()");
  return m_stream->GetStreamIds(ids);
}

bool InputStreamFFmpegDirect::GetStream(int streamid, kodi::addon::InputstreamInfo& info)
{
  return m_stream->GetStream(streamid, info);
}

void InputStreamFFmpegDirect::EnableStream(int streamid, bool enable)
{
  m_stream->EnableStream(streamid, enable);
}

bool InputStreamFFmpegDirect::OpenStream(int streamid)
{
  return m_stream->OpenStream(streamid);
}

void InputStreamFFmpegDirect::DemuxReset()
{
  m_stream->DemuxReset();
}

void InputStreamFFmpegDirect::DemuxAbort()
{
  m_stream->DemuxAbort();
}

void InputStreamFFmpegDirect::DemuxFlush()
{
  m_stream->DemuxFlush();
}

DEMUX_PACKET* InputStreamFFmpegDirect::DemuxRead()
{
  return m_stream->DemuxRead();
}

bool InputStreamFFmpegDirect::DemuxSeekTime(double time, bool backwards, double& startpts)
{
  return m_stream->DemuxSeekTime(time, backwards, startpts);
}

void InputStreamFFmpegDirect::DemuxSetSpeed(int speed)
{
  m_stream->DemuxSetSpeed(speed);
}

void InputStreamFFmpegDirect::SetVideoResolution(unsigned int width, unsigned int height)
{
  Log(LOGLEVEL_DEBUG, "inputstream.ffmpegdirect: SetVideoResolution()");

  m_videoWidth = width;
  m_videoHeight = height;
}

int InputStreamFFmpegDirect::GetTotalTime()
{
  return m_stream->GetTotalTime();
}

int InputStreamFFmpegDirect::GetTime()
{
  return m_stream->GetTime();
}

bool InputStreamFFmpegDirect::GetTimes(kodi::addon::InputstreamTimes& times)
{
  return m_stream->GetTimes(times);
}

bool InputStreamFFmpegDirect::PosTime(int ms)
{
  return m_stream->PosTime(ms);
}

int InputStreamFFmpegDirect::GetChapter()
{
  return m_stream->GetChapter();
}

int InputStreamFFmpegDirect::GetChapterCount()
{
  return m_stream->GetChapterCount();
}

const char* InputStreamFFmpegDirect::GetChapterName(int ch)
{
  return m_stream->GetChapterName(ch);
}

int64_t InputStreamFFmpegDirect::GetChapterPos(int ch)
{
  return m_stream->GetChapterPos(ch);
}

bool InputStreamFFmpegDirect::SeekChapter(int ch)
{
  return m_stream->SeekChapter(ch);
}

int InputStreamFFmpegDirect::ReadStream(uint8_t* buf, unsigned int size)
{
  return m_stream->ReadStream(buf, size);
}

int64_t InputStreamFFmpegDirect::SeekStream(int64_t position, int whence /* SEEK_SET */)
{
  return m_stream->SeekStream(position, whence);
}

int64_t InputStreamFFmpegDirect::PositionStream()
{
  return m_stream->PositionStream();
}

int64_t InputStreamFFmpegDirect::LengthStream()
{
  return m_stream->LengthStream();
}

bool InputStreamFFmpegDirect::IsRealTimeStream()
{
  return m_stream->IsRealTimeStream();
}

/*****************************************************************************************************/

DEMUX_PACKET* InputStreamFFmpegDirect::AllocateDemuxPacketFromInputStreamAPI(int dataSize)
{
  return AllocateDemuxPacket(dataSize);
}

DEMUX_PACKET* InputStreamFFmpegDirect::AllocateEncryptedDemuxPacketFromInputStreamAPI(int dataSize, unsigned int encryptedSubsampleCount)
{
  return AllocateEncryptedDemuxPacket(dataSize, encryptedSubsampleCount);
}

void InputStreamFFmpegDirect::FreeDemuxPacketFromInputStreamAPI(DEMUX_PACKET* packet)
{
  return FreeDemuxPacket(packet);
}

/*****************************************************************************************************/

class ATTR_DLL_LOCAL CMyAddon
  : public kodi::addon::CAddonBase
{
public:
  CMyAddon() = default;
  ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
                              KODI_ADDON_INSTANCE_HDL& hdl) override
  {
    if (instance.IsType(ADDON_INSTANCE_INPUTSTREAM))
    {
      hdl = new InputStreamFFmpegDirect(instance);
      return ADDON_STATUS_OK;
    }
    return ADDON_STATUS_NOT_IMPLEMENTED;
  }
};

ADDONCREATOR(CMyAddon)
