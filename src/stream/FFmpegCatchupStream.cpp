/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FFmpegCatchupStream.h"

#include "threads/SingleLock.h"
#include "../utils/Log.h"

#ifdef TARGET_POSIX
#include "platform/posix/XTimeUtils.h"
#endif

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

#include <regex>

//#include "platform/posix/XTimeUtils.h"

#include <p8-platform/util/StringUtils.h>

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

FFmpegCatchupStream::FFmpegCatchupStream(IManageDemuxPacket* demuxPacketManager,
                                         std::string& defaultUrl,
                                         bool playbackAsLive,
                                         time_t programmeStartTime,
                                         time_t programmeEndTime,
                                         std::string& catchupUrlFormatString,
                                         time_t catchupBufferStartTime,
                                         time_t catchupBufferEndTime,
                                         long long catchupBufferOffset,
                                         int timezoneShift,
                                         int defaultProgrammeDuration,
                                         std::string& programmeCatchupId)
  : FFmpegStream(demuxPacketManager), m_bIsOpening(false), m_seekOffset(0),
    m_defaultUrl(defaultUrl), m_playbackAsLive(playbackAsLive),
    m_programmeStartTime(programmeStartTime), m_programmeEndTime(programmeEndTime),
    m_catchupUrlFormatString(catchupUrlFormatString),
    m_catchupBufferStartTime(catchupBufferStartTime), m_catchupBufferEndTime(catchupBufferEndTime),
    m_catchupBufferOffset(catchupBufferOffset), m_timezoneShift(timezoneShift), 
    m_defaultProgrammeDuration(defaultProgrammeDuration), m_programmeCatchupId(programmeCatchupId)
{
}

FFmpegCatchupStream::~FFmpegCatchupStream()
{

}

bool FFmpegCatchupStream::Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty)
{
  m_bIsOpening = true;
  bool ret = FFmpegStream::Open(streamUrl, mimeType, isRealTimeStream, programProperty);

  // We need to make an initial seek to the correct time otherwise the stream
  // will always start at the beginning instead of at the offset.
  // The value of time is irrelevant here we will want to seek to SEEK_CUR
  double temp = 0;
  DemuxSeekTime(0, false, temp);

  m_bIsOpening = false;
  return ret;
}

bool FFmpegCatchupStream::DemuxSeekTime(double time, bool backwards, double& startpts)
{
  if (/*!m_pInput ||*/ time < 0)
    return false;

  int whence = m_bIsOpening ? SEEK_CUR : SEEK_SET;
  int64_t seekResult = SeekStream(static_cast<int64_t>(time), whence);
  if (seekResult >= 0)
  {
    {
      CSingleLock lock(m_critSection);
      m_seekOffset = seekResult;
    }

    Log(LOGLEVEL_DEBUG, "Seek successful. m_seekOffset = %f, m_currentPts = %f, time = %f, backwards = %d, startptr = %f",
      m_seekOffset, m_currentPts, time, backwards, startpts);

    if (!m_bIsOpening)
    {
      DemuxReset();
      return m_demuxResetOpenSuccess;
    }

    return true;
  }

  Log(LOGLEVEL_DEBUG, "Seek failed. m_currentPts = %f, time = %f, backwards = %d, startptr = %f",
    m_currentPts, time, backwards, startpts);
  return false;
}

DemuxPacket* FFmpegCatchupStream::DemuxRead()
{
  DemuxPacket* pPacket = FFmpegStream::DemuxRead();
  if (pPacket)
  {
    CSingleLock lock(m_critSection);
    pPacket->pts += m_seekOffset;
    pPacket->dts += m_seekOffset;
  }

  return pPacket;
}

void FFmpegCatchupStream::GetCapabilities(INPUTSTREAM_CAPABILITIES& caps)
{
  Log(LOGLEVEL_DEBUG, "GetCapabilities()");
  caps.m_mask = INPUTSTREAM_CAPABILITIES::SUPPORTS_IDEMUX |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_IDISPLAYTIME |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_ITIME |
    // INPUTSTREAM_CAPABILITIES::SUPPORTS_IPOSTIME |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_SEEK |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_PAUSE |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_ICHAPTER;
}

