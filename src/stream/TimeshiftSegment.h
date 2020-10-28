/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "IManageDemuxPacket.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <kodi/Filesystem.h>

#include <kodi/addon-instance/Inputstream.h>

namespace ffmpegdirect
{

static const std::string DEFAULT_TIMESHIFT_BUFFER_PATH = "special://userdata/addon_data/inputstream.ffmpegdirect/timeshift";

class TimeshiftSegment
{
public:
  TimeshiftSegment(IManageDemuxPacket* demuxPacketManager, const std::string& streamId, int segmentId, const std::string& timeshiftBufferPath);
  ~TimeshiftSegment();

  void AddPacket(DEMUX_PACKET* packet);
  DEMUX_PACKET* ReadPacket();
  bool Seek(double timeMs);

  int GetPacketCount();
  void MarkAsComplete();
  bool HasPacketAvailable();
  bool ReadAllPackets();
  void SetNextSegment(std::shared_ptr<TimeshiftSegment> nextSegment);
  std::shared_ptr<TimeshiftSegment> GetNextSegment();
  void ResetReadIndex();
  int GetReadIndex();
  int GetSegmentId();
  void ClearPackets();
  void ForceLoadSegment();
  void LoadSegment();

protected:
  IManageDemuxPacket* m_demuxPacketManager;

private:
  void CopyPacket(DEMUX_PACKET* sourcePacket, DEMUX_PACKET* newPacket, bool allocateData);
  void CopySideData(DEMUX_PACKET *sourcePacket, DEMUX_PACKET* newPacket);
  void FreeSideData(std::shared_ptr<DEMUX_PACKET>& packet);
  void WritePacket(std::shared_ptr<DEMUX_PACKET>& packet);
  int LoadPacket(std::shared_ptr<DEMUX_PACKET>& packet);

  std::shared_ptr<TimeshiftSegment> m_nextSegment;

  int32_t m_currentPacketIndex = 0;
  int m_readPacketIndex = 0;
  int m_lastPacketSecondsSinceStart = 0;

  std::vector<std::shared_ptr<DEMUX_PACKET>> m_packetBuffer;
  std::map<int, int> m_packetTimeIndexMap;

  bool m_completed = false;
  bool m_persisted = false;
  bool m_loaded = true;
  bool m_persistSegments = true;

  int m_segmentId;

  std::string m_streamId;
  std::string m_segmentFilename;

  kodi::vfs::CFile m_fileHandle;

  std::string m_timeshiftSegmentFilePath;

  std::mutex m_mutex;
};

} //namespace ffmpegdirect