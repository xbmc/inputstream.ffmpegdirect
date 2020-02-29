/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FFmpegStream.h"

class FFmpegCatchupStream : public FFmpegStream
{
public:
  FFmpegCatchupStream(IManageDemuxPacket* demuxPacketManager,
                      std::string& defaultUrl,
                      bool playbackAsLive,
                      time_t programmeStartTime,
                      time_t programmeEndTime,
                      std::string& catchupUrlFormatString,
                      std::string& catchupUrlNearLiveFormatString,
                      time_t catchupBufferStartTime,
                      time_t catchupBufferEndTime,
                      long long catchupBufferOffset,
                      int timezoneShift,
                      int defaultProgrammeDuration,
                      std::string& m_programmeCatchupId);
  ~FFmpegCatchupStream();

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) override;
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) override;
  virtual DemuxPacket* DemuxRead() override;
  virtual void GetCapabilities(INPUTSTREAM_CAPABILITIES& caps) override;

  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) override;
  virtual int64_t LengthStream() override;
  virtual bool GetTimes(INPUTSTREAM_TIMES& times) override;

  virtual bool CanPauseStream() override;
  virtual bool CanSeekStream() override;

protected:
  void UpdateCurrentPTS() override;

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
  int m_timezoneShift = 0;
  int m_defaultProgrammeDuration = 0;
  std::string m_programmeCatchupId;

  bool m_bIsOpening;
  double m_seekOffset;
};