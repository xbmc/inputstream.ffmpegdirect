/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <atomic>

class CAtomicSpinLock
{
public:
  explicit CAtomicSpinLock(std::atomic_flag& lock);
  ~CAtomicSpinLock();
private:
  std::atomic_flag& m_Lock;
};

