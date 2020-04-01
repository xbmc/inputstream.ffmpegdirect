/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Exception.h"

#include "../../utils/Log.h"

namespace XbmcCommons
{
  Exception::~Exception() = default;

  void Exception::LogThrowMessage(const char* prefix) const
  {
    Log(LOGLEVEL_ERROR, "EXCEPTION Thrown (%s) : %s", classname.c_str(), message.c_str());
  }
}

