/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "stream/FFmpegCatchupStream.h"
#include "stream/FFmpegStream.h"
#include "stream/IManageDemuxPacket.h"

#include <memory>
#include <string>

#include <kodi/addon-instance/Inputstream.h>

static const std::string MIME_TYPE = "inputstream.ffmpegdirect.mime_type";
static const std::string PROGRAM_NUMBER = "inputstream.ffmpegdirect.program_number";
static const std::string IS_REALTIME_STREAM = "inputstream.ffmpegdirect.is_realtime_stream";
static const std::string CHUNK_SIZE_KB = "inputstream.ffmpegdirect.chunk_size_kb";
static const std::string STREAM_MODE = "inputstream.ffmpegdirect.stream_mode";
static const std::string DEFAULT_URL = "inputstream.ffmpegdirect.default_url";
static const std::string PLAYBACK_AS_LIVE = "inputstream.ffmpegdirect.playback_as_live";
static const std::string PROGRAMME_START_TIME = "inputstream.ffmpegdirect.programme_start_time";
static const std::string PROGRAMME_END_TIME = "inputstream.ffmpegdirect.programme_end_time";
static const std::string CATCHUP_URL_FORMAT_STRING = "inputstream.ffmpegdirect.catchup_url_format_string";
static const std::string CATCHUP_URL_NEAR_LIVE_FORMAT_STRING = "inputstream.ffmpegdirect.catchup_url_near_live_format_string";
static const std::string CATCHUP_BUFFER_START_TIME = "inputstream.ffmpegdirect.catchup_buffer_start_time";
static const std::string CATCHUP_BUFFER_END_TIME = "inputstream.ffmpegdirect.catchup_buffer_end_time";
static const std::string CATCHUP_BUFFER_OFFSET = "inputstream.ffmpegdirect.catchup_buffer_offset";
static const std::string TIMEZONE_SHIFT = "inputstream.ffmpegdirect.timezone_shift";
static const std::string DEFAULT_PROGRAMME_DURATION = "inputstream.ffmpegdirect.default_programme_duration";
static const std::string PROGRAMME_CATCHUP_ID = "inputstream.ffmpegdirect.programme_catchup_id";

static const int CHUNK_SIZE_KB_MAX = 128;

enum class StreamMode
  : int // same type as addon settings
{
  NONE = 0,
  CATCHUP
};

class CInputStreamLibavformat
  : public kodi::addon::CInstanceInputStream, IManageDemuxPacket
{
public:
  CInputStreamLibavformat(KODI_HANDLE instance);
  ~CInputStreamLibavformat();

  virtual bool Open(INPUTSTREAM& props) override;
  virtual void Close() override;
  virtual void GetCapabilities(INPUTSTREAM_CAPABILITIES& caps) override;
  virtual INPUTSTREAM_IDS GetStreamIds() override;
  virtual INPUTSTREAM_INFO GetStream(int streamid) override;
  virtual void EnableStream(int streamid, bool enable) override;
  virtual bool OpenStream(int streamid) override;

  virtual void DemuxReset() override;
  virtual void DemuxAbort() override;
  virtual void DemuxFlush() override;
  virtual DemuxPacket* DemuxRead() override;
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) override;
  virtual void DemuxSetSpeed(int speed) override;
  virtual void SetVideoResolution(int width, int height) override;

  virtual int GetTotalTime() override;
  virtual int GetTime() override;
  virtual bool GetTimes(INPUTSTREAM_TIMES& times) override;
  virtual bool PosTime(int ms) override;

  virtual int GetChapter() override;
  virtual int GetChapterCount() override;
  virtual const char* GetChapterName(int ch) override;
  virtual int64_t GetChapterPos(int ch) override;
  virtual bool SeekChapter(int ch) override;

  virtual bool CanPauseStream() override;
  virtual bool CanSeekStream() override;
  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) override;
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) override;
  virtual int64_t PositionStream() override;
  virtual int64_t LengthStream() override;
  virtual void PauseStream(double time) override;
  virtual bool IsRealTimeStream() override; // { return true; }
  virtual int GetBlockSize() override;

  DemuxPacket* AllocateDemuxPacketFromInputStreamAPI(int dataSize) override;
  DemuxPacket* AllocateEncryptedDemuxPacketFromInputStreamAPI(int dataSize, unsigned int encryptedSubsampleCount) override;
  void FreeDemuxPacketFromInputStreamAPI(DemuxPacket* packet) override;

protected:

private:
  bool m_opened;

  std::string m_streamUrl;
  std::string m_mimeType;
  std::string m_programProperty;
  bool m_isRealTimeStream;
  int m_streamReadChunkSizeKb = 0;
  StreamMode m_streamMode = StreamMode::NONE;
  std::string m_defaultUrl;

  bool m_playbackAsLive = false;
  time_t m_programmeStartTime = 0;
  time_t m_programmeEndTime = 0;
  std::string m_catchupUrlFormatString;
  std::string m_catchupUrlNearLiveFormatString;
  time_t m_catchupBufferStartTime = 0;
  time_t m_catchupBufferEndTime = 0;
  long long m_catchupBufferOffset = 0;
  int m_timezoneShiftSecs = 0;
  int m_defaultProgrammeDurationSecs = 4 * 60 * 60; //Four hours
  std::string m_programmeCatchupId;

  int m_videoWidth;
  int m_videoHeight;

  std::shared_ptr<BaseStream> m_stream;
};