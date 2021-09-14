/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IManageDemuxPacket.h"
#include "TimeshiftSegment.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <kodi/addon-instance/Inputstream.h>

namespace ffmpegdirect
{

struct SegmentIndexOnDiskEntry
{
  int m_segmentId = -1;
  int m_timeIndexStart = -1;
  int m_timeIndexEnd = -1;
};

enum class SegmentIndexSearchBy
{
  SEGMENT_ID,
  TIME_INDEX
};

class TimeshiftBuffer
{
public:
  TimeshiftBuffer(IManageDemuxPacket* demuxPacketManager);
  ~TimeshiftBuffer();

  void AddPacket(DEMUX_PACKET* packet);
  DEMUX_PACKET* ReadPacket();
  bool Seek(double timeMs);
  void SetPaused(bool paused);

  bool Start(const std::string& streamId);

  time_t GetStartTimeSecs() { return m_startTime; }

  int GetSecondsSinceStart()
  {
    return std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::high_resolution_clock::now() - m_startedTimePoint).count();
  }

  int64_t GetMillisecondsSinceStart()
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::high_resolution_clock::now() - m_startedTimePoint).count();
  }

  int64_t GetEarliestSegmentMillisecondsSinceStart()
  {
    return m_minOnDiskSeekTimeIndex * 1000;
  }

  bool HasPacketAvailable()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_readSegment && m_readSegment->HasPacketAvailable();
  }

protected:
  IManageDemuxPacket* m_demuxPacketManager;

private:
  static const int TIMESHIFT_SEGMENT_LENGTH_SECS = 12;
  static const int SEGMENT_INDEX_FILE_LINE_LENGTH = 30;
  static const int TIMESHIFT_SEGMENT_IN_MEMORY_INDEXED_LENGTH_SECS = 60 * 12; // 12 minutes
  static const int MAX_IN_MEMORY_SEGMENT_INDEXES = TIMESHIFT_SEGMENT_IN_MEMORY_INDEXED_LENGTH_SECS / TIMESHIFT_SEGMENT_LENGTH_SECS + 1;
  static constexpr float DEFAULT_TIMESHIFT_SEGMENT_ON_DISK_LENGTH_HOURS = 1.0f;

  void RemoveOldestInMemoryAndOnDiskSegments();
  SegmentIndexOnDiskEntry SearchOnDiskIndex(const SegmentIndexSearchBy& segmentIndexSearchBy, int searchValue);

  int m_lastPacketSecondsSinceStart = 0;
  int m_lastSegmentSecondsSinceStart = 0;
  int m_minInMemorySeekTimeIndex = 0;
  int m_segmentInMemoryIndexOffset = 0;
  int m_minOnDiskSeekTimeIndex = 0;

  std::shared_ptr<TimeshiftSegment> m_firstSegment;
  std::shared_ptr<TimeshiftSegment> m_readSegment;
  std::shared_ptr<TimeshiftSegment> m_writeSegment;

  std::map<int, std::shared_ptr<TimeshiftSegment>> m_segmentTimeIndexMap;
  int m_currentSegmentIndex = 0;
  int m_earliestOnDiskSegmentId = 0;
  int m_segmentTotalCount = 0;

  std::chrono::high_resolution_clock::time_point m_startedTimePoint;
  time_t m_startTime;

  std::string m_streamId;

  bool m_readingInitialPackets = true;

  kodi::vfs::CFile m_segmentIndexFileHandle;

  std::string m_timeshiftBufferPath;
  std::string m_segmentIndexFilePath;

  std::mutex m_mutex;

  int m_currentDemuxTimeIndex;
  bool m_paused = false;

  bool m_enableOnDiskSegmentLimit = false;
  int m_maxOnDiskSegments;
};

} //namespace ffmpegdirect
