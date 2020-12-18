/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "FFmpegCatchupStream.h"

#include "CurlCatchupInput.h"
#include "url/URL.h"
#include "../utils/Log.h"

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

#include <iomanip>
#include <regex>

//#include "platform/posix/XTimeUtils.h"

#include <kodi/tools/StringUtils.h>
#include <kodi/Filesystem.h>

using namespace ffmpegdirect;
using namespace kodi::tools;

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

FFmpegCatchupStream::FFmpegCatchupStream(IManageDemuxPacket* demuxPacketManager,
                                         const Properties& props,
                                         const HttpProxy& httpProxy)
  : FFmpegStream(demuxPacketManager, props, std::make_shared<CurlCatchupInput>(), httpProxy),
    m_isOpeningStream(false), m_seekOffset(0),
    m_defaultUrl(props.m_defaultUrl), m_playbackAsLive(props.m_playbackAsLive),
    m_programmeStartTime(props.m_programmeStartTime), m_programmeEndTime(props.m_programmeEndTime),
    m_catchupUrlFormatString(props.m_catchupUrlFormatString),
    m_catchupUrlNearLiveFormatString(props.m_catchupUrlNearLiveFormatString),
    m_catchupBufferStartTime(props.m_catchupBufferStartTime), m_catchupBufferEndTime(props.m_catchupBufferEndTime),
    m_catchupBufferOffset(props.m_catchupBufferOffset), m_catchupTerminates(props.m_catchupTerminates),
    m_catchupGranularity(props.m_catchupGranularity), m_timezoneShift(props.m_timezoneShiftSecs),
    m_defaultProgrammeDuration(props.m_defaultProgrammeDurationSecs), m_programmeCatchupId(props.m_programmeCatchupId)
{
  m_catchupGranularityLowWaterMark = m_catchupGranularity - (m_catchupGranularity / 4);
}

FFmpegCatchupStream::~FFmpegCatchupStream()
{

}

bool FFmpegCatchupStream::Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty)
{
  m_isOpeningStream = true;
  bool ret = FFmpegStream::Open(streamUrl, mimeType, isRealTimeStream, programProperty);

  m_lastPacketWasAvoidedEOF = false;

  // We need to make an initial seek to the correct time otherwise the stream
  // will always start at the beginning instead of at the offset.
  // The value of time is irrelevant here we will want to seek to SEEK_CUR
  DemuxSeekTime(0);

  m_isOpeningStream = false;
  return ret;
}

bool FFmpegCatchupStream::DemuxSeekTime(double timeMs, bool backwards, double& startpts)
{
  if (/*!m_pInput ||*/ timeMs < 0)
    return false;

  int64_t seekResult = SeekCatchupStream(timeMs, backwards);
  if (seekResult >= 0)
  {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_seekOffset = seekResult;
    }

    Log(LOGLEVEL_DEBUG, "%s - Seek successful. m_seekOffset = %f, m_currentPts = %f, time = %f, backwards = %d, startpts = %f",
      __FUNCTION__, m_seekOffset, m_currentPts, timeMs, backwards, startpts);

    if (!m_isOpeningStream)
    {
      DemuxReset();
      return m_demuxResetOpenSuccess;
    }

    return true;
  }

  Log(LOGLEVEL_DEBUG, "%s - Seek failed. m_currentPts = %f, time = %f, backwards = %d, startpts = %f",
    __FUNCTION__, m_currentPts, timeMs, backwards, startpts);
  return false;
}

