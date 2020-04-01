/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
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