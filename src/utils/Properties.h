/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace ffmpegdirect
{
  enum class StreamMode
    : int // same type as addon settings
  {
    NONE = 0,
    CATCHUP,
    TIMESHIFT
  };

  enum class OpenMode
    : int // same type as addon settings
  {
    DEFAULT = 0,
    FFMPEG,
    CURL
  };

  struct Properties
  {
    std::string m_programProperty;
    bool m_isRealTimeStream;
    StreamMode m_streamMode = StreamMode::NONE;
    OpenMode m_openMode = OpenMode::DEFAULT;
    std::string m_manifestType;
    std::string m_defaultUrl;

    bool m_playbackAsLive = false;
    time_t m_programmeStartTime = 0;
    time_t m_programmeEndTime = 0;
    std::string m_catchupUrlFormatString;
    std::string m_catchupUrlNearLiveFormatString;
    time_t m_catchupBufferStartTime = 0;
    time_t m_catchupBufferEndTime = 0;
    long long m_catchupBufferOffset = 0;
    bool m_catchupTerminates = false;
    int m_catchupGranularity = 1;
    int m_timezoneShiftSecs = 0;
    int m_defaultProgrammeDurationSecs = 4 * 60 * 60; //Four hours
    std::string m_programmeCatchupId;      
  };
} //namespace ffmpegdirect