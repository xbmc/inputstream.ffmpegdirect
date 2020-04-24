/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "FFmpegStream.h"
#include "../utils/HttpProxy.h"

static const int VIDEO_PLAYER_BUFFER_SECONDS = 10;

class FFmpegCatchupStream : public FFmpegStream
{
public:
  FFmpegCatchupStream(IManageDemuxPacket* demuxPacketManager,
                      const ffmpegdirect::utils::HttpProxy& httpProxy,
                      std::string& defaultUrl,
                      bool playbackAsLive,
                      time_t programmeStartTime,
                      time_t programmeEndTime,
                      std::string& catchupUrlFormatString,
                      std::string& catchupUrlNearLiveFormatString,
                      time_t catchupBufferStartTime,
                      time_t catchupBufferEndTime,
                      long long catchupBufferOffset,
                      bool catchupTerminates,
                      int catchupGranularity,
                      int timezoneShift,
                      int defaultProgrammeDuration,
                      std::string& m_programmeCatchupId);
  ~FFmpegCatchupStream();

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) override;
  virtual bool DemuxSeekTime(double timeMs, bool backwards, double& startpts) override;
  virtual DemuxPacket* DemuxRead() override;
  virtual void DemuxSetSpeed(int speed) override;
  virtual void GetCapabilities(INPUTSTREAM_CAPABILITIES& caps) override;
  bool DemuxSeekTime(double timeMs)
  {
    double temp = 0;
    return DemuxSeekTime(timeMs, false, temp);
  }

  int64_t SeekCatchupStream(double timeMs, int whence);
  virtual int64_t LengthStream() override;
  virtual bool GetTimes(INPUTSTREAM_TIMES& times) override;

protected:
  void UpdateCurrentPTS() override;
  bool CheckReturnEmptryOnPacketResult(int result) override;

  long long GetCurrentLiveOffset() { return std::time(nullptr) - m_catchupBufferStartTime; }

  std::string GetUpdatedCatchupUrl() const;

  bool m_playbackAsLive = false;
  std::string m_defaultUrl;
  time_t m_programmeStartTime = 0;
  time_t m_programmeEndTime = 0;
  std::string m_catchupUrlFormatString;
  std::string m_catchupUrlNearLiveFormatString;
  time_t m_catchupBufferStartTime = 0;
  time_t m_catchupBufferEndTime = 0;
  long long m_catchupBufferOffset = 0;
  bool m_catchupTerminates = false;
  int m_catchupGranularity = 1;
  int m_timezoneShift = 0;
  int m_defaultProgrammeDuration = 0;
  std::string m_programmeCatchupId;

  bool m_isOpeningStream;
  double m_seekOffset;
  double m_pauseStartTime;
  double m_currentDemuxTime;

  long long m_previousLiveBufferOffset = 0;
  bool m_lastSeekWasLive = false;
  bool m_lastPacketWasAvoidedEOF = false;
  bool m_seekCorrectsEOF = false;
};