DEMUX_PACKET* FFmpegCatchupStream::DemuxRead()
{
  DEMUX_PACKET* pPacket = FFmpegStream::DemuxRead();
  if (pPacket)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    pPacket->pts += m_seekOffset;
    pPacket->dts += m_seekOffset;

    if (m_lastPacketResult == AVERROR_EOF && m_catchupTerminates && !m_isOpeningStream && !m_lastSeekWasLive)
    {
      if (!m_lastPacketWasAvoidedEOF)
      {
        Log(LOGLEVEL_INFO, "%s - EOF detected on terminating catchup stream, starting continuing stream at offset: %lld, ending offset approx %lld", __FUNCTION__, m_previousLiveBufferOffset, static_cast<long long>(std::time(nullptr) - m_catchupBufferStartTime));

        m_seekCorrectsEOF = true;
        DemuxSeekTime(m_previousLiveBufferOffset * 1000);
        m_seekCorrectsEOF = false;
      }
      m_lastPacketWasAvoidedEOF = true;
    }
    else
    {
      m_lastPacketWasAvoidedEOF = false;
    }

    m_currentDemuxTime = static_cast<double>(pPacket->pts) / 1000;
  }

  return pPacket;
}

bool FFmpegCatchupStream::CheckReturnEmptyOnPacketResult(int result)
{
  // If the server returns EOF then for a terminating stream we should should keep playing
  // sending an empty packet instead will allow VideoPlayer to continue as we swap to an
  // updated stream running from current end time to now
  // This will only happen if we are within the default programme duration of live

  if (result == AVERROR_EOF)
    Log(LOGLEVEL_DEBUG, "%s - isEOF: %d, terminates: %d, isOpening: %d, lastSeekWasLive: %d, lastLiveOffset+duration: %lld > currentDemuxTime: %lld",
        __FUNCTION__, result == AVERROR_EOF, m_catchupTerminates, m_isOpeningStream, m_lastSeekWasLive, m_previousLiveBufferOffset + m_defaultProgrammeDuration, static_cast<long long>(m_currentDemuxTime) / 1000);

  if (result == AVERROR_EOF && m_catchupTerminates && !m_isOpeningStream && !m_lastSeekWasLive &&
      m_previousLiveBufferOffset + m_defaultProgrammeDuration > static_cast<long long>(m_currentDemuxTime) / 1000)
    return true;

  return false;
}

void FFmpegCatchupStream::DemuxSetSpeed(int speed)
{
  Log(LOGLEVEL_INFO, "%s - DemuxSetSpeed %d", __FUNCTION__, speed);

  if (IsPaused() && speed != STREAM_PLAYSPEED_PAUSE)
  {
    // Resume Playback
    Log(LOGLEVEL_DEBUG, "%s - DemuxSetSpeed - Unpause time: %lld", __FUNCTION__, static_cast<long long>(m_pauseStartTime));
    m_lastSeekWasLive = false;
    DemuxSeekTime(m_pauseStartTime);
  }
  else if (!IsPaused() && speed == STREAM_PLAYSPEED_PAUSE)
  {
    // Pause Playback
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pauseStartTime = m_currentDemuxTime;
    Log(LOGLEVEL_DEBUG, "%s - DemuxSetSpeed - Pause time: %lld", __FUNCTION__, static_cast<long long>(m_pauseStartTime));
  }

  FFmpegStream::DemuxSetSpeed(speed);
}

void FFmpegCatchupStream::GetCapabilities(kodi::addon::InputstreamCapabilities& caps)
{
  Log(LOGLEVEL_DEBUG, "%s - Called", __FUNCTION__);
  caps.SetMask(INPUTSTREAM_SUPPORTS_IDEMUX |
    // INPUTSTREAM_SUPPORTS_IDISPLAYTIME |
    INPUTSTREAM_SUPPORTS_ITIME |
    // INPUTSTREAM_SUPPORTS_IPOSTIME |
    INPUTSTREAM_SUPPORTS_SEEK |
    INPUTSTREAM_SUPPORTS_PAUSE |
    INPUTSTREAM_SUPPORTS_ICHAPTER);
}

