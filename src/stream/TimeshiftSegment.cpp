/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "TimeshiftSegment.h"

#include "../utils/Log.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <kodi/DemuxCrypto.h>
#include <p8-platform/util/StringUtils.h>

using namespace ffmpegdirect;

TimeshiftSegment::TimeshiftSegment(IManageDemuxPacket* demuxPacketManager, const std::string& streamId, int segmentId, const std::string& timeshiftBufferPath)
  : m_demuxPacketManager(demuxPacketManager), m_streamId(streamId), m_segmentId(segmentId)
{
  m_segmentFilename = StringUtils::Format("%s-%08d.seg", streamId.c_str(), segmentId);
  Log(LOGLEVEL_DEBUG, "%s - Segment ID: %d, Segment Filename: %s", __FUNCTION__, segmentId, m_segmentFilename.c_str());

  m_timeshiftSegmentFilePath = timeshiftBufferPath + "/" + m_segmentFilename;

  // Only open the file for writing if it doesn't exist
  // If it does exist then this segment is being created to
  // to load an out of memory segment for a seek operation
  if (!kodi::vfs::FileExists(m_timeshiftSegmentFilePath))
  {
    if (m_fileHandle.OpenFileForWrite(m_timeshiftSegmentFilePath))
    {
      int32_t packetCountPlaceholder = 0;
      m_fileHandle.Write(&packetCountPlaceholder, sizeof(packetCountPlaceholder));
    }
    else
    {
      Log(LOGLEVEL_ERROR, "%s - Failed to open segment file on disk: %s", __FUNCTION__, m_timeshiftSegmentFilePath.c_str());
      m_persistSegments = false;
    }
  }
}

TimeshiftSegment::~TimeshiftSegment()
{
  m_fileHandle.Close();

  for (auto& demuxPacket : m_packetBuffer)
  {
    delete[] demuxPacket->pData;
    FreeSideData(demuxPacket);
  }
}

void TimeshiftSegment::AddPacket(DemuxPacket* packet)
{
  std::shared_ptr<DemuxPacket> newPacket = std::make_shared<DemuxPacket>();

  CopyPacket(packet, newPacket.get(), true);

  m_demuxPacketManager->FreeDemuxPacketFromInputStreamAPI(packet);

  std::lock_guard<std::mutex> lock(m_mutex);

  //Checksum
  if (m_persistSegments)
  {
    m_fileHandle.Write(&m_currentPacketIndex, sizeof(m_currentPacketIndex));
    WritePacket(newPacket);
  }

  m_packetBuffer.emplace_back(newPacket);

  int secondsSinceStart = 0;
  if (packet->pts != DVD_NOPTS_VALUE && packet->pts > 0)
    secondsSinceStart = packet->pts / DVD_TIME_BASE;

  if (secondsSinceStart != m_lastPacketSecondsSinceStart)
  {
    m_packetTimeIndexMap[secondsSinceStart] = m_currentPacketIndex;
    m_lastPacketSecondsSinceStart = secondsSinceStart;
  }

  m_currentPacketIndex++;
}

void TimeshiftSegment::CopyPacket(DemuxPacket* sourcePacket, DemuxPacket* newPacket, bool allocateData)
{
  // Note that this is not needed if allocating in kodi
  if (allocateData)
    newPacket->pData = new uint8_t[sourcePacket->iSize];

  newPacket->iSize = sourcePacket->iSize;
  if (newPacket->iSize)
    memcpy(newPacket->pData, sourcePacket->pData, newPacket->iSize);
  newPacket->iStreamId = sourcePacket->iStreamId;
  newPacket->demuxerId = sourcePacket->demuxerId;
  newPacket->iGroupId = sourcePacket->iGroupId;

  CopySideData(sourcePacket, newPacket);

  newPacket->pts = sourcePacket->pts;
  newPacket->dts = sourcePacket->dts;
  newPacket->duration = sourcePacket->duration;
  newPacket->dispTime = sourcePacket->dispTime;
  newPacket->recoveryPoint = sourcePacket->recoveryPoint;
  newPacket->cryptoInfo = sourcePacket->cryptoInfo;
}

