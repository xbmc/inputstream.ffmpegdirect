/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "TimeshiftBuffer.h"

#include "url/URL.h"
#include "../utils/DiskUtils.h"
#include "../utils/Log.h"

#include <kodi/tools/StringUtils.h>
#include <kodi/Filesystem.h>

using namespace ffmpegdirect;
using namespace kodi::tools;

TimeshiftBuffer::TimeshiftBuffer(IManageDemuxPacket* demuxPacketManager)
  : m_demuxPacketManager(demuxPacketManager)
{
  m_timeshiftBufferPath = kodi::GetSettingString("timeshiftBufferPath");
  if (m_timeshiftBufferPath.empty())
  {
    m_timeshiftBufferPath = DEFAULT_TIMESHIFT_BUFFER_PATH;
  }
  else
  {
    if (StringUtils::EndsWith(m_timeshiftBufferPath, "/") || StringUtils::EndsWith(m_timeshiftBufferPath, "\\"))
      m_timeshiftBufferPath.pop_back();
  }

  if (!kodi::vfs::DirectoryExists(m_timeshiftBufferPath))
    kodi::vfs::CreateDirectory(m_timeshiftBufferPath);

  if (!kodi::CheckSettingBoolean("timeshiftEnableLimit", m_enableOnDiskSegmentLimit))
    m_enableOnDiskSegmentLimit = true;
  float onDiskTotalLengthHours = kodi::GetSettingFloat("timeshiftOnDiskLength");
  if (onDiskTotalLengthHours <= 0.0f)
    onDiskTotalLengthHours = DEFAULT_TIMESHIFT_SEGMENT_ON_DISK_LENGTH_HOURS;
  int onDiskTotalLengthSeconds = onDiskTotalLengthHours * 60 * 60;

  if (m_enableOnDiskSegmentLimit)
    Log(LOGLEVEL_INFO, "%s - On disk length limit 'enabled', length limit set to %.2f hours", __FUNCTION__, onDiskTotalLengthHours);
  else
    Log(LOGLEVEL_INFO, "%s - On disk length limit 'disabled'", __FUNCTION__);

  m_maxOnDiskSegments = (onDiskTotalLengthSeconds / TIMESHIFT_SEGMENT_LENGTH_SECS) + 1;
}

TimeshiftBuffer::~TimeshiftBuffer()
{
  if (!m_streamId.empty())
  {
    //We need to make sure any filehandle is closed as you can't delete an open file on windows
    m_writeSegment->MarkAsComplete();
    for (int segmentId = m_earliestOnDiskSegmentId; segmentId <= m_writeSegment->GetSegmentId(); segmentId++)
    {
      std::string segmentFilename = StringUtils::Format("%s-%08d.seg", m_streamId.c_str(), segmentId);
      Log(LOGLEVEL_DEBUG, "%s - Deleting on disk segment - Segment ID: %d, Segment Filename: %s", __FUNCTION__, segmentId, segmentFilename.c_str());

      kodi::vfs::DeleteFile(m_timeshiftBufferPath + "/" + segmentFilename);
    }
  }

  m_segmentIndexFileHandle.Close();
  kodi::vfs::DeleteFile(m_segmentIndexFilePath);
}

bool TimeshiftBuffer::Start(const std::string& streamId)
{
  m_segmentIndexFilePath = m_timeshiftBufferPath + "/" + streamId + ".idx";
  // We need to pass the overwrite parameter as true as otherwise
  // opening on SMB for write on android will fail.
  if (!m_segmentIndexFileHandle.OpenFileForWrite(m_segmentIndexFilePath, true))
  {
    uint64_t freeSpaceMB = 0;
    if (DiskUtils::GetFreeDiskSpaceMB(m_timeshiftBufferPath, freeSpaceMB))
      Log(LOGLEVEL_ERROR, "%s - Failed to open segment index file on disk: %s, disk free space (MB): %lld", __FUNCTION__, CURL::GetRedacted(m_segmentIndexFilePath).c_str(), static_cast<long long>(freeSpaceMB));
    else
      Log(LOGLEVEL_ERROR, "%s - Failed to open segment index file on disk: %s, not possible to calculate free space", __FUNCTION__, CURL::GetRedacted(m_segmentIndexFilePath).c_str());
    return false;
  }

  m_streamId = streamId;

  m_startedTimePoint = std::chrono::high_resolution_clock::now();
  m_startTime = std::time(nullptr);

  m_firstSegment = std::make_shared<TimeshiftSegment>(m_demuxPacketManager, m_streamId, m_currentSegmentIndex, m_timeshiftBufferPath);
  m_writeSegment = m_firstSegment;
  m_segmentTimeIndexMap[0] = m_writeSegment;
  m_currentSegmentIndex++;
  m_segmentTotalCount++;
  m_readSegment = m_writeSegment;

  return true;
}

