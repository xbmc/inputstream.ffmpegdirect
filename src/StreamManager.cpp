/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamManager.h"

#include "stream/FFmpegCatchupStream.h"
#include "utils/HttpProxy.h"
#include "utils/Log.h"

#include <p8-platform/util/StringUtils.h>

using namespace ffmpegdirect;

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

void Log(const LogLevel logLevel, const char* format, ...)
{
  AddonLog addonLevel;

  switch (logLevel)
  {
    case LogLevel::LOGLEVEL_FATAL:
      addonLevel = AddonLog::ADDON_LOG_FATAL;
      break;
    case LogLevel::LOGLEVEL_ERROR:
      addonLevel = AddonLog::ADDON_LOG_ERROR;
      break;
    case LogLevel::LOGLEVEL_WARNING:
      addonLevel = AddonLog::ADDON_LOG_WARNING;
      break;
    case LogLevel::LOGLEVEL_INFO:
      addonLevel = AddonLog::ADDON_LOG_INFO;
      break;
    default:
      addonLevel = AddonLog::ADDON_LOG_DEBUG;
  }

  char buffer[16384];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ::kodi::addon::CAddonBase::m_interface->toKodi->addon_log_msg(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, addonLevel, buffer);
}

InputStreamFFmpegDirect::InputStreamFFmpegDirect(KODI_HANDLE instance, const std::string& version)
  : CInstanceInputStream(instance)
{
}

InputStreamFFmpegDirect::~InputStreamFFmpegDirect()
{
}