int64_t FFmpegCatchupStream::SeekStream(int64_t position, int whence /* SEEK_SET */)
{
  int64_t ret = -1;
  if (m_catchupBufferStartTime > 0)
  {
    Log(LOGLEVEL_DEBUG, "SeekLiveStream - iPosition = %lld, iWhence = %d", position, whence);
    const time_t timeNow = time(0);
    switch (whence)
    {
      case SEEK_SET:
      {
        Log(LOGLEVEL_DEBUG, "SeekLiveStream - SeekSet: %lld", static_cast<long long>(position));
        position += 500;
        position /= 1000;
        if (m_catchupBufferStartTime + position < timeNow - 10)
        {
          ret = position;
          m_catchupBufferOffset = position;
        }
        else
        {
          // TODO we appear to require an extra 10 seconds less to skip hitting EOF
          // There must be a cleaner solutiom than this.
          ret = timeNow - m_catchupBufferStartTime - 10;
          m_catchupBufferOffset = ret;
        }
        ret *= DVD_TIME_BASE;

        m_streamUrl = GetUpdatedCatchupUrl();
      }
      break;
      case SEEK_CUR:
      {
        int64_t offset = m_catchupBufferOffset;
        //Log(LOGLEVEL_DEBUG, "SeekLiveStream - timeNow = %d, startTime = %d, iTvgShift = %d, offset = %d", timeNow, m_catchupStartTime, m_programmeChannelTvgShift, offset);
        ret = offset * DVD_TIME_BASE;
      }
      break;
      default:
        Log(LOGLEVEL_DEBUG, "SeekLiveStream - Unsupported SEEK command (%d)", whence);
      break;
    }
  }
  return ret;
}

int64_t FFmpegCatchupStream::LengthStream()
{
  int64_t length = -1;
  if (m_catchupBufferStartTime > 0 && m_catchupBufferEndTime >= m_catchupBufferStartTime)
  {
    INPUTSTREAM_TIMES times = {0};
    if (GetTimes(times) && times.ptsEnd >= times.ptsBegin)
      length = static_cast<int64_t>(times.ptsEnd - times.ptsBegin);
  }

  Log(LOGLEVEL_DEBUG, "LengthLiveStream: %lld", static_cast<long long>(length));

  return length;
}

bool FFmpegCatchupStream::GetTimes(INPUTSTREAM_TIMES& times)
{
  if (m_catchupBufferStartTime == 0)
    return false;

  times = {0};
  const time_t dateTimeNow = time(0);

  times.startTime = m_catchupBufferStartTime;
  if (m_playbackAsLive)
    times.ptsEnd = static_cast<double>(dateTimeNow - times.startTime) * DVD_TIME_BASE;
  else // it's like a video
    times.ptsEnd = static_cast<double>(std::min(dateTimeNow, m_catchupBufferEndTime) - times.startTime) * DVD_TIME_BASE;

  // Log(LOGLEVEL_DEBUG, "GetStreamTimes - Ch = %u \tTitle = \"%s\" \tepgTag->startTime = %ld \tepgTag->endTime = %ld",
  //           m_programmeUniqueChannelId, m_programmeTitle.c_str(), m_catchupBufferStartTime, m_catchupBufferEndTime);
  Log(LOGLEVEL_DEBUG, "GetStreamTimes - startTime = %ld \tptsStart = %lld \tptsBegin = %lld \tptsEnd = %lld",
            times.startTime, static_cast<long long>(times.ptsStart), static_cast<long long>(times.ptsBegin), static_cast<long long>(times.ptsEnd));

  return true;
}

void FFmpegCatchupStream::UpdateCurrentPTS()
{
  FFmpegStream::UpdateCurrentPTS();
  if (m_currentPts != DVD_NOPTS_VALUE)
    m_currentPts += m_seekOffset;
}

bool FFmpegCatchupStream::CanPauseStream()
{
  return true;
}

bool FFmpegCatchupStream::CanSeekStream()
{
  return true;
}