void TimeshiftBuffer::AddPacket(DEMUX_PACKET* packet)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  // Useful for debugging the initial set of packets in a stream
  if (m_readingInitialPackets)
  {
    Log(LOGLEVEL_DEBUG, "%s - Writing first segment - PTS: %f, DTA: %f, pts sec: %f, dts sec: %f", __FUNCTION__, packet->pts, packet->dts, packet->pts / STREAM_TIME_BASE, packet->dts / STREAM_TIME_BASE);

    // Note that this is a heuristic for a packet stream stabilising, unknown if it's true of all stream types
    if (packet->pts != STREAM_NOPTS_VALUE && packet->pts == packet->dts)
      m_readingInitialPackets = false;
  }

  int secondsSinceStart = 0;
  if (packet->pts != STREAM_NOPTS_VALUE && packet->pts > 0)
    secondsSinceStart = packet->pts / STREAM_TIME_BASE;

  if (secondsSinceStart - m_lastSegmentSecondsSinceStart >= TIMESHIFT_SEGMENT_LENGTH_SECS)
  {
    m_readingInitialPackets = false;

    if (secondsSinceStart != m_lastPacketSecondsSinceStart)
    {
      m_readingInitialPackets = false;

      std::shared_ptr<TimeshiftSegment> m_previousWriteSegment = m_writeSegment;
      m_previousWriteSegment->MarkAsComplete();

      Log(LOGLEVEL_DEBUG, "%s - Writing new segment - seconds: %d, last seg seconds: %d, last seg packet count: %d, new seg index: %d, pts %.2f, dts: %.2f, pts sec: %.0f, dts sec: %.0f",
                         __FUNCTION__, secondsSinceStart, m_lastSegmentSecondsSinceStart, m_previousWriteSegment->GetPacketCount(), m_currentSegmentIndex,
                         packet->pts, packet->dts, packet->pts / STREAM_TIME_BASE, packet->dts / STREAM_TIME_BASE);

      if (m_segmentIndexFileHandle.IsOpen())
      {
        std::string line = StringUtils::Format("%9d,%9d,%9d\n", m_previousWriteSegment->GetSegmentId(), m_lastSegmentSecondsSinceStart, secondsSinceStart); // 30 characters per line
        m_segmentIndexFileHandle.Write(line.c_str(), line.length());
      }

      if (m_segmentTimeIndexMap.size() > MAX_IN_MEMORY_SEGMENT_INDEXES)
        RemoveOldestInMemoryAndOnDiskSegments();

      m_writeSegment = std::make_shared<TimeshiftSegment>(m_demuxPacketManager, m_streamId, m_currentSegmentIndex, m_timeshiftBufferPath);
      m_previousWriteSegment->SetNextSegment(m_writeSegment);
      m_segmentTimeIndexMap[secondsSinceStart] = m_writeSegment;
      m_currentSegmentIndex++;
      m_segmentTotalCount++;
      m_lastSegmentSecondsSinceStart = secondsSinceStart;
    }
  }
  m_lastPacketSecondsSinceStart = secondsSinceStart;

  m_writeSegment->AddPacket(packet);
}