namespace
{

int GetGranularityCorrectionFromLive(long long bufferStartTimeSecs, long long bufferOffset, int granularitySecs)
{
  // We need to make sure we don't seek to a time within granularity seconds of live
  // as that will not be supported
  // Note: only valid for sources with a granularity more than 1 second (usually 60)

  int correction = 0;

  if (granularitySecs > 1)
  {
    const time_t timeNow = std::time(0);
    long long currentLiveOffset = timeNow - bufferStartTimeSecs;
    if (bufferOffset + granularitySecs > currentLiveOffset)
      correction = (bufferOffset + granularitySecs) - currentLiveOffset + 1;

    Log(LOGLEVEL_INFO, "%s - correction of %d seconds for live, granularity %d seconds, %lld seconds from live", __FUNCTION__, correction, granularitySecs, currentLiveOffset - bufferOffset);
  }

  return correction;
}

} // unnamed namespace

int64_t FFmpegCatchupStream::SeekCatchupStream(double timeMs, bool backwards)
{
  // The argument timeMs that is supplied will not be in the same units as our m_catchupBufferOffset
  // So we need to divide by 1000 to convert to seconds.
  // When we return the value we need to convert our seconds to microseconds so multiply by STREAM_TIME_BASE

  if (m_catchupBufferStartTime > 0)
  {
    long long liveBufferOffset = GetCurrentLiveOffset();

    if (m_isOpeningStream)
    {
      m_lastSeekWasLive = m_catchupBufferOffset >= liveBufferOffset - (VIDEO_PLAYER_BUFFER_SECONDS / 2); // (-5 seconds)

      if (m_catchupTerminates)
        m_previousLiveBufferOffset = liveBufferOffset;
    }
    else
    {
      int64_t seekBufferOffset = static_cast<int64_t>(timeMs);
      seekBufferOffset += 500;
      seekBufferOffset /= 1000;
      Log(LOGLEVEL_INFO, "%s - Seek offset: %lld - time: %s", __FUNCTION__, static_cast<long long>(seekBufferOffset), GetDateTime(m_catchupBufferStartTime + seekBufferOffset).c_str());

      if (!SeekDistanceSupported(seekBufferOffset))
        return -1;

      if (m_catchupGranularity > 1 && (m_lastSeekWasLive || m_seekCorrectsEOF))
        seekBufferOffset -= GetGranularityCorrectionFromLive(m_catchupBufferStartTime, seekBufferOffset, m_catchupGranularity);

      Log(LOGLEVEL_DEBUG, "%s - seekBufferOffset %lld < liveBufferOffset %lld -10", __FUNCTION__, static_cast<long long>(seekBufferOffset), liveBufferOffset);

      if (seekBufferOffset < liveBufferOffset - VIDEO_PLAYER_BUFFER_SECONDS) // (-10 seconds)
      {
        if (!TargetDistanceFromLiveSupported(liveBufferOffset - seekBufferOffset)) // terminating streams only
          return -1;

        Log(LOGLEVEL_INFO, "%s - Seek to catchup", __FUNCTION__);
        m_catchupBufferOffset = seekBufferOffset;
        m_lastSeekWasLive = false;

        if (m_seekCorrectsEOF)
          Log(LOGLEVEL_INFO, "%s - continuing stream %lld seconds from live at offset: %lld, live offset: %lld", __FUNCTION__, static_cast<long long>(liveBufferOffset - seekBufferOffset), static_cast<long long>(seekBufferOffset), static_cast<long long>(liveBufferOffset));
      }
      else
      {
        Log(LOGLEVEL_INFO, "%s - Seek to live", __FUNCTION__);
        m_catchupBufferOffset = liveBufferOffset;
        m_lastSeekWasLive = true;

        if (m_seekCorrectsEOF)
          Log(LOGLEVEL_INFO, "%s - Resetting continuing stream to live as within %lld seconds - crossed threshold of %d seconds", __FUNCTION__, static_cast<long long>(liveBufferOffset - seekBufferOffset), VIDEO_PLAYER_BUFFER_SECONDS);
      }

      if (m_catchupTerminates)
        m_previousLiveBufferOffset = liveBufferOffset;

      m_streamUrl = GetUpdatedCatchupUrl();
    }

    return static_cast<int64_t>(m_catchupBufferOffset) * STREAM_TIME_BASE;
  }

  return -1;
}

