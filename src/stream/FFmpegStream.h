/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"

#include "../utils/HttpProxy.h"
#include "../utils/Properties.h"
#include "BaseStream.h"
#include "DemuxStream.h"
#include "CurlInput.h"

#include <iostream>
#include <map>
#include <memory>
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

namespace ffmpegdirect
{

enum class TRANSPORT_STREAM_STATE
{
  NONE,
  READY,
  NOTREADY,
};

class FFmpegStream
  : public BaseStream
{
public:
  FFmpegStream(IManageDemuxPacket* demuxPacketManager, const Properties& props, const HttpProxy& httpProxy);
  FFmpegStream(IManageDemuxPacket* demuxPacketManager, const Properties& props, std::shared_ptr<CurlInput> curlInput, const HttpProxy& httpProxy);
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

  void Dispose();
  void DisposeStreams();
  bool Aborted();

  AVFormatContext* m_pFormatContext;
  std::shared_ptr<CurlInput> m_curlInput;

protected:
  virtual std::string GetStreamCodecName(int iStreamId);
  virtual void UpdateCurrentPTS();
  bool IsPaused() { return m_speed == DVD_PLAYSPEED_PAUSE; }
  virtual bool CheckReturnEmptyOnPacketResult(int result);

  int64_t m_demuxerId;
  CCriticalSection m_critSection;
  double m_currentPts; // used for stream length estimation
  bool m_demuxResetOpenSuccess = false;
  std::string m_streamUrl;
  int m_lastPacketResult;
  bool m_isRealTimeStream;

private:
  bool Open(bool fileinfo);
  bool OpenWithFFmpeg(AVInputFormat* iformat, const AVIOInterruptCB& int_cb);
  bool OpenWithCURL(AVInputFormat* iformat);
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
  void GetL16Parameters(int& channels, int& samplerate);
  double SelectAspect(AVStream* st, bool& forced);
  std::string GetStereoModeFromMetadata(AVDictionary* pMetadata);
  std::string ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap* conversionMap);
  bool SeekTime(double time, bool backwards = false, double* startpts = nullptr);
  void ParsePacket(AVPacket* pkt);
  TRANSPORT_STREAM_STATE TransportStreamAudioState();
  TRANSPORT_STREAM_STATE TransportStreamVideoState();
  bool IsTransportStreamReady();
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

  FFmpegDirectThreads::EndTime  m_timeout;

  // Due to limitations of ffmpeg, we only can detect a program change
  // with a packet. This struct saves the packet for the next read and
  // signals STREAMCHANGE to player
  struct
  {
    AVPacket pkt;       // packet ffmpeg returned
    int      result;    // result from av_read_packet
  }m_pkt;

  bool m_streaminfo;
  bool m_reopen = false;
  bool m_checkTransportStream;
  int m_displayTime = 0;
  double m_dtsAtDisplayTime;
  bool m_seekToKeyFrame = false;
  double m_startTime = 0;

  std::string m_mimeType;
  std::string m_programProperty;
  std::string m_manifestType;
  bool m_opened;

  HttpProxy m_httpProxy;
  OpenMode m_openMode;
};

} //namespace ffmpegdirect