void TimeshiftBuffer::RemoveOldestInMemoryAndOnDiskSegments()
{
  std::shared_ptr<TimeshiftSegment> oldFirstSegment = m_firstSegment;

  m_firstSegment = oldFirstSegment->GetNextSegment();
  oldFirstSegment->SetNextSegment(nullptr);
  int timeToRemove = m_segmentTimeIndexMap.cbegin()->first;
  m_segmentTimeIndexMap.erase(timeToRemove);
  m_minInMemorySeekTimeIndex = m_segmentTimeIndexMap.cbegin()->first;

  Log(LOGLEVEL_DEBUG, "%s - Removed oldest in memory segment with ID: %d", __FUNCTION__, oldFirstSegment->GetSegmentId());
  Log(LOGLEVEL_DEBUG, "%s - Removed oldest on disk segment CHECK enabled: %d, paused: %d - segmentTotalCount: %d, maxOnDiskSegments: %d, currentDemuxTimeIndex: %d, minOnDiskSeekTimeIndex: %d", __FUNCTION__,
                      m_enableOnDiskSegmentLimit, m_paused, m_segmentTotalCount, m_maxOnDiskSegments, m_currentDemuxTimeIndex, m_minOnDiskSeekTimeIndex);

  if (m_enableOnDiskSegmentLimit && !m_paused &&
      m_segmentTotalCount > m_maxOnDiskSegments &&
      m_currentDemuxTimeIndex > m_minOnDiskSeekTimeIndex)
  {
    while (m_segmentTotalCount > m_maxOnDiskSegments && m_currentDemuxTimeIndex > m_minOnDiskSeekTimeIndex)
    {
      std::string segmentFilename = StringUtils::Format("%s-%08d.seg", m_streamId.c_str(), m_earliestOnDiskSegmentId);
      if (kodi::vfs::FileExists(m_timeshiftBufferPath + "/" + segmentFilename))
      {
        kodi::vfs::DeleteFile(m_timeshiftBufferPath + "/" + segmentFilename);
        Log(LOGLEVEL_DEBUG, "%s - Removed oldest on disk segment with ID: %d - currentDemuxTimeSeconds: %d, min on disk time: %d", __FUNCTION__, m_earliestOnDiskSegmentId, m_currentDemuxTimeIndex, m_minOnDiskSeekTimeIndex);
        m_earliestOnDiskSegmentId++;
        m_segmentTotalCount--;

        SegmentIndexOnDiskEntry indexEntry = SearchOnDiskIndex(SegmentIndexSearchBy::SEGMENT_ID, m_earliestOnDiskSegmentId);

        if (indexEntry.m_segmentId >= 0)
          m_minOnDiskSeekTimeIndex = indexEntry.m_timeIndexStart;
      }
    }
  }
}

DEMUX_PACKET* TimeshiftBuffer::ReadPacket()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  DEMUX_PACKET* packet = nullptr;

  if (m_readSegment)
  {
    m_readSegment->LoadSegment();

    packet = m_readSegment->ReadPacket();

    if (!m_readSegment->HasPacketAvailable() && m_readSegment->ReadAllPackets())
    {
      std::shared_ptr<TimeshiftSegment> m_previousReadSegment = m_readSegment;

      m_readSegment = m_readSegment->GetNextSegment();
      if (!m_readSegment) // We need to load the next read segment from disk as it doesn't exist in memory
      {
        m_readSegment = std::make_shared<TimeshiftSegment>(m_demuxPacketManager, m_streamId, m_previousReadSegment->GetSegmentId() + 1, m_timeshiftBufferPath);
        m_readSegment->ForceLoadSegment();
      }
      m_readSegment->ResetReadIndex();

      m_previousReadSegment->ClearPackets();
      if (m_readSegment)
        Log(LOGLEVEL_DEBUG, "%s - Reading next segment with id: %d, packet count: %d", __FUNCTION__, m_readSegment->GetSegmentId(), m_readSegment->GetPacketCount());
    }

    if (packet && packet->pts != STREAM_NOPTS_VALUE && packet->pts > 0)
      m_currentDemuxTimeIndex = packet->pts / STREAM_TIME_BASE;
  }
  else
  {
    packet = m_demuxPacketManager->AllocateDemuxPacketFromInputStreamAPI(0);
  }

  return packet;
}