bool FFmpegCatchupStream::SeekDistanceSupported(int64_t seekBufferOffset)
{
  if (!m_seekCorrectsEOF)
  {
    long long currentDemuxSecs = static_cast<long long>(m_currentDemuxTime) / 1000;
    int seekDistanceSecs = std::llabs(seekBufferOffset - currentDemuxSecs);

    if (m_lastSeekWasLive &&
        ((seekDistanceSecs < VIDEO_PLAYER_BUFFER_SECONDS) ||
         (m_catchupTerminates && m_catchupGranularity == 1 && seekDistanceSecs < (TERMINATING_SECOND_STREAM_MIN_SEEK_FROM_LIVE_TIME - 5)) ||
         (m_catchupTerminates && m_catchupGranularity > 1 && seekDistanceSecs < (TERMINATING_MINUTE_STREAM_MIN_SEEK_FROM_LIVE_TIME - 5)) ||
         (!m_catchupTerminates && m_catchupGranularity > 1 && seekDistanceSecs < m_catchupGranularityLowWaterMark)))
    {
      Log(LOGLEVEL_INFO, "%s - skipping as seek distance of %d seconds is too short", __FUNCTION__, seekDistanceSecs);
      return false;
    }

    Log(LOGLEVEL_INFO, "%s - seek distance of %d seconds is ok", __FUNCTION__, seekDistanceSecs);
  }

  return true;
}

bool FFmpegCatchupStream::TargetDistanceFromLiveSupported(long long secondsFromLive)//, bool backwards)
{
  if (m_catchupTerminates && !m_seekCorrectsEOF)
  {
    if ((m_catchupGranularity == 1 && secondsFromLive < (TERMINATING_SECOND_STREAM_MIN_SEEK_FROM_LIVE_TIME - 5)) ||
        (m_catchupGranularity > 1 && secondsFromLive < (TERMINATING_MINUTE_STREAM_MIN_SEEK_FROM_LIVE_TIME - 5)))
    {
      Log(LOGLEVEL_INFO, "%s - skipping as %d seconds from live is too close", __FUNCTION__, secondsFromLive);
      return false;
    }

    Log(LOGLEVEL_INFO, "%s - %d seconds from live is ok", __FUNCTION__, secondsFromLive, secondsFromLive);
  }

  return true;
}

int64_t FFmpegCatchupStream::LengthStream()
{
  int64_t length = -1;
  if (m_catchupBufferStartTime > 0 && m_catchupBufferEndTime >= m_catchupBufferStartTime)
  {
    kodi::addon::InputstreamTimes times;
    if (GetTimes(times) && times.GetPtsEnd() >= times.GetPtsBegin())
      length = static_cast<int64_t>(times.GetPtsEnd() - times.GetPtsBegin());
  }

  Log(LOGLEVEL_DEBUG, "%s: %lld", __FUNCTION__, static_cast<long long>(length));

  return length;
}

bool FFmpegCatchupStream::GetTimes(kodi::addon::InputstreamTimes& times)
{
  if (m_catchupBufferStartTime == 0)
    return false;

  const time_t dateTimeNow = time(0);

  times.SetStartTime(m_catchupBufferStartTime);
  if (m_playbackAsLive)
    times.SetPtsEnd(static_cast<double>(dateTimeNow - times.GetStartTime()) * STREAM_TIME_BASE);
  else // it's like a video
    times.SetPtsEnd(static_cast<double>(std::min(dateTimeNow, m_catchupBufferEndTime) - times.GetStartTime()) * STREAM_TIME_BASE);

  Log(LOGLEVEL_DEBUG, "%s - startTime = %ld \tptsStart = %lld \tptsBegin = %lld \tptsEnd = %lld", __FUNCTION__,
            times.GetStartTime(), static_cast<long long>(times.GetPtsStart()), static_cast<long long>(times.GetPtsBegin()), static_cast<long long>(times.GetPtsEnd()));

  return true;
}

