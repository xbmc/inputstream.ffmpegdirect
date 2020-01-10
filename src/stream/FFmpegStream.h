/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"

#include "BaseStream.h"
#include "DemuxStream.h"

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <kodi/addon-instance/Inputstream.h>
#include <kodi/DemuxCrypto.h>

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mastering_display_metadata.h>
}

#ifndef __GNUC__
#pragma warning(pop)
#endif

#define FFMPEG_DVDNAV_BUFFER_SIZE 2048  // for dvd's

struct StereoModeConversionMap;

class FFmpegStream
  : public BaseStream
{
public:
  FFmpegStream(IManageDemuxPacket* demuxPacketManager);
  ~FFmpegStream();

  virtual bool Open(const std::string& streamUrl, const std::string& mimeType, bool isRealTimeStream, const std::string& programProperty) override;
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

  virtual int GetTotalTime() override;// { return 20; }
  virtual int GetTime() override;// { return m_displayTime; }
  virtual bool GetTimes(INPUTSTREAM_TIMES& times) override;
  virtual bool PosTime(int ms) override;

  virtual int GetChapter() override { return -1; };
  virtual int GetChapterCount() override { return 0; };
  virtual const char* GetChapterName(int ch) override { return nullptr; };
  virtual int64_t GetChapterPos(int ch) override { return 0; };
  virtual bool SeekChapter(int ch) override { return false; };

  virtual bool CanPauseStream() override;
  virtual bool CanSeekStream() override;
  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) override;
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) override;
  virtual int64_t PositionStream() override;
  virtual int64_t LengthStream() override;
  virtual void PauseStream(double time) override;
  virtual bool IsRealTimeStream() override; // { return true; }

  void Dispose();
  void DisposeStreams();
  bool Aborted();

  AVFormatContext* m_pFormatContext;

protected:
  virtual std::string GetStreamCodecName(int iStreamId);
  virtual void UpdateCurrentPTS();

  int64_t m_demuxerId;
  CCriticalSection m_critSection;
  double m_currentPts; // used for stream length estimation
  bool m_demuxResetOpenSuccess = false;
  std::string m_streamUrl;

private:
  bool Open(bool streaminfo = true, bool fileinfo = false);
  bool OpenWithAVFormat(AVInputFormat* iformat, const AVIOInterruptCB& int_cb);
  AVDictionary* GetFFMpegOptionsFromInput();
  void ResetVideoStreams();
  double ConvertTimestamp(int64_t pts, int den, int num);
  unsigned int HLSSelectProgram();
  int GetNrOfStreams() const;
  int GetNrOfStreams(INPUTSTREAM_INFO::STREAM_TYPE streamType);
  int GetNrOfSubtitleStreams();
  std::vector<DemuxStream*> GetDemuxStreams() const;
  DemuxStream* GetDemuxStream(int iStreamId) const;
  void CreateStreams(unsigned int program);
  void AddStream(int streamIdx, DemuxStream* stream);
  DemuxStream* AddStream(int streamIdx);
  double SelectAspect(AVStream* st, bool& forced);
  std::string GetStereoModeFromMetadata(AVDictionary* pMetadata);
  std::string ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap* conversionMap);
  bool IsVideoReady();
  bool SeekTime(double time, bool backwards = false, double* startpts = nullptr);
  void ParsePacket(AVPacket* pkt);
  bool IsProgramChange();
  void StoreSideData(DemuxPacket *pkt, AVPacket *src);

  bool StreamsOpened() { return m_streams.size() > 0; }

  int64_t NewGuid()
  {
    static int64_t guid = 0;
    return guid++;
  }

  bool m_paused;

  std::map<int, DemuxStream*> m_streams;
  std::map<int, std::unique_ptr<DemuxParserFFmpeg>> m_parsers;

  AVIOContext* m_ioContext;

  bool     m_bMatroska;
  bool     m_bAVI;
  bool     m_bSup;
  int      m_speed;
  unsigned int m_program;
  unsigned int m_streamsInProgram;
  unsigned int m_newProgram;
  unsigned int m_initialProgramNumber;
  int m_seekStream;

  XbmcThreads::EndTime  m_timeout;

  // Due to limitations of ffmpeg, we only can detect a program change
  // with a packet. This struct saves the packet for the next read and
  // signals STREAMCHANGE to player
  struct
  {
    AVPacket pkt;       // packet ffmpeg returned
    int      result;    // result from av_read_packet
  }m_pkt;

  bool m_streaminfo;
  bool m_checkvideo;
  int m_displayTime = 0;
  double m_dtsAtDisplayTime;
  bool m_seekToKeyFrame = false;
  double m_startTime = 0;

  std::string m_mimeType;
  std::string m_programProperty;
  bool m_isRealTimeStream;
  bool m_opened;
};