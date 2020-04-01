/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#if (defined TARGET_POSIX)
#include "pthreads/ThreadImpl.h"
#elif (defined TARGET_WINDOWS)
#include "win/ThreadImpl.h"
#endif