void TimeshiftSegment::CopySideData(DemuxPacket* sourcePacket, DemuxPacket* newPacket)
{
  newPacket->pSideData = nullptr;
  newPacket->iSideDataElems = 0;
  if (sourcePacket->iSideDataElems > 0)
  {
    AVPacket srcAvPacket;
    av_init_packet(&srcAvPacket);
    srcAvPacket.side_data = static_cast<AVPacketSideData*>(sourcePacket->pSideData);
    srcAvPacket.side_data_elems = sourcePacket->iSideDataElems;

    AVPacket newAvPacket;
    av_init_packet(&newAvPacket);
    av_packet_copy_props(&newAvPacket, &srcAvPacket);
    newPacket->pSideData = newAvPacket.side_data;
    newPacket->iSideDataElems = newAvPacket.side_data_elems;
  }
}

void TimeshiftSegment::FreeSideData(std::shared_ptr<DemuxPacket>& packet)
{
  if (packet->iSideDataElems > 0)
  {
    AVPacket avPacket;
    av_init_packet(&avPacket);
    avPacket.side_data = (AVPacketSideData*) packet->pSideData;
    avPacket.side_data_elems = packet->iSideDataElems;
    av_packet_unref(&avPacket);
  }
}

void TimeshiftSegment::WritePacket(std::shared_ptr<DemuxPacket>& packet)
{
  m_fileHandle.Write(&packet->iSize, sizeof(packet->iSize));
  if (packet->iSize > 0)
    m_fileHandle.Write(packet->pData, packet->iSize);

  m_fileHandle.Write(&packet->iStreamId, sizeof(packet->iStreamId));
  m_fileHandle.Write(&packet->demuxerId, sizeof(packet->demuxerId));
  m_fileHandle.Write(&packet->iGroupId, sizeof(packet->iGroupId));

  m_fileHandle.Write(&packet->iSideDataElems, sizeof(packet->iSideDataElems));

  if (packet->iSideDataElems > 0)
  {
    AVPacketSideData* sideData = static_cast<AVPacketSideData*>(packet->pSideData);
    for (int i = 0; i < packet->iSideDataElems; i++)
    {
      m_fileHandle.Write(&sideData[i].type, sizeof(sideData[i].type));
      m_fileHandle.Write(&sideData[i].size, sizeof(sideData[i].size));
      if (sideData[i].size > 0)
        m_fileHandle.Write(sideData[i].data, sideData[i].size);
    }
  }

  m_fileHandle.Write(&packet->pts, sizeof(packet->pts));
  m_fileHandle.Write(&packet->dts, sizeof(packet->dts));
  m_fileHandle.Write(&packet->duration, sizeof(packet->duration));
  m_fileHandle.Write(&packet->recoveryPoint, sizeof(packet->recoveryPoint));

  bool hasCryptoInfo = packet->cryptoInfo != nullptr;
  m_fileHandle.Write(&hasCryptoInfo, sizeof(hasCryptoInfo));
  if (hasCryptoInfo)
  {
    int numSubSamples = packet->cryptoInfo->numSubSamples;
    m_fileHandle.Write(&numSubSamples, sizeof(numSubSamples));
    m_fileHandle.Write(&packet->cryptoInfo->flags, sizeof(packet->cryptoInfo->flags));
    if (numSubSamples > 0)
    {
      m_fileHandle.Write(packet->cryptoInfo->clearBytes, sizeof(uint16_t) * numSubSamples);
      m_fileHandle.Write(packet->cryptoInfo->cipherBytes, sizeof(uint32_t) * numSubSamples);
    }
    m_fileHandle.Write(packet->cryptoInfo->iv, sizeof(uint8_t) * 16);
    m_fileHandle.Write(packet->cryptoInfo->kid, sizeof(uint8_t) * 16);
  }
}

void TimeshiftSegment::ForceLoadSegment()
{
  m_loaded = false;
  LoadSegment();
}