void FFmpegCatchupStream::UpdateCurrentPTS()
{
  FFmpegStream::UpdateCurrentPTS();
  if (m_currentPts != STREAM_NOPTS_VALUE)
    m_currentPts += m_seekOffset;
}

bool FFmpegCatchupStream::IsRealTimeStream()
{
  if (kodi::GetSettingBoolean("forceRealtimeOffCatchup"))
    return false;

  return m_isRealTimeStream && m_pFormatContext->duration <= 0;
}

namespace
{

void FormatUnits(const std::string& name, time_t tTime, std::string &urlFormatString)
{
  const std::regex timeSecondsRegex(".*(\\{" + name + ":(\\d+)\\}).*");
  std::cmatch mr;
  if (std::regex_match(urlFormatString.c_str(), mr, timeSecondsRegex) && mr.length() >= 3)
  {
    std::string timeSecondsExp = mr[1].first;
    std::string second = mr[1].second;
    if (second.length() > 0)
      timeSecondsExp = timeSecondsExp.erase(timeSecondsExp.find(second));
    std::string dividerStr = mr[2].first;
    second = mr[2].second;
    if (second.length() > 0)
      dividerStr = dividerStr.erase(dividerStr.find(second));

    const time_t divider = stoi(dividerStr);
    if (divider != 0)
    {
      time_t units = tTime / divider;
      if (units < 0)
        units = 0;
      urlFormatString.replace(urlFormatString.find(timeSecondsExp), timeSecondsExp.length(), std::to_string(units));
    }
  }
}

void FormatTime(const char ch, const struct tm *pTime, std::string &urlFormatString)
{
  std::string str = {'{', ch, '}'};
  size_t pos = urlFormatString.find(str);
  while (pos != std::string::npos)
  {
    std::ostringstream os;
    os << std::put_time(pTime, StringUtils::Format("%%%c", ch).c_str());
    std::string timeString = os.str();

    if (timeString.size() > 0)
      urlFormatString.replace(pos, str.size(), timeString);

    pos = urlFormatString.find(str);
  }
}

void FormatTime(const std::string name, const struct tm *pTime, std::string &urlFormatString, bool hasVarPrefix)
{
  std::string qualifier = hasVarPrefix ? "$" : "";
  qualifier += "{" + name + ":";
  size_t found = urlFormatString.find(qualifier);
  if (found != std::string::npos)
  {
    size_t foundStart = found + qualifier.size();
    size_t foundEnd = urlFormatString.find("}", foundStart + 1);
    if (foundEnd != std::string::npos)
    {
      std::string formatString = urlFormatString.substr(foundStart, foundEnd - foundStart);
      const std::regex timeSpecifiers("([YmdHMS])");
      formatString = std::regex_replace(formatString, timeSpecifiers, R"(%$&)");

      std::ostringstream os;
      os << std::put_time(pTime, formatString.c_str());
      std::string timeString = os.str();

      if (timeString.size() > 0)
        urlFormatString.replace(found, foundEnd - found + 1, timeString);
    }
  }
}

void FormatUtc(const std::string& str, time_t tTime, std::string &urlFormatString)
{
  auto pos = urlFormatString.find(str);
  if (pos != std::string::npos)
  {
    std::string utcTimeAsString = StringUtils::Format("%lu", tTime);
    urlFormatString.replace(pos, str.size(), utcTimeAsString);
  }
}

std::string FormatDateTime(time_t timeStart, time_t duration, const std::string &urlFormatString)
{
  std::string formattedUrl = urlFormatString;

  const time_t timeEnd = timeStart + duration;
  const time_t timeNow = std::time(0);

  std::tm dateTimeStart = SafeLocaltime(timeStart);
  std::tm dateTimeEnd = SafeLocaltime(timeEnd);
  std::tm dateTimeNow = SafeLocaltime(timeNow);

  FormatTime('Y', &dateTimeStart, formattedUrl);
  FormatTime('m', &dateTimeStart, formattedUrl);
  FormatTime('d', &dateTimeStart, formattedUrl);
  FormatTime('H', &dateTimeStart, formattedUrl);
  FormatTime('M', &dateTimeStart, formattedUrl);
  FormatTime('S', &dateTimeStart, formattedUrl);
  FormatUtc("{utc}", timeStart, formattedUrl);
  FormatUtc("${start}", timeStart, formattedUrl);
  FormatUtc("{utcend}", timeStart + duration, formattedUrl);
  FormatUtc("${end}", timeStart + duration, formattedUrl);
  FormatUtc("{lutc}", timeNow, formattedUrl);
  FormatUtc("${now}", timeNow, formattedUrl);
  FormatUtc("${timestamp}", timeNow, formattedUrl);
  FormatUtc("{duration}", duration, formattedUrl);
  FormatUnits("duration", duration, formattedUrl);
  FormatUtc("${offset}", timeNow - timeStart, formattedUrl);
  FormatUnits("offset", timeNow - timeStart, formattedUrl);

  FormatTime("utc", &dateTimeStart, formattedUrl, false);
  FormatTime("start", &dateTimeStart, formattedUrl, true);

  FormatTime("utcend", &dateTimeEnd, formattedUrl, false);
  FormatTime("end", &dateTimeEnd, formattedUrl, true);

  FormatTime("lutc", &dateTimeNow, formattedUrl, false);
  FormatTime("now", &dateTimeNow, formattedUrl, true);
  FormatTime("timestamp", &dateTimeNow, formattedUrl, true);

  Log(LOGLEVEL_DEBUG, "%s - \"%s\"", __FUNCTION__, CURL::GetRedacted(formattedUrl).c_str());

  return formattedUrl;
}

} // unnamed namespace

