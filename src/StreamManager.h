/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "stream/FFmpegStream.h"
#include "stream/IManageDemuxPacket.h"
#include "utils/Properties.h"

#include <memory>
#include <string>

#include <kodi/addon-instance/Inputstream.h>

static const std::string PROGRAM_NUMBER = "inputstream.ffmpegdirect.program_number";
static const std::string IS_REALTIME_STREAM = "inputstream.ffmpegdirect.is_realtime_stream";
static const std::string STREAM_MODE = "inputstream.ffmpegdirect.stream_mode";
static const std::string OPEN_MODE = "inputstream.ffmpegdirect.open_mode";
static const std::string MANIFEST_TYPE = "inputstream.ffmpegdirect.manifest_type";
static const std::string DEFAULT_URL = "inputstream.ffmpegdirect.default_url";
static const std::string PLAYBACK_AS_LIVE = "inputstream.ffmpegdirect.playback_as_live";
static const std::string PROGRAMME_START_TIME = "inputstream.ffmpegdirect.programme_start_time";
static const std::string PROGRAMME_END_TIME = "inputstream.ffmpegdirect.programme_end_time";
static const std::string CATCHUP_URL_FORMAT_STRING = "inputstream.ffmpegdirect.catchup_url_format_string";
static const std::string CATCHUP_URL_NEAR_LIVE_FORMAT_STRING = "inputstream.ffmpegdirect.catchup_url_near_live_format_string";
static const std::string CATCHUP_BUFFER_START_TIME = "inputstream.ffmpegdirect.catchup_buffer_start_time";
static const std::string CATCHUP_BUFFER_END_TIME = "inputstream.ffmpegdirect.catchup_buffer_end_time";
static const std::string CATCHUP_BUFFER_OFFSET = "inputstream.ffmpegdirect.catchup_buffer_offset";
static const std::string CATCHUP_TERMINATES = "inputstream.ffmpegdirect.catchup_terminates";
static const std::string CATCHUP_GRANULARITY = "inputstream.ffmpegdirect.catchup_granularity";
static const std::string TIMEZONE_SHIFT = "inputstream.ffmpegdirect.timezone_shift";
static const std::string DEFAULT_PROGRAMME_DURATION = "inputstream.ffmpegdirect.default_programme_duration";
static const std::string PROGRAMME_CATCHUP_ID = "inputstream.ffmpegdirect.programme_catchup_id";

class ATTRIBUTE_HIDDEN InputStreamFFmpegDirect
  : public kodi::addon::CInstanceInputStream, ffmpegdirect::IManageDemuxPacket
{
public:
  InputStreamFFmpegDirect(KODI_HANDLE instance, const std::string& version);
  ~InputStreamFFmpegDirect();

  virtual bool Open(const kodi::addon::InputstreamProperty& props) override;
  virtual void Close() override;
  virtual void GetCapabilities(kodi::addon::InputstreamCapabilities& caps) override;
  virtual bool GetStreamIds(std::vector<unsigned int>& ids) override;
  virtual bool GetStream(int streamid, kodi::addon::InputstreamInfo& info) override;
  virtual void EnableStream(int streamid, bool enable) override;
  virtual bool OpenStream(int streamid) override;

  virtual void DemuxReset() override;
  virtual void DemuxAbort() override;
  virtual void DemuxFlush() override;
  virtual DEMUX_PACKET* DemuxRead() override;
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) override;
  virtual void DemuxSetSpeed(int speed) override;
  virtual void SetVideoResolution(int width, int height) override;

  virtual int GetTotalTime() override;
  virtual int GetTime() override;
  virtual bool GetTimes(kodi::addon::InputstreamTimes& times) override;
  virtual bool PosTime(int ms) override;

  virtual int GetChapter() override;
  virtual int GetChapterCount() override;
  virtual const char* GetChapterName(int ch) override;
  virtual int64_t GetChapterPos(int ch) override;
  virtual bool SeekChapter(int ch) override;

  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) override;
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) override;
  virtual int64_t PositionStream() override;
  virtual int64_t LengthStream() override;
  virtual bool IsRealTimeStream() override; // { return true; }

  DEMUX_PACKET* AllocateDemuxPacketFromInputStreamAPI(int dataSize) override;
  DEMUX_PACKET* AllocateEncryptedDemuxPacketFromInputStreamAPI(int dataSize, unsigned int encryptedSubsampleCount) override;
  void FreeDemuxPacketFromInputStreamAPI(DEMUX_PACKET* packet) override;

protected:

private:
  bool m_opened;

  std::string m_streamUrl;
  std::string m_mimeType;

  ffmpegdirect::Properties properties;

  int m_videoWidth;
  int m_videoHeight;

  std::shared_ptr<ffmpegdirect::BaseStream> m_stream;
};
