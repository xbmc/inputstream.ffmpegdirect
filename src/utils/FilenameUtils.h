/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace ffmpegdirect
{
  static const int LEGAL_NONE = 0;
  static const int LEGAL_WIN32_COMPAT = 1;
  static const int LEGAL_FATX = 2;

  class FilenameUtils
  {
  public:
#ifdef TARGET_WINDOWS
    static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_WIN32_COMPAT);
    static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
    static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_NONE);
    static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_NONE);
#endif      
  };
} //namespace ffmpegdirect