std::string FFmpegCatchupStream::GetUpdatedCatchupUrl() const
{
  time_t timeNow = time(0);
  time_t offset = m_catchupBufferStartTime + m_catchupBufferOffset;

  if (m_catchupBufferStartTime > 0 && offset < (timeNow - 5))
  {
    time_t duration = m_defaultProgrammeDuration;
    // use the programme duration if it's valid for the offset
    if (m_programmeStartTime > 0 && m_programmeStartTime < m_programmeEndTime &&
        m_programmeStartTime <= offset && m_programmeEndTime >= offset)
      duration = m_programmeEndTime - m_programmeStartTime;

    // cap duration to timeNow
    if (offset + duration > timeNow)
      duration = timeNow - offset;

    // if we have a different URL format to use when we are close to live
    // use if we are within 4 hours of a live stream
    std::string urlFormatString = m_catchupUrlFormatString;
    if (offset > (timeNow - m_defaultProgrammeDuration) && !m_catchupUrlNearLiveFormatString.empty())
      urlFormatString = m_catchupUrlNearLiveFormatString;

    Log(LOGLEVEL_DEBUG, "%s - Offset Time - \"%lld\" - %s", __FUNCTION__, static_cast<long long>(offset), CURL::GetRedacted(m_catchupUrlFormatString).c_str());

    std::string catchupUrl = FormatDateTime(offset - m_timezoneShift, duration, urlFormatString);

    static const std::regex CATCHUP_ID_REGEX("\\{catchup-id\\}");
    if (!m_programmeCatchupId.empty())
      catchupUrl = std::regex_replace(catchupUrl, CATCHUP_ID_REGEX, m_programmeCatchupId);

    if (!catchupUrl.empty())
    {
      Log(LOGLEVEL_DEBUG, "%s - Catchup URL: %s", __FUNCTION__, CURL::GetRedacted(catchupUrl).c_str());
      return catchupUrl;
    }
  }

  Log(LOGLEVEL_DEBUG, "%s - Default URL: %s", __FUNCTION__, CURL::GetRedacted(m_defaultUrl).c_str());
  return m_defaultUrl;
}
