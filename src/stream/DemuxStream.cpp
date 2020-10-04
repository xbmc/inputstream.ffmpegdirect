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

#ifdef TARGET_POSIX
#include "platform/posix/XTimeUtils.h"
#endif

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

bool DemuxStream::GetInformation(INPUTSTREAM_INFO &info)
{
  info.m_streamType = type;

  info.m_flags = flags;
  strncpy(info.m_name, name.c_str(), sizeof(info.m_name) - 1);
  strncpy(info.m_codecName, codecName.c_str(), sizeof(info.m_codecName) - 1);
  info.m_codecProfile = static_cast<STREAMCODEC_PROFILE>(profile);
  info.m_pID = uniqueId;

  info.m_ExtraData = ExtraData;
  info.m_ExtraSize = ExtraSize;

  strncpy(info.m_language, language.c_str(), sizeof(info.m_language) - 1);

  info.m_codecFourCC = codec_fourcc;

  if (cryptoSession)
  {
    if (cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_NONE)
      info.m_cryptoInfo.m_CryptoKeySystem = CRYPTO_INFO::CRYPTO_KEY_SYSTEM_NONE;
    else if (cryptoSession->keySystem == CRYPTO_SESSION_SYSTEM_WIDEVINE)
      info.m_cryptoInfo.m_CryptoKeySystem = CRYPTO_INFO::CRYPTO_KEY_SYSTEM_WIDEVINE;
    else
      info.m_cryptoInfo.m_CryptoKeySystem = CRYPTO_INFO::CRYPTO_KEY_SYSTEM_PLAYREADY;
  }

  return true;
}

bool DemuxStreamVideoFFmpeg::GetInformation(INPUTSTREAM_INFO &info)
{
  DemuxStream::GetInformation(info);

  info.m_FpsScale = iFpsScale;
  info.m_FpsRate = iFpsRate;
  info.m_Height = iHeight;
  info.m_Width = iWidth;
  info.m_Aspect = fAspect;
  info.m_Channels = 0;
  info.m_SampleRate = 0;
  info.m_BitRate = 0;
  info.m_BitsPerSample = 0;
  info.m_BlockAlign = 0;

  info.m_colorSpace = INPUTSTREAM_INFO::COLORSPACE_UNSPECIFIED;
  info.m_colorRange = INPUTSTREAM_INFO::COLORRANGE_UNKNOWN;
  info.m_colorPrimaries = INPUTSTREAM_INFO::COLORPRIMARY_UNSPECIFIED;
  info.m_colorTransferCharacteristic = INPUTSTREAM_INFO::COLORTRC_UNSPECIFIED;

  info.m_masteringMetadata = nullptr;
  info.m_contentLightMetadata = nullptr;

  if (masteringMetaData)
  {
    m_inputstreamMasteringMetadata = {0};

    if (masteringMetaData->has_primaries)
    {
      m_inputstreamMasteringMetadata.primary_r_chromaticity_x = masteringMetaData->display_primaries[0][0].num / masteringMetaData->display_primaries[0][0].den;
      m_inputstreamMasteringMetadata.primary_r_chromaticity_y = masteringMetaData->display_primaries[0][1].num / masteringMetaData->display_primaries[0][1].den;
      m_inputstreamMasteringMetadata.primary_g_chromaticity_x = masteringMetaData->display_primaries[1][0].num / masteringMetaData->display_primaries[1][0].den;
      m_inputstreamMasteringMetadata.primary_g_chromaticity_y = masteringMetaData->display_primaries[1][1].num / masteringMetaData->display_primaries[1][1].den;
      m_inputstreamMasteringMetadata.primary_b_chromaticity_x = masteringMetaData->display_primaries[2][0].num / masteringMetaData->display_primaries[2][0].den;
      m_inputstreamMasteringMetadata.primary_b_chromaticity_y = masteringMetaData->display_primaries[2][1].num / masteringMetaData->display_primaries[2][1].den;
      m_inputstreamMasteringMetadata.white_point_chromaticity_x = masteringMetaData->white_point[0].num / masteringMetaData->white_point[0].den;
      m_inputstreamMasteringMetadata.white_point_chromaticity_y = masteringMetaData->white_point[1].num / masteringMetaData->white_point[1].den;
    }

    if (masteringMetaData->has_luminance)
    {
      m_inputstreamMasteringMetadata.luminance_max = masteringMetaData->max_luminance.num / masteringMetaData->max_luminance.den;
      m_inputstreamMasteringMetadata.luminance_min = masteringMetaData->min_luminance.num / masteringMetaData->min_luminance.den;
    }

    info.m_masteringMetadata = &m_inputstreamMasteringMetadata;
  }

  if (contentLightMetaData)
  {
    m_inputstreamContentLightMetadata = {0};

    m_inputstreamContentLightMetadata.max_cll = contentLightMetaData->MaxCLL;
    m_inputstreamContentLightMetadata.max_fall = contentLightMetaData->MaxFALL;

    info.m_contentLightMetadata = &m_inputstreamContentLightMetadata;
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

bool DemuxStreamAudioFFmpeg::GetInformation(INPUTSTREAM_INFO &info)
{
  DemuxStream::GetInformation(info);

  info.m_Channels = iChannels;
  info.m_SampleRate = iSampleRate;
  info.m_BitRate = iBitRate;
  info.m_BitsPerSample = iBitsPerSample;
  info.m_BlockAlign = iBlockAlign;

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

bool DemuxStreamSubtitleFFmpeg::GetInformation(INPUTSTREAM_INFO &info)
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