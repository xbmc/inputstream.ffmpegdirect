/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#include <kodi/addon-instance/Inputstream.h>

class IManageDemuxPacket;

class BaseStream
{
public:
  BaseStream(IManageDemuxPacket* demuxPacketMamnager) : m_demuxPacketMamnager(demuxPacketMamnager) {};

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) = 0;
  virtual void Close() = 0;
  virtual void GetCapabilities(INPUTSTREAM_CAPABILITIES& caps) = 0;
  virtual INPUTSTREAM_IDS GetStreamIds() = 0;
  virtual INPUTSTREAM_INFO GetStream(int streamid) = 0;
  virtual void EnableStream(int streamid, bool enable) = 0;
  virtual bool OpenStream(int streamid) = 0;

  //New
  virtual void DemuxReset() = 0;
  virtual void DemuxAbort() = 0;
  virtual void DemuxFlush() = 0;
  virtual DemuxPacket* DemuxRead() = 0;
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) = 0;
  virtual void DemuxSetSpeed(int speed) = 0;
  virtual void SetVideoResolution(int width, int height) = 0;

  virtual int GetTotalTime() = 0;
  virtual int GetTime() = 0;

  //New
  virtual bool GetTimes(INPUTSTREAM_TIMES& times) = 0;

  virtual bool PosTime(int ms) = 0;

  //New
  virtual int GetChapter() = 0;
  virtual int GetChapterCount() = 0;
  virtual const char* GetChapterName(int ch) = 0;
  virtual int64_t GetChapterPos(int ch) = 0;
  virtual bool SeekChapter(int ch) = 0;


  virtual bool CanPauseStream() = 0;
  virtual bool CanSeekStream() = 0;
  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) = 0;
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) = 0;
  virtual int64_t PositionStream() = 0;
  virtual int64_t LengthStream() = 0;
  virtual void PauseStream(double time) = 0;
  virtual bool IsRealTimeStream() = 0;

protected:
  IManageDemuxPacket* m_demuxPacketMamnager;
};