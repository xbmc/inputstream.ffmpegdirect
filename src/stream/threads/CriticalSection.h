/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "platform/RecursiveMutex.h"
#include "Lockables.h"

class CCriticalSection : public XbmcThreads::CountingLockable<XbmcThreads::CRecursiveMutex> {};
