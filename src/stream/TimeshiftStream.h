/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "../utils/HttpProxy.h"
#include "../utils/Properties.h"
#include "FFmpegStream.h"
#include "TimeshiftBuffer.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <random>
#include <string>
#include <thread>

namespace ffmpegdirect
{

class TimeshiftStream
  : public FFmpegStream
{
public:
  TimeshiftStream(IManageDemuxPacket* demuxPacketManager,
                  const Properties& props,
                  const HttpProxy& httpProxy);
  ~TimeshiftStream();

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) override;
  virtual void Close() override;
  virtual void GetCapabilities(kodi::addon::InputstreamCapabilities& caps) override;

  virtual DEMUX_PACKET* DemuxRead() override;
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) override;
  virtual void DemuxSetSpeed(int speed) override;

  virtual bool GetTimes(kodi::addon::InputstreamTimes& times) override;

  virtual int64_t LengthStream() override;
  virtual bool IsRealTimeStream() override;

private:
  void DoReadWrite();
  bool Start();
  std::string GenerateStreamId(const std::string streamUrl);

  std::mt19937 m_randomGenerator;
  std::uniform_int_distribution<> m_randomDistribution;

  std::atomic<bool> m_running = {false};
  std::thread m_inputThread;
  std::condition_variable m_condition;
  std::mutex m_mutex;

  double m_demuxSpeed = STREAM_PLAYSPEED_NORMAL;

  TimeshiftBuffer m_timeshiftBuffer{m_demuxPacketManager};
};

} //namespace ffmpegdirect
