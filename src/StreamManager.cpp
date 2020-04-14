/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "StreamManager.h"

#include "utils/HttpProxy.h"
#include "utils/Log.h"

#include <p8-platform/util/StringUtils.h>

using namespace ffmpegdirect::utils;

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

void Log(const LogLevel loglevel, const char* format, ...)
{
  char buffer[16384];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ::kodi::addon::CAddonBase::m_interface->toKodi->addon_log_msg(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, loglevel, buffer);
}

CInputStreamLibavformat::CInputStreamLibavformat(KODI_HANDLE instance)
  : CInstanceInputStream(instance)
{
}

CInputStreamLibavformat::~CInputStreamLibavformat()
{
}

bool CInputStreamLibavformat::Open(INPUTSTREAM& props)
{
  Log(LOGLEVEL_NOTICE, "inputstream.ffmpegdirect: OpenStream() - Num Props: %d", props.m_nCountInfoValues);
  std::string tempString;

  for (size_t i = 0; i < props.m_nCountInfoValues; ++i)
  {
    Log(LOGLEVEL_NOTICE, "inputstream.ffmpegdirect property: %s = %s", props.m_ListItemProperties[i].m_strKey, props.m_ListItemProperties[i].m_strValue);

    if (MIME_TYPE == props.m_ListItemProperties[i].m_strKey)
    {
      m_mimeType = props.m_ListItemProperties[i].m_strValue;
    }
    else if (PROGRAM_NUMBER == props.m_ListItemProperties[i].m_strKey)
    {
      m_programProperty = props.m_ListItemProperties[i].m_strValue;
    }
    else if (IS_REALTIME_STREAM == props.m_ListItemProperties[i].m_strKey)
    {
      m_isRealTimeStream = StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "true");
    }
    else if (STREAM_MODE == props.m_ListItemProperties[i].m_strKey)
    {
      if (StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "catchup"))
        m_streamMode = StreamMode::CATCHUP;
    }
    else if (DEFAULT_URL == props.m_ListItemProperties[i].m_strKey)
    {
      m_defaultUrl = props.m_ListItemProperties[i].m_strValue;
    }
    else if (PLAYBACK_AS_LIVE == props.m_ListItemProperties[i].m_strKey)
    {
      m_playbackAsLive = StringUtils::EqualsNoCase(props.m_ListItemProperties[i].m_strValue, "true");
    }
    else if (PROGRAMME_START_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_programmeStartTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (PROGRAMME_END_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_programmeEndTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (CATCHUP_URL_FORMAT_STRING == props.m_ListItemProperties[i].m_strKey)
    {
      m_catchupUrlFormatString = props.m_ListItemProperties[i].m_strValue;
    }
    else if (CATCHUP_URL_NEAR_LIVE_FORMAT_STRING == props.m_ListItemProperties[i].m_strKey)
    {
      m_catchupUrlNearLiveFormatString = props.m_ListItemProperties[i].m_strValue;
    }
    else if (CATCHUP_BUFFER_START_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_catchupBufferStartTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (CATCHUP_BUFFER_END_TIME == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_catchupBufferEndTime = static_cast<time_t>(std::stoll(tempString));
    }
    else if (CATCHUP_BUFFER_OFFSET == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_catchupBufferOffset = std::stoll(tempString);
    }
    else if (TIMEZONE_SHIFT == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_timezoneShiftSecs = std::stoi(tempString);
    }
    else if (DEFAULT_PROGRAMME_DURATION == props.m_ListItemProperties[i].m_strKey)
    {
      tempString = props.m_ListItemProperties[i].m_strValue;
      m_defaultProgrammeDurationSecs = std::stoi(tempString);
    }
    else if (PROGRAMME_CATCHUP_ID == props.m_ListItemProperties[i].m_strKey)
    {
      m_programmeCatchupId = props.m_ListItemProperties[i].m_strValue;
    }
  }

  m_streamUrl = props.m_strURL;

  HttpProxy httpProxy;

  bool useHttpProxy = kodi::GetSettingBoolean("useHttpProxy"); 
  if (useHttpProxy)
  {
    httpProxy.SetProxyHost(kodi::GetSettingString("httpProxyHost"));
    kodi::Log(ADDON_LOG_NOTICE, "HttpProxy host set: '%s'", httpProxy.GetProxyHost().c_str());

    httpProxy.SetProxyPort(static_cast<uint16_t>(kodi::GetSettingInt("httpProxyPort")));
    kodi::Log(ADDON_LOG_NOTICE, "HttpProxy port set: %d", static_cast<int>(httpProxy.GetProxyPort()));

    httpProxy.SetProxyUser(kodi::GetSettingString("httpProxyUser"));
    kodi::Log(ADDON_LOG_NOTICE, "HttpProxy user set: '%s'", httpProxy.GetProxyUser().c_str());

    httpProxy.SetProxyPassword(kodi::GetSettingString("httpProxyPassword"));
  }

  if (m_streamMode == StreamMode::CATCHUP)
    m_stream = std::make_shared<FFmpegCatchupStream>(static_cast<IManageDemuxPacket*>(this),
                                                     httpProxy,
                                                     m_defaultUrl,
                                                     m_playbackAsLive,
                                                     m_programmeStartTime,
                                                     m_programmeEndTime,
                                                     m_catchupUrlFormatString,
                                                     m_catchupUrlNearLiveFormatString,
                                                     m_catchupBufferStartTime,
                                                     m_catchupBufferEndTime,
                                                     m_catchupBufferOffset,
                                                     m_timezoneShiftSecs,
                                                     m_defaultProgrammeDurationSecs,
                                                     m_programmeCatchupId);
  else
    m_stream = std::make_shared<FFmpegStream>(static_cast<IManageDemuxPacket*>(this), httpProxy);

  m_stream->SetVideoResolution(m_videoWidth, m_videoHeight);

  m_opened = m_stream->Open(m_streamUrl, m_mimeType, m_isRealTimeStream, m_programProperty);

  return m_opened;
}

void CInputStreamLibavformat::Close()
{
  m_opened = false;

  m_stream->Close();
}

void CInputStreamLibavformat::GetCapabilities(INPUTSTREAM_CAPABILITIES &caps)
{
  Log(LOGLEVEL_DEBUG, "GetCapabilities()");
  m_stream->GetCapabilities(caps);
}

INPUTSTREAM_IDS CInputStreamLibavformat::GetStreamIds()
{
  Log(LOGLEVEL_DEBUG, "GetStreamIds()");
  return m_stream->GetStreamIds();
}

INPUTSTREAM_INFO CInputStreamLibavformat::GetStream(int streamid)
{
  return m_stream->GetStream(streamid);
}

void CInputStreamLibavformat::EnableStream(int streamid, bool enable)
{
  m_stream->EnableStream(streamid, enable);
}

bool CInputStreamLibavformat::OpenStream(int streamid)
{
  return m_stream->OpenStream(streamid);
}

void CInputStreamLibavformat::DemuxReset()
{
  m_stream->DemuxReset();
}

void CInputStreamLibavformat::DemuxAbort()
{
  m_stream->DemuxAbort();
}

void CInputStreamLibavformat::DemuxFlush()
{
  m_stream->DemuxFlush();
}

DemuxPacket* CInputStreamLibavformat::DemuxRead()
{
  return m_stream->DemuxRead();
}

bool CInputStreamLibavformat::DemuxSeekTime(double time, bool backwards, double& startpts)
{
  return m_stream->DemuxSeekTime(time, backwards, startpts);
}

void CInputStreamLibavformat::DemuxSetSpeed(int speed)
{
  m_stream->DemuxSetSpeed(speed);
}

void CInputStreamLibavformat::SetVideoResolution(int width, int height)
{
  Log(LOGLEVEL_DEBUG, "inputstream.ffmpegdirect: SetVideoResolution()");

  m_videoWidth = width;
  m_videoHeight = height;
}

int CInputStreamLibavformat::GetTotalTime()
{
  return m_stream->GetTotalTime();
}

int CInputStreamLibavformat::GetTime()
{
  return m_stream->GetTime();
}

bool CInputStreamLibavformat::GetTimes(INPUTSTREAM_TIMES& times)
{
  return m_stream->GetTimes(times);
}

bool CInputStreamLibavformat::PosTime(int ms)
{
  return m_stream->PosTime(ms);
}

int CInputStreamLibavformat::GetChapter()
{
  return m_stream->GetChapter();
}

int CInputStreamLibavformat::GetChapterCount()
{
  return m_stream->GetChapterCount();
}

const char* CInputStreamLibavformat::GetChapterName(int ch)
{
  return m_stream->GetChapterName(ch);
}

int64_t CInputStreamLibavformat::GetChapterPos(int ch)
{
  return m_stream->GetChapterPos(ch);
}

bool CInputStreamLibavformat::SeekChapter(int ch)
{
  return m_stream->SeekChapter(ch);
}

int CInputStreamLibavformat::ReadStream(uint8_t* buf, unsigned int size)
{
  return m_stream->ReadStream(buf, size);
}

int64_t CInputStreamLibavformat::SeekStream(int64_t position, int whence /* SEEK_SET */)
{
  return m_stream->SeekStream(position, whence);
}

int64_t CInputStreamLibavformat::PositionStream()
{
  return m_stream->PositionStream();
}

int64_t CInputStreamLibavformat::LengthStream()
{
  return m_stream->LengthStream();
}

bool CInputStreamLibavformat::IsRealTimeStream()
{
  return m_stream->IsRealTimeStream();
}

/*****************************************************************************************************/

DemuxPacket* CInputStreamLibavformat::AllocateDemuxPacketFromInputStreamAPI(int dataSize)
{
  return AllocateDemuxPacket(dataSize);
}

DemuxPacket* CInputStreamLibavformat::AllocateEncryptedDemuxPacketFromInputStreamAPI(int dataSize, unsigned int encryptedSubsampleCount)
{
  return AllocateEncryptedDemuxPacket(dataSize, encryptedSubsampleCount);
}

void CInputStreamLibavformat::FreeDemuxPacketFromInputStreamAPI(DemuxPacket* packet)
{
  return FreeDemuxPacket(packet);
}

/*****************************************************************************************************/

class CMyAddon
  : public kodi::addon::CAddonBase
{
public:
  CMyAddon() { }
  virtual ADDON_STATUS CreateInstance(int instanceType, std::string instanceID, KODI_HANDLE instance, KODI_HANDLE& addonInstance) override
  {
    if (instanceType == ADDON_INSTANCE_INPUTSTREAM)
    {
      addonInstance = new CInputStreamLibavformat(instance);
      return ADDON_STATUS_OK;
    }
    return ADDON_STATUS_NOT_IMPLEMENTED;
  }
};

ADDONCREATOR(CMyAddon)
