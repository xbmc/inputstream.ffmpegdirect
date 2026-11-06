/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "TimeshiftStream.h"

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
  , m_terminalEof(props.m_terminalEof)
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
    if (Start())
      return true;
    else
      Close();
  }

  return false;
}

DEMUX_PACKET* TimeshiftStream::DemuxRead()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait_for(lock, std::chrono::milliseconds(10), [&] { return !m_running || m_timeshiftBuffer.HasPacketAvailable(); });

  DEMUX_PACKET* packet = m_timeshiftBuffer.ReadPacket();
  // An empty packet is EOF only if the source ended and the reader has caught
  // up; disk segments still pending or failed to load also read empty.
  if (m_sourceEnded && packet && packet->iSize == 0 && m_timeshiftBuffer.ReadAllWrittenPackets())
  {
    m_demuxPacketManager->FreeDemuxPacketFromInputStreamAPI(packet);
    return nullptr;
  }
  return packet;
}

bool TimeshiftStream::Start()
{
  if (m_running)
    return true;

  if (m_timeshiftBuffer.Start(GenerateStreamId(m_streamUrl)))
  {
    Log(LOGLEVEL_DEBUG, "%s - Timeshift: started", __FUNCTION__);
    m_sourceEnded = false;
    m_running = true;
    m_inputThread = std::thread([&] { DoReadWrite(); });

    return true;
  }

  Log(LOGLEVEL_DEBUG, "%s - Timeshift: failed to start", __FUNCTION__);
  return false;
}

void TimeshiftStream::Close()
{
  m_running = false;
  DemuxAbort();
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
    DEMUX_PACKET* pPacket = FFmpegStream::DemuxRead();
    if (pPacket)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_timeshiftBuffer.AddPacket(pPacket);
    }

    else if (m_lastPacketResult == AVERROR_EOF)
    {
      if (m_terminalEof)
      {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sourceEnded = true;
        m_condition.notify_all();
        break;
      }

      // A live HLS reload can hit FFmpeg's retry limit without the source
      // ending. Sleep so the retry is not a busy loop.
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    m_condition.notify_one();
  }
  Log(LOGLEVEL_DEBUG, "%s - Timeshift: stopped", __FUNCTION__);
  return;
}

void TimeshiftStream::GetCapabilities(kodi::addon::InputstreamCapabilities& caps)
{
  caps.SetMask(INPUTSTREAM_SUPPORTS_IDEMUX |
    INPUTSTREAM_SUPPORTS_ITIME |
    INPUTSTREAM_SUPPORTS_SEEK |
    INPUTSTREAM_SUPPORTS_PAUSE);
}

int64_t TimeshiftStream::LengthStream()
{
  int64_t length = -1;
  kodi::addon::InputstreamTimes times;
  if (GetTimes(times) && times.GetPtsEnd() >= times.GetPtsBegin())
    length = static_cast<int64_t>(times.GetPtsEnd() - times.GetPtsBegin());

  return length;
}

bool TimeshiftStream::GetTimes(kodi::addon::InputstreamTimes& times)
{
  // A finite source is read faster than it plays, so the buffered endpoint can
  // sit ahead of wall time; after the source ends it stops moving.
  int64_t endMilliseconds = m_timeshiftBuffer.GetBufferedMillisecondsSinceStart();
  if (!m_sourceEnded)
  {
    const int64_t wallMilliseconds = m_timeshiftBuffer.GetMillisecondsSinceStart();
    if (wallMilliseconds > endMilliseconds)
      endMilliseconds = wallMilliseconds;
  }

  times.SetStartTime(m_timeshiftBuffer.GetStartTimeSecs());
  times.SetPtsStart(0);
  times.SetPtsBegin(m_timeshiftBuffer.GetEarliestSegmentMillisecondsSinceStart() * 1000);
  times.SetPtsEnd(endMilliseconds * 1000);

  return true;
}

bool TimeshiftStream::IsRealTimeStream()
{
  return m_isRealTimeStream;
}

bool TimeshiftStream::DemuxSeekTime(double timeMs, bool backwards, double& startpts)
{
  return m_timeshiftBuffer.Seek(timeMs);
}

void TimeshiftStream::DemuxSetSpeed(int speed)
{
  Log(LOGLEVEL_DEBUG, "%s - DemuxSetSpeed %d", __FUNCTION__, speed);

  if (m_demuxSpeed == STREAM_PLAYSPEED_PAUSE && speed != STREAM_PLAYSPEED_PAUSE)
    m_timeshiftBuffer.SetPaused(false); // Resume Playback
  else if (m_demuxSpeed != STREAM_PLAYSPEED_PAUSE && speed == STREAM_PLAYSPEED_PAUSE)
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
