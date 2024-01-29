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
  static const int LEGAL_NONE = 0;
  static const int LEGAL_WIN32_COMPAT = 1;
  static const int LEGAL_FATX = 2;

  constexpr const char* TEMP_FONT_PATH = "special://temp/fonts/";

  class FilenameUtils
  {
  public:

  /*
   * \brief Find a language code with subtag (e.g. zh-tw, zh-Hans) in to a string.
   *        This function find a limited set of IETF BCP47 specs, so:
   *        language tag + region subtag, or, language tag + script subtag.
   *        The language code can be found also if wrapped with round brackets.
   * \param str The string where find the language code.
   * \return The language code found in the string, otherwise empty string
   */
  static std::string FindLanguageCodeWithSubtag(const std::string& str);

#ifdef TARGET_WINDOWS
    static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_WIN32_COMPAT);
    static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
    static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_NONE);
    static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_NONE);
#endif
  };
} //namespace ffmpegdirect