void TimeshiftSegment::LoadSegment()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_loaded && m_fileHandle.OpenFile(m_timeshiftSegmentFilePath, ADDON_READ_NO_CACHE))
  {
    int32_t packetCount;
    m_fileHandle.Read(&packetCount, sizeof(packetCount));

    for (int i = 0; i < packetCount; i++)
    {
      std::shared_ptr<DemuxPacket> newPacket = std::make_shared<DemuxPacket>();
      int loadedPacketIndex = LoadPacket(newPacket);
      // Checksum does not match
      if (loadedPacketIndex != i)
        Log(LOGLEVEL_ERROR, "%s - segment load error, packet index %d does not equal expected value of %d with a total packet count of: %d", __FUNCTION__, loadedPacketIndex, i, m_currentPacketIndex);
      m_packetBuffer.emplace_back(newPacket);
    }

    m_currentPacketIndex = packetCount;
    m_persisted = true;
    m_completed = true;

    m_loaded = true;
  }
}

int TimeshiftSegment::LoadPacket(std::shared_ptr<DemuxPacket>& packet)
{
  //Checksum
  int packetIndex;
  m_fileHandle.Read(&packetIndex, sizeof(packetIndex));

  m_fileHandle.Read(&packet->iSize, sizeof(packet->iSize));
  if (packet->iSize > 0)
  {
    packet->pData = new uint8_t[packet->iSize];
    m_fileHandle.Read(packet->pData, packet->iSize);
  }

  m_fileHandle.Read(&packet->iStreamId, sizeof(packet->iStreamId));
  m_fileHandle.Read(&packet->demuxerId, sizeof(packet->demuxerId));
  m_fileHandle.Read(&packet->iGroupId, sizeof(packet->iGroupId));

  m_fileHandle.Read(&packet->iSideDataElems, sizeof(packet->iSideDataElems));
  if (packet->iSideDataElems > 0)
  {
    AVPacket avPacket;
    av_init_packet(&avPacket);
    enum AVPacketSideDataType type;
    int size;
    for (int i = 0; i < packet->iSideDataElems; i++)
    {
      m_fileHandle.Read(&type, sizeof(type));
      m_fileHandle.Read(&size, sizeof(size));

      uint8_t* data = av_packet_new_side_data(&avPacket, type, size);
      m_fileHandle.Read(data, size);
    }

    packet->pSideData = avPacket.side_data;
  }

  m_fileHandle.Read(&packet->pts, sizeof(packet->pts));
  m_fileHandle.Read(&packet->dts, sizeof(packet->dts));
  m_fileHandle.Read(&packet->duration, sizeof(packet->duration));
  m_fileHandle.Read(&packet->recoveryPoint, sizeof(packet->recoveryPoint));

  bool hasCryptoInfo;
  m_fileHandle.Read(&hasCryptoInfo, sizeof(hasCryptoInfo));
  if (hasCryptoInfo)
  {
    int numSubSamples;
    m_fileHandle.Read(&numSubSamples, sizeof(numSubSamples));

    packet->cryptoInfo = std::make_shared<DemuxCryptoInfo>(numSubSamples);
    m_fileHandle.Read(&packet->cryptoInfo->flags, sizeof(packet->cryptoInfo->flags));
    if (numSubSamples > 0)
    {
      m_fileHandle.Read(packet->cryptoInfo->clearBytes, sizeof(uint16_t) * numSubSamples);
      m_fileHandle.Read(packet->cryptoInfo->cipherBytes, sizeof(uint32_t) * numSubSamples);
    }
    m_fileHandle.Read(packet->cryptoInfo->iv, sizeof(uint8_t) * 16);
    m_fileHandle.Read(packet->cryptoInfo->kid, sizeof(uint8_t) * 16);
  }

  return packetIndex;
}

int TimeshiftSegment::GetPacketCount()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_currentPacketIndex;
}

void TimeshiftSegment::MarkAsComplete()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_fileHandle.IsOpen())
  {
    m_fileHandle.Seek(0);
    m_fileHandle.Write(&m_currentPacketIndex, sizeof(m_currentPacketIndex));
  }

  m_completed = true;
  m_fileHandle.Close();
  m_persisted = true;
}

void TimeshiftSegment::ClearPackets()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  int m_readPacketIndex = 0;

  for (auto& demuxPacket : m_packetBuffer)
  {
    delete[] demuxPacket->pData;
    FreeSideData(demuxPacket);
  }

  m_packetBuffer.clear();
  m_loaded = false;
}

