/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

class IRunnable
{
public:
  virtual void Run()=0;
  virtual void Cancel() {};
  virtual ~IRunnable() = default;
};
