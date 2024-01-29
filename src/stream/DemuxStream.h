/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <kodi/addon-instance/Inputstream.h>

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/dovi_meta.h>
#include <libavformat/avformat.h>
#include <libavutil/mastering_display_metadata.h>
}

#ifndef __GNUC__
#pragma warning(pop)
#endif

namespace ffmpegdirect
{


class FFmpegExtraData
{
public:
  FFmpegExtraData() = default;
  explicit FFmpegExtraData(size_t size);
  FFmpegExtraData(const uint8_t* data, size_t size);
  FFmpegExtraData(const FFmpegExtraData& other);
  FFmpegExtraData(FFmpegExtraData&& other) noexcept;

  ~FFmpegExtraData();

  FFmpegExtraData& operator=(const FFmpegExtraData& other);
  FFmpegExtraData& operator=(FFmpegExtraData&& other) noexcept;

  bool operator==(const FFmpegExtraData& other) const;
  bool operator!=(const FFmpegExtraData& other) const;

  operator bool() const { return m_data != nullptr && m_size != 0; }
  uint8_t* GetData() { return m_data; }
  const uint8_t* GetData() const { return m_data; }
  size_t GetSize() const { return m_size; }
  /*!
   * \brief Take ownership over the extra data buffer
   *
   * It's in the responsibility of the caller to free the buffer with av_free. After the call
   * FFmpegExtraData is empty.
   *
   * \return The extra data buffer or nullptr if FFmpegExtraData is empty.
   */
  uint8_t* TakeData();

private:
  uint8_t* m_data{nullptr};
  size_t m_size{};
};

class DemuxStream
{
public:
  DemuxStream()
  {
    uniqueId = 0;
    dvdNavId = 0;
    demuxerId = -1;
    codec_fourcc = 0;
    profile = FF_PROFILE_UNKNOWN;
    level = FF_LEVEL_UNKNOWN;
    type = INPUTSTREAM_TYPE_NONE;
    iDuration = 0;
    pPrivate = NULL;
    disabled = false;
    changes = 0;
    flags = INPUTSTREAM_FLAG_NONE;
  }

  virtual ~DemuxStream() = default;
  DemuxStream(DemuxStream&&) = default;

  virtual std::string GetStreamName();
  virtual bool GetInformation(kodi::addon::InputstreamInfo& info);

  int uniqueId; // unique stream id
  int dvdNavId;
  int64_t demuxerId; // id of the associated demuxer
  AVCodecID codec = AV_CODEC_ID_NONE;
  unsigned int codec_fourcc; // if available
  int profile; // encoder profile of the stream reported by the decoder. used to qualify hw decoders.
  int level; // encoder level of the stream reported by the decoder. used to qualify hw decoders.
  INPUTSTREAM_TYPE type;

  int iDuration; // in mseconds
  void* pPrivate; // private pointer for the demuxer
  FFmpegExtraData extraData;

  INPUTSTREAM_FLAGS flags;
  std::string language; // RFC 5646 language code (empty string if undefined)
  bool disabled; // set when stream is disabled. (when no decoder exists)

  std::string name;
  std::string codecName;

  int changes; // increment on change which player may need to know about

  std::shared_ptr<kodi::addon::StreamCryptoSession> cryptoSession;
};

enum class StreamHdrType
{
  HDR_TYPE_NONE, ///< <b>None</b>, returns an empty string when used in infolabels
  HDR_TYPE_HDR10, ///< <b>HDR10</b>, returns `hdr10` when used in infolabels
  HDR_TYPE_DOLBYVISION, ///< <b>Dolby Vision</b>, returns `dolbyvision` when used in infolabels
  HDR_TYPE_HLG ///< <b>HLG</b>, returns `hlg` when used in infolabels
};

class DemuxStreamVideo : public DemuxStream
{
public:
  DemuxStreamVideo() { type = INPUTSTREAM_TYPE_VIDEO; };

  ~DemuxStreamVideo() override = default;