bool TimeshiftSegment::ReadAllPackets()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_completed && m_readPacketIndex == m_packetBuffer.size();
}

bool TimeshiftSegment::HasPacketAvailable()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_readPacketIndex != m_packetBuffer.size();
}

void TimeshiftSegment::SetNextSegment(std::shared_ptr<TimeshiftSegment> nextSegment)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_nextSegment = nextSegment;
}

std::shared_ptr<TimeshiftSegment> TimeshiftSegment::GetNextSegment()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_nextSegment;
}

void TimeshiftSegment::ResetReadIndex()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_readPacketIndex = 0;
}

int TimeshiftSegment::GetReadIndex()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_readPacketIndex;
}

int TimeshiftSegment::GetSegmentId()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_segmentId;
}

DemuxPacket* TimeshiftSegment::ReadPacket()
{
  DemuxPacket* packet = nullptr;

  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_readPacketIndex != m_packetBuffer.size())
  {
    std::shared_ptr<DemuxPacket>& nextPacket = m_packetBuffer[m_readPacketIndex++];

    packet = m_demuxPacketManager->AllocateDemuxPacketFromInputStreamAPI(nextPacket->iSize);

    CopyPacket(nextPacket.get(), packet, false);
  }
  else
  {
    packet = m_demuxPacketManager->AllocateDemuxPacketFromInputStreamAPI(0);
  }

  return packet;
}

bool TimeshiftSegment::Seek(double timeMs)
{
  int seekSeconds = timeMs / 1000;
  std::lock_guard<std::mutex> lock(m_mutex);

  auto seekPacketIndex = m_packetTimeIndexMap.upper_bound(seekSeconds);
  // Upper bound gets the packet after the one we want
  if (seekPacketIndex != m_packetTimeIndexMap.begin())
    --seekPacketIndex;

  if (seekPacketIndex != m_packetTimeIndexMap.end())
  {
    m_readPacketIndex = seekPacketIndex->second;

    auto it = m_packetTimeIndexMap.begin();
    int timeIndexStart = it->first;
    auto it2 = m_packetTimeIndexMap.rbegin();
    int timeIndexEnd = it2->first;
    Log(LOGLEVEL_INFO, "%s - Seek segment packet - segment ID: %d, packet index: %d, seek seconds: %d, segmetn start seconds: %d, segment end seconds: %d", __FUNCTION__, m_segmentId, m_readPacketIndex, seekSeconds, timeIndexStart, timeIndexEnd);

    return true;
  }

  return false;
}

/* For refernce */

// typedef struct DemuxPacket
// {
//   DemuxPacket() = default;

//   uint8_t *pData = nullptr;
//   int iSize = 0;
//   int iStreamId = -1;
//   int64_t demuxerId = -1; // id of the demuxer that created the packet
//   int iGroupId = -1; // the group this data belongs to, used to group data from different streams together

//   void *pSideData = nullptr;
//   int iSideDataElems = 0;

//   double pts = DVD_NOPTS_VALUE;
//   double dts = DVD_NOPTS_VALUE;
//   double duration = 0; // duration in DVD_TIME_BASE if available
//   int dispTime = 0;
//   bool recoveryPoint = false;

//   std::shared_ptr<DemuxCryptoInfo> cryptoInfo;
// } DemuxPacket;

// typedef struct AVPacketSideData {
//     uint8_t *data;
//     int      size;
//     enum AVPacketSideDataType type;
// } AVPacketSideData;

  // explicit DemuxCryptoInfo(const unsigned int numSubs)
  //   : numSubSamples(numSubs)
  //   , flags(0)
  //   , clearBytes(new uint16_t[numSubs])
  //   , cipherBytes(new uint32_t[numSubs])
  // {};

  // ~DemuxCryptoInfo()
  // {
  //   delete[] clearBytes;
  //   delete[] cipherBytes;
  // }

  // uint16_t numSubSamples; //number of subsamples
  // uint16_t flags; //flags for later use

  // uint16_t *clearBytes; // numSubSamples uint16_t's wich define the size of clear size of a subsample
  // uint32_t *cipherBytes; // numSubSamples uint32_t's wich define the size of cipher size of a subsample

  // uint8_t iv[16]; // initialization vector
  // uint8_t kid[16]; // key id