bool TimeshiftBuffer::Seek(double timeMs)
{
  int seekSeconds = timeMs / 1000;
  std::lock_guard<std::mutex> lock(m_mutex);

  if (seekSeconds < 0)
      seekSeconds = m_minOnDiskSeekTimeIndex;

  if (seekSeconds >= m_minInMemorySeekTimeIndex)
  {
    auto seekSegmentIndex = m_segmentTimeIndexMap.upper_bound(seekSeconds);
    // Upper bound gets the segment after the one we want
    if (seekSegmentIndex != m_segmentTimeIndexMap.begin())
      --seekSegmentIndex;

    if (seekSegmentIndex != m_segmentTimeIndexMap.end())
      m_readSegment = seekSegmentIndex->second;
    else // Jump to live segment
      m_readSegment = m_segmentTimeIndexMap.rbegin()->second;

    Log(LOGLEVEL_DEBUG, "%s - Buffer - SegmentID: %d, SeekSeconds: %d", __FUNCTION__, m_readSegment->GetSegmentId(), seekSeconds);

    m_readSegment->LoadSegment();
    if (m_readSegment->Seek(timeMs))
      return true;
  }
  else // We need to find the segment in the index file as it's not in memory
  {
    SegmentIndexOnDiskEntry indexEntry = SearchOnDiskIndex(SegmentIndexSearchBy::TIME_INDEX, seekSeconds);

    if (indexEntry.m_segmentId >= 0)
    {
      std::string segmentFilename = StringUtils::Format("%s-%08d.seg", m_streamId.c_str(), indexEntry.m_segmentId);

      if (kodi::vfs::FileExists(m_timeshiftBufferPath + "/" + segmentFilename))
      {
        m_readSegment = std::make_shared<TimeshiftSegment>(m_demuxPacketManager, m_streamId, indexEntry.m_segmentId, m_timeshiftBufferPath);
        m_readSegment->ForceLoadSegment();
        return true;
      }
    }
  }

  return false;
}

void TimeshiftBuffer::SetPaused(bool paused)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (paused)
  {
    // If the read segment is not in memory clear it's next pointer so stays only on disk
    // Otherwise the the shared pointer will stay referenced.
    if (m_readSegment->GetSegmentId() < m_firstSegment->GetSegmentId())
      m_readSegment->SetNextSegment(nullptr);
  }

  Log(LOGLEVEL_INFO, "%s - Stream %s - time seconds: %d", __FUNCTION__, paused ? "paused" : "resumed", m_currentDemuxTimeIndex);

  m_paused = paused;
}

SegmentIndexOnDiskEntry TimeshiftBuffer::SearchOnDiskIndex(const SegmentIndexSearchBy& segmentIndexSearchBy, int searchValue)
{
  SegmentIndexOnDiskEntry entry;

  int seekStart = 0;
  // Here we calulate an exact line number for segment id search
  // and a definitely below line number for time index searches
  if (segmentIndexSearchBy == SegmentIndexSearchBy::SEGMENT_ID)
    seekStart = searchValue * SEGMENT_INDEX_FILE_LINE_LENGTH;
  else if (segmentIndexSearchBy == SegmentIndexSearchBy::TIME_INDEX)
    seekStart = searchValue / TIMESHIFT_SEGMENT_LENGTH_SECS;

  kodi::vfs::CFile readFileHandle;
  if (readFileHandle.OpenFile(m_segmentIndexFilePath, ADDON_READ_NO_CACHE))
  {
    readFileHandle.Seek(seekStart);

    std::string line;
    bool foundSegmentOnDisk = false;
    while (readFileHandle.ReadLine(line))
    {
      const auto& values = StringUtils::Split(line, ",");

      if (values.size() == 3)
      {
        int segmentId = std::atoi(values[0].c_str());
        int timeIndexStart = std::atoi(values[1].c_str());
        int timeIndexEnd = std::atoi(values[2].c_str());

        if ((segmentIndexSearchBy == SegmentIndexSearchBy::SEGMENT_ID && searchValue == segmentId) ||
            (segmentIndexSearchBy == SegmentIndexSearchBy::TIME_INDEX && searchValue >= timeIndexStart && searchValue < timeIndexEnd))
        {
          entry.m_segmentId = segmentId;
          entry.m_timeIndexStart = timeIndexStart;
          entry.m_timeIndexEnd = timeIndexEnd;
          break;
        }
      }
    }

    readFileHandle.Close();
  }

  return entry;
}