namespace
{

void FormatOffset(time_t tTime, std::string &urlFormatString)
{
  const std::string regexStr = ".*(\\{offset:(\\d+)\\}).*";
  std::cmatch mr;
  std::regex rx(regexStr);
  if (std::regex_match(urlFormatString.c_str(), mr, rx) && mr.length() >= 3)
  {
    std::string offsetExp = mr[1].first;
    std::string second = mr[1].second;
    if (second.length() > 0)
      offsetExp = offsetExp.erase(offsetExp.find(second));
    std::string dividerStr = mr[2].first;
    second = mr[2].second;
    if (second.length() > 0)
      dividerStr = dividerStr.erase(dividerStr.find(second));

    const time_t divider = stoi(dividerStr);
    if (divider != 0)
    {
      time_t offset = tTime / divider;
      if (offset < 0)
        offset = 0;
      urlFormatString.replace(urlFormatString.find(offsetExp), offsetExp.length(), std::to_string(offset));
    }
  }
}

void FormatTime(const char ch, const struct tm *pTime, std::string &urlFormatString)
{
  char str[] = { '{', ch, '}', 0 };
  auto pos = urlFormatString.find(str);
  if (pos != std::string::npos)
  {
    char buff[256], timeFmt[3];
    std::snprintf(timeFmt, sizeof(timeFmt), "%%%c", ch);
    std::strftime(buff, sizeof(buff), timeFmt, pTime);
    if (std::strlen(buff) > 0)
      urlFormatString.replace(pos, 3, buff);
  }
}

void FormatUtc(const char *str, time_t tTime, std::string &urlFormatString)
{
  auto pos = urlFormatString.find(str);
  if (pos != std::string::npos)
  {
    char buff[256];
    std::snprintf(buff, sizeof(buff), "%lu", tTime);
    urlFormatString.replace(pos, std::strlen(str), buff);
  }
}

std::string FormatDateTime(time_t dateTimeEpg, time_t duration, const std::string &urlFormatString)
{
  std::string fomrattedUrl = urlFormatString;

  const time_t dateTimeNow = std::time(0);
  tm* dateTime = std::localtime(&dateTimeEpg);

  FormatTime('Y', dateTime, fomrattedUrl);
  FormatTime('m', dateTime, fomrattedUrl);
  FormatTime('d', dateTime, fomrattedUrl);
  FormatTime('H', dateTime, fomrattedUrl);
  FormatTime('M', dateTime, fomrattedUrl);
  FormatTime('S', dateTime, fomrattedUrl);
  FormatUtc("{utc}", dateTimeEpg, fomrattedUrl);
  FormatUtc("${start}", dateTimeEpg, fomrattedUrl);
  FormatUtc("{utcend}", dateTimeEpg + duration, fomrattedUrl);
  FormatUtc("${end}", dateTimeEpg + duration, fomrattedUrl);
  FormatUtc("{lutc}", dateTimeNow, fomrattedUrl);
  FormatUtc("${timestamp}", dateTimeNow, fomrattedUrl);
  FormatUtc("{duration}", duration, fomrattedUrl);
  FormatOffset(dateTimeNow - dateTimeEpg, fomrattedUrl);

  Log(LOGLEVEL_DEBUG, "CArchiveConfig::FormatDateTime - \"%s\"", fomrattedUrl.c_str());

  return fomrattedUrl;
}

} // unnamed namespace

std::string FFmpegCatchupStream::GetUpdatedCatchupUrl() const
{
  time_t timeNow = time(0);
  time_t offset = m_catchupBufferStartTime + m_catchupBufferOffset;

  if (m_catchupBufferStartTime > 0 && offset < (timeNow - 5))
  {
    time_t duration = m_defaultProgrammeDuration;
    if (m_programmeStartTime > 0 && m_programmeStartTime < m_programmeEndTime)
      duration = m_programmeEndTime - m_programmeStartTime;

    Log(LOGLEVEL_DEBUG, "Offset Time - \"%lld\" - %s", static_cast<long long>(offset), m_catchupUrlFormatString.c_str());

    std::string catchupUrl = FormatDateTime(offset - m_timezoneShift, duration, m_catchupUrlFormatString);

    static const std::regex CATCHUP_ID_REGEX("\\{catchup-id\\}");
    if (!m_programmeCatchupId.empty())
      catchupUrl = std::regex_replace(catchupUrl, CATCHUP_ID_REGEX, m_programmeCatchupId);

    if (!catchupUrl.empty())
      return catchupUrl;
  }

  return m_defaultUrl;
}