  int iFpsScale = 0; // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int iFpsRate = 0;
  int iHeight = 0; // height of the stream reported by the demuxer
  int iWidth = 0; // width of the stream reported by the demuxer
  double fAspect = 0; // display aspect of stream
  bool bVFR = false; // variable framerate
  bool bPTSInvalid = false; // pts cannot be trusted (avi's).
  bool bForcedAspect = false; // aspect is forced from container
  int iOrientation = 0; // orientation of the video in degrees counter clockwise
  int iBitsPerPixel = 0;
  int iBitRate = 0;
  int iBitDepth = 0;

  AVColorSpace colorSpace = AVCOL_SPC_UNSPECIFIED;
  AVColorRange colorRange = AVCOL_RANGE_UNSPECIFIED;
  AVColorPrimaries colorPrimaries = AVCOL_PRI_UNSPECIFIED;
  AVColorTransferCharacteristic colorTransferCharacteristic = AVCOL_TRC_UNSPECIFIED;

  std::shared_ptr<AVMasteringDisplayMetadata> masteringMetaData;
  std::shared_ptr<AVContentLightMetadata> contentLightMetaData;

  std::string stereo_mode; // expected stereo mode
  AVDOVIDecoderConfigurationRecord dovi{};
  StreamHdrType hdr_type = StreamHdrType::HDR_TYPE_NONE; // type of HDR for this stream (hdr10, etc)
};

class DemuxStreamAudio : public DemuxStream
{
public:
  DemuxStreamAudio()
    : DemuxStream()
  {
    iChannels = 0;
    iSampleRate = 0;
    iBlockAlign = 0;
    iBitRate = 0;
    iBitsPerSample = 0;
    iChannelLayout = 0;
    type = INPUTSTREAM_TYPE_AUDIO;
  }

  ~DemuxStreamAudio() override = default;

  std::string GetStreamType();

  int iChannels;
  int iSampleRate;
  int iBlockAlign;
  int iBitRate;
  int iBitsPerSample;
  uint64_t iChannelLayout;
  std::string m_channelLayoutName;
};

class DemuxStreamSubtitle : public DemuxStream
{
public:
  DemuxStreamSubtitle()
    : DemuxStream()
  {
    type = INPUTSTREAM_TYPE_SUBTITLE;
  }
};

class DemuxStreamTeletext : public DemuxStream
{
public:
  DemuxStreamTeletext()
    : DemuxStream()
  {
    type = INPUTSTREAM_TYPE_TELETEXT;
  }
};

class FFmpegStream;

class DemuxStreamVideoFFmpeg : public DemuxStreamVideo
{
public:
  explicit DemuxStreamVideoFFmpeg(AVStream* stream) : m_stream(stream) {}
  std::string GetStreamName() override;
  bool GetInformation(kodi::addon::InputstreamInfo& info) override;

  std::string m_description;
protected:
  AVStream* m_stream = nullptr;
};

class DemuxStreamAudioFFmpeg : public DemuxStreamAudio
{
public:
  explicit DemuxStreamAudioFFmpeg(AVStream* stream) : m_stream(stream) {}
  std::string GetStreamName() override;
  bool GetInformation(kodi::addon::InputstreamInfo& info) override;

  std::string m_description;
protected:
  FFmpegStream* m_parent;
  AVStream* m_stream  = nullptr;
};

class DemuxStreamSubtitleFFmpeg : public DemuxStreamSubtitle
{
public:
  explicit DemuxStreamSubtitleFFmpeg(AVStream* stream) : m_stream(stream) {}
  std::string GetStreamName() override;
  bool GetInformation(kodi::addon::InputstreamInfo& info) override;

  std::string m_description;
protected:
  FFmpegStream* m_parent;
  AVStream* m_stream = nullptr;
};

class DemuxParserFFmpeg
{
public:
  ~DemuxParserFFmpeg();
  AVCodecParserContext* m_parserCtx = nullptr;
  AVCodecContext* m_codecCtx = nullptr;
};

} //namespace ffmpegdirect
