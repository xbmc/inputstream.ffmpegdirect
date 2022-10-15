/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

// https://github.com/FFmpeg/FFmpeg/blob/56450a0ee4/doc/APIchanges#L18-L26
#if LIBAVFORMAT_BUILD >= AV_VERSION_INT(59, 0, 100)
#define FFMPEG_FMT_CONST const
#else
#define FFMPEG_FMT_CONST
#endif