bool InputStreamFFmpegDirect::Open(INPUTSTREAM& props)
{
  Log(LOGLEVEL_INFO, "inputstream.ffmpegdirect: OpenStream() - Num Props: %d", props.m_nCountInfoValues);
  std::string tempString;

  for (size_t i = 0; i < props.m_nCountInfoValues; ++i)
  {
    Log(LOGLEVEL_INFO, "inputstream.ffmpegdirect property: %s = %s", props.m_ListItemProperties[i].m_strKey, props.m_ListItemProperties[i].m_strValue);

    if (PROGRAM_NUMBER == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_programProperty = props.m_ListItemProperties[i].m_strValue;
    }
    else if (IS_REALTIME_STREAM == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_isRealTimeStream = StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "true");
    }
    else if (STREAM_MODE == props.m_ListItemProperties[i].m_strKey)
    {
      if (StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "catchup"))
        properties.m_streamMode = StreamMode::CATCHUP;
    }
    else if (OPEN_MODE == props.m_ListItemProperties[i].m_strKey)
    {
      if (StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "ffmpeg"))
        properties.m_openMode = OpenMode::FFMPEG;
      else if (StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "curl"))
        properties.m_openMode = OpenMode::CURL;
    }
    else if (MANIFEST_TYPE == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_manifestType = props.m_ListItemProperties[i].m_strValue;
    }
    else if (DEFAULT_URL == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_defaultUrl = props.m_ListItemProperties[i].m_strValue;
    }
    else if (PLAYBACK_AS_LIVE == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_playbackAsLive = StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "true");
    }
    else if (PROGRAMME_START_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_programmeStartTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (PROGRAMME_END_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_programmeEndTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (CATCHUP_URL_FORMAT_STRING == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_catchupUrlFormatString = props.m_ListItemProperties[i].m_strValue;
    }
    else if (CATCHUP_URL_NEAR_LIVE_FORMAT_STRING == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_catchupUrlNearLiveFormatString = props.m_ListItemProperties[i].m_strValue;
    }
    else if (CATCHUP_BUFFER_START_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_catchupBufferStartTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (CATCHUP_BUFFER_END_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_catchupBufferEndTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (CATCHUP_BUFFER_OFFSET == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_catchupBufferOffset = std::stoll(tempString);
    }
    else if (CATCHUP_TERMINATES == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_catchupTerminates = StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "true");
    }
    else if (CATCHUP_GRANULARITY == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_catchupGranularity = std::stoi(tempString);
    }
    else if (TIMEZONE_SHIFT == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_timezoneShiftSecs = std::stoi(tempString);
    }
    else if (DEFAULT_PROGRAMME_DURATION == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      properties.m_defaultProgrammeDurationSecs = std::stoi(tempString);
    }
    else if (PROGRAMME_CATCHUP_ID == props.m_ListItemProperties[i].m_strKey)
    {
      properties.m_programmeCatchupId = props.m_ListItemProperties[i].m_strValue;
    }
  }

  m_streamUrl = props.m_strURL;
  m_mimeType = props.m_mimeType;

  Log(LOGLEVEL_INFO, "inputstream.ffmpegdirect property: mimetype = %s", m_mimeType.c_str());

  const std::string& manifestType = properties.m_manifestType;
  if (properties.m_openMode == OpenMode::DEFAULT)
  {
    if (m_mimeType == "application/x-mpegURL" || // HLS
        m_mimeType == "application/vnd.apple.mpegurl" || //HLS
        m_mimeType == "application/xml+dash" ||
        manifestType == "hls" || // HLS
        manifestType == "mpd" || // DASH
        manifestType == "ism") //Smooth Streaming
      properties.m_openMode = OpenMode::FFMPEG;
    else
      properties.m_openMode = OpenMode::CURL;
  }

  HttpProxy httpProxy;

  bool useHttpProxy = kodi::GetSettingBoolean("useHttpProxy");
  if (useHttpProxy)
  {
    httpProxy.SetProxyHost(kodi::GetSettingString("httpProxyHost"));
    kodi::Log(ADDON_LOG_INFO, "HttpProxy host set: '%s'", httpProxy.GetProxyHost().c_str());

    httpProxy.SetProxyPort(static_cast<uint16_t>(kodi::GetSettingInt("httpProxyPort")));
    kodi::Log(ADDON_LOG_INFO, "HttpProxy port set: %d", static_cast<int>(httpProxy.GetProxyPort()));

    httpProxy.SetProxyUser(kodi::GetSettingString("httpProxyUser"));
    kodi::Log(ADDON_LOG_INFO, "HttpProxy user set: '%s'", httpProxy.GetProxyUser().c_str());

    httpProxy.SetProxyPassword(kodi::GetSettingString("httpProxyPassword"));
  }

  if (properties.m_streamMode == StreamMode::CATCHUP)
    m_stream = std::make_shared<FFmpegCatchupStream>(static_cast<IManageDemuxPacket*>(this), properties, httpProxy);
  else
    m_stream = std::make_shared<FFmpegStream>(static_cast<IManageDemuxPacket*>(this), properties.m_openMode, httpProxy);

  m_stream->SetVideoResolution(m_videoWidth, m_videoHeight);

  m_opened = m_stream->Open(m_streamUrl, m_mimeType, properties.m_isRealTimeStream, properties.m_programProperty);

  return m_opened;
}

void InputStreamFFmpegDirect::Close()
{
  m_opened = false;

  m_stream->Close();
}

void InputStreamFFmpegDirect::GetCapabilities(INPUTSTREAM_CAPABILITIES &caps)
{
  Log(LOGLEVEL_DEBUG, "GetCapabilities()");
  m_stream->GetCapabilities(caps);
}

INPUTSTREAM_IDS InputStreamFFmpegDirect::GetStreamIds()
{
  Log(LOGLEVEL_DEBUG, "GetStreamIds()");
  return m_stream->GetStreamIds();
}

INPUTSTREAM_INFO InputStreamFFmpegDirect::GetStream(int streamid)
{
  return m_stream->GetStream(streamid);
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

DemuxPacket* InputStreamFFmpegDirect::DemuxRead()
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

void InputStreamFFmpegDirect::SetVideoResolution(int width, int height)
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

bool InputStreamFFmpegDirect::GetTimes(INPUTSTREAM_TIMES& times)
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

DemuxPacket* InputStreamFFmpegDirect::AllocateDemuxPacketFromInputStreamAPI(int dataSize)
{
  return AllocateDemuxPacket(dataSize);
}

DemuxPacket* InputStreamFFmpegDirect::AllocateEncryptedDemuxPacketFromInputStreamAPI(int dataSize, unsigned int encryptedSubsampleCount)
{
  return AllocateEncryptedDemuxPacket(dataSize, encryptedSubsampleCount);
}

void InputStreamFFmpegDirect::FreeDemuxPacketFromInputStreamAPI(DemuxPacket* packet)
{
  return FreeDemuxPacket(packet);
}

/*****************************************************************************************************/

class ATTRIBUTE_HIDDEN CMyAddon
  : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType,
                                      const std::string& instanceID,
                                      KODI_HANDLE instance,
                                      const std::string& version,
                                      KODI_HANDLE& addonInstance) override
  {
    if (instanceType == ADDON_INSTANCE_INPUTSTREAM)
    {
      addonInstance = new InputStreamFFmpegDirect(instance, version);
      return ADDON_STATUS_OK;
    }
    return ADDON_STATUS_NOT_IMPLEMENTED;
  }
};

ADDONCREATOR(CMyAddon)
