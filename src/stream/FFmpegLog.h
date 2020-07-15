/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

// #include "ServiceBroker.h"
// #include "utils/CPUInfo.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
}

// callback used for logging
void ff_avutil_log(void* ptr, int level, const char* format, va_list va);
void ff_flush_avutil_log_buffers(void);

namespace ffmpegdirect
{

class FFmpegLog
{
public:
  static void SetLogLevel(int level);
  static void SetEnabled(bool enabled);
  static bool GetEnabled();
  static int GetLogLevel();

  static int level;
  static bool enabled;
};

} //namespace ffmpegdirect
