/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "DemuxStream.h"

#include "../utils/Log.h"
#include "url/URL.h"

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef TARGET_POSIX
#include <stdint.h>
#endif

extern "C" {
#include <libavutil/dict.h>
#include <libavutil/opt.h>
}

#include <kodi/tools/StringUtils.h>

using namespace ffmpegdirect;
using namespace kodi::tools;

/***********************************************************
* InputSteam Client AddOn specific public library functions
***********************************************************/

std::string DemuxStream::GetStreamName()
{
  return name;
}

bool DemuxStream::GetInformation(kodi::addon::InputstreamInfo& info)
{
  info.SetStreamType(type);

  info.SetFlags(flags);
  info.SetName(name);
  info.SetCodecName(codecName);
  info.SetCodecProfile(static_cast<STREAMCODEC_PROFILE>(profile));
  info.SetPhysicalIndex(uniqueId);
  info.SetExtraData(ExtraData, ExtraSize);
  info.SetLanguage(language);
  info.SetCodecFourCC(codec_fourcc);

  if (cryptoSession)
  {
    info.SetCryptoSession(*cryptoSession);
  }

  return true;
}

bool DemuxStreamVideoFFmpeg::GetInformation(kodi::addon::InputstreamInfo& info)
{
  DemuxStream::GetInformation(info);

  info.SetFpsScale(iFpsScale);
  info.SetFpsRate(iFpsRate);
  info.SetHeight(iHeight);
  info.SetWidth(iWidth);
  info.SetAspect(fAspect);
  info.SetChannels(0);
  info.SetSampleRate(0);
  info.SetBitRate(0);
  info.SetBitsPerSample(0);
  info.SetBlockAlign(0);

  info.SetColorSpace(INPUTSTREAM_COLORSPACE_UNSPECIFIED);
  info.SetColorRange(INPUTSTREAM_COLORRANGE_UNKNOWN);
  info.SetColorPrimaries(INPUTSTREAM_COLORPRIMARY_UNSPECIFIED);
  info.SetColorTransferCharacteristic(INPUTSTREAM_COLORTRC_UNSPECIFIED);

  if (masteringMetaData)
  {
    kodi::addon::InputstreamMasteringMetadata masteringMetadata;

    if (masteringMetaData->has_primaries)
    {
      masteringMetadata.SetPrimaryR_ChromaticityX(masteringMetaData->display_primaries[0][0].num / masteringMetaData->display_primaries[0][0].den);
      masteringMetadata.SetPrimaryR_ChromaticityY(masteringMetaData->display_primaries[0][1].num / masteringMetaData->display_primaries[0][1].den);
      masteringMetadata.SetPrimaryG_ChromaticityX(masteringMetaData->display_primaries[1][0].num / masteringMetaData->display_primaries[1][0].den);
      masteringMetadata.SetPrimaryG_ChromaticityY(masteringMetaData->display_primaries[1][1].num / masteringMetaData->display_primaries[1][1].den);
      masteringMetadata.SetPrimaryB_ChromaticityX(masteringMetaData->display_primaries[2][0].num / masteringMetaData->display_primaries[2][0].den);
      masteringMetadata.SetPrimaryB_ChromaticityY(masteringMetaData->display_primaries[2][1].num / masteringMetaData->display_primaries[2][1].den);
      masteringMetadata.SetWhitePoint_ChromaticityX(masteringMetaData->white_point[0].num / masteringMetaData->white_point[0].den);
      masteringMetadata.SetWhitePoint_ChromaticityY(masteringMetaData->white_point[1].num / masteringMetaData->white_point[1].den);
    }

    if (masteringMetaData->has_luminance)
    {
      masteringMetadata.SetLuminanceMax(masteringMetaData->max_luminance.num / masteringMetaData->max_luminance.den);
      masteringMetadata.SetLuminanceMin(masteringMetaData->min_luminance.num / masteringMetaData->min_luminance.den);
    }

    info.SetMasteringMetadata(masteringMetadata);
  }

  if (contentLightMetaData)
  {
    kodi::addon::InputstreamContentlightMetadata contentlightMetadata;

    contentlightMetadata.SetMaxCll(contentLightMetaData->MaxCLL);
    contentlightMetadata.SetMaxFall(contentLightMetaData->MaxFALL);

    info.SetContentLightMetadata(contentlightMetadata);
  }

  return true;
}

std::string DemuxStreamAudioFFmpeg::GetStreamName()
{
  if (!m_stream)
    return "";
  if (!m_description.empty())
    return m_description;
  else
    return DemuxStream::GetStreamName();
}

bool DemuxStreamAudioFFmpeg::GetInformation(kodi::addon::InputstreamInfo& info)
{
  DemuxStream::GetInformation(info);

  info.SetChannels(iChannels);
  info.SetSampleRate(iSampleRate);
  info.SetBitRate(iBitRate);
  info.SetBitsPerSample(iBitsPerSample);
  info.SetBlockAlign(iBlockAlign);

  return true;
}

std::string DemuxStreamSubtitleFFmpeg::GetStreamName()
{
  if (!m_stream)
    return "";
  if (!m_description.empty())
    return m_description;
  else
    return DemuxStream::GetStreamName();
}

bool DemuxStreamSubtitleFFmpeg::GetInformation(kodi::addon::InputstreamInfo& info)
{
  DemuxStream::GetInformation(info);

  return true;
}

std::string DemuxStreamVideoFFmpeg::GetStreamName()
{
  if (!m_stream)
    return "";
  if (!m_description.empty())
    return m_description;
  else
    return DemuxStream::GetStreamName();
}

DemuxParserFFmpeg::~DemuxParserFFmpeg()
{
  if (m_codecCtx)
    avcodec_free_context(&m_codecCtx);
  if (m_parserCtx)
  {
    av_parser_close(m_parserCtx);
    m_parserCtx = nullptr;
  }
}
