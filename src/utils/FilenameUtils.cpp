/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "FilenameUtils.h"

#include <p8-platform/util/StringUtils.h>

using namespace ffmpegdirect::utils;

std::string FilenameUtils::MakeLegalFileName(const std::string &strFile, int LegalType)
{
  std::string result = strFile;

  StringUtils::Replace(result, '/', '_');
  StringUtils::Replace(result, '\\', '_');
  StringUtils::Replace(result, '?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT)
  {
    // just filter out some illegal characters on windows
    StringUtils::Replace(result, ':', '_');
    StringUtils::Replace(result, '*', '_');
    StringUtils::Replace(result, '?', '_');
    StringUtils::Replace(result, '\"', '_');
    StringUtils::Replace(result, '<', '_');
    StringUtils::Replace(result, '>', '_');
    StringUtils::Replace(result, '|', '_');
    StringUtils::TrimRight(result, ". ");
  }
  return result;
}
