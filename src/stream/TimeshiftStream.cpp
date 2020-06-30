/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "TimeshiftStream.h"

#include "threads/SingleLock.h"
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

using namespace ffmpegdirect;

TimeshiftStream::TimeshiftStream(IManageDemuxPacket* demuxPacketManager,
                                 const Properties& props,
                                 const HttpProxy& httpProxy)
  : FFmpegStream(demuxPacketManager, props, httpProxy)
{
  std::random_device randomDevice; //Will be used to obtain a seed for the random number engine
  m_randomGenerator = std::mt19937(randomDevice()); //Standard mersenne_twister_engine seeded with randomDevice()
  m_randomDistribution = std::uniform_int_distribution<>(0, 1000);
}

TimeshiftStream::~TimeshiftStream()
{

}

bool TimeshiftStream::Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty)
{
  if (FFmpegStream::Open(streamUrl, mimeType, isRealTimeStream, programProperty))
  {
    Start();

    return true;
  }

  return false;
}

DemuxPacket* TimeshiftStream::DemuxRead()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait_for(lock, std::chrono::milliseconds(10), [&] { return m_timeshiftBuffer.HasPacketAvailable(); });

  return m_timeshiftBuffer.ReadPacket();
}

bool TimeshiftStream::Start()
{
  if (m_running)
    return true;

  Log(LOGLEVEL_DEBUG, "%s - Timeshift: started", __FUNCTION__);
  m_timeshiftBuffer.Start(GenerateStreamId(m_streamUrl));
  m_running = true;
  m_inputThread = std::thread([&] { DoReadWrite(); });

  return true;
}

void TimeshiftStream::Close()
{
  m_running = false;
  if (m_inputThread.joinable())
    m_inputThread.join();

  FFmpegStream::Close();

  Log(LOGLEVEL_DEBUG, "%s - Timeshift: closed", __FUNCTION__);
}

void TimeshiftStream::DoReadWrite()
{
  Log(LOGLEVEL_DEBUG, "%s - Timeshift: started", __FUNCTION__);
  while (m_running)
  {
    DemuxPacket* pPacket = FFmpegStream::DemuxRead();
    if (pPacket)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_timeshiftBuffer.AddPacket(pPacket);
    }

    m_condition.notify_one();
  }
  Log(LOGLEVEL_DEBUG, "%s - Timeshift: stopped", __FUNCTION__);
  return;
}

void TimeshiftStream::GetCapabilities(INPUTSTREAM_CAPABILITIES &caps)
{
  caps.m_mask = INPUTSTREAM_CAPABILITIES::SUPPORTS_IDEMUX |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_ITIME |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_SEEK |
    INPUTSTREAM_CAPABILITIES::SUPPORTS_PAUSE;
}

int64_t TimeshiftStream::LengthStream()
{
  int64_t length = -1;
  INPUTSTREAM_TIMES times = {0};
  if (GetTimes(times) && times.ptsEnd >= times.ptsBegin)
    length = static_cast<int64_t>(times.ptsEnd - times.ptsBegin);

  return length;
}

bool TimeshiftStream::GetTimes(INPUTSTREAM_TIMES& times)
{
  times = {0};

  times.startTime = m_timeshiftBuffer.GetStartTimeSecs();
  times.ptsStart = 0;
  times.ptsBegin = m_timeshiftBuffer.GetEarliestSegmentMillisecondsSinceStart() * 1000;
  times.ptsEnd = m_timeshiftBuffer.GetMillisecondsSinceStart() * 1000;

  return true;
}

bool TimeshiftStream::IsRealTimeStream()
{
  return true;
}

bool TimeshiftStream::DemuxSeekTime(double timeMs, bool backwards, double& startpts)
{
  if (timeMs < 0)
    return false;

  return m_timeshiftBuffer.Seek(timeMs);
}

void TimeshiftStream::DemuxSetSpeed(int speed)
{
  Log(LOGLEVEL_DEBUG, "%s - DemuxSetSpeed %d", __FUNCTION__, speed);

  if (m_demuxSpeed == DVD_PLAYSPEED_PAUSE && speed != DVD_PLAYSPEED_PAUSE)
    m_timeshiftBuffer.SetPaused(false); // Resume Playback
  else if (m_demuxSpeed != DVD_PLAYSPEED_PAUSE && speed == DVD_PLAYSPEED_PAUSE)
    m_timeshiftBuffer.SetPaused(true); // Pause Playback

  m_demuxSpeed = speed;
}

std::string TimeshiftStream::GenerateStreamId(const std::string streamUrl)
{
  std::string sourceString;
  sourceString.append(streamUrl);

  sourceString += "-" + std::to_string(m_randomDistribution(m_randomGenerator));

  const char* calcString = sourceString.c_str();
  int iId = 0;
  int c;
  while ((c = *calcString++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */

  return std::to_string(abs(iId));
}