/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

typedef enum LogLevel
{
  LOGLEVEL_DEBUG = 0,
  LOGLEVEL_INFO = 1,
  LOGLEVEL_NOTICE = 2,
  LOGLEVEL_WARNING = 3,
  LOGLEVEL_ERROR = 4,
  LOGLEVEL_SEVERE = 5,
  LOGLEVEL_FATAL = 6
} LogLevel;


extern void Log(const LogLevel loglevel, const char* format, ...);