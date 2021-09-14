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
  class DiskUtils
  {
  public:
    static bool GetFreeDiskSpaceMB(const std::string& path, uint64_t& freeMB);
  };
} //namespace ffmpegdirect
