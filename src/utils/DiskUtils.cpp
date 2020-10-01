/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "DiskUtils.h"

#include "StringUtils.h"

#include <limits>

#include <kodi/Filesystem.h>

using namespace ffmpegdirect;

bool DiskUtils::GetFreeDiskSpaceMB(const std::string& path, uint64_t& freeMB)
{
  uint64_t capacity = std::numeric_limits<uint64_t>::max();
  uint64_t free = std::numeric_limits<uint64_t>::max();
  uint64_t available = std::numeric_limits<uint64_t>::max();
  bool success = kodi::vfs::GetDiskSpace(path, capacity, free, available);

  freeMB = free / 1024 /1024;

  return success;
}
