/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

typedef enum LogLevel
{
  LOGLEVEL_DEBUG,
  LOGLEVEL_INFO,
  LOGLEVEL_WARNING,
  LOGLEVEL_ERROR,
  LOGLEVEL_FATAL
} LogLevel;


extern void Log(const LogLevel loglevel, const char* format, ...);