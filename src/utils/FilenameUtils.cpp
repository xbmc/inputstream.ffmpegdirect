/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "FilenameUtils.h"

#include <kodi/tools/StringUtils.h>

#include <regex>

using namespace ffmpegdirect;
using namespace kodi::tools;

std::string FilenameUtils::FindLanguageCodeWithSubtag(const std::string& str)
{
  static std::regex regLangCode("(?:^|\\s|\\()(([A-Za-z]{2,3})-([A-Za-z]{2}|[0-9]{3}|[A-Za-z]{4}))(?:$|\\s|\\))");
  std::smatch match;
  std::string matchText = "";

  if (std::regex_match(str, match, regLangCode))
  {
    if (match.size() == 2)
    {
      std::ssub_match base_sub_match = match[1];
      matchText = base_sub_match.str();
    }
  }

  return matchText;
}

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
