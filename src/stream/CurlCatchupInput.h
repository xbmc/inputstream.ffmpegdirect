/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "CurlInput.h"
#include "../utils/Log.h"

namespace ffmpegdirect
{

class CurlCatchupInput : public CurlInput
{
public:
  void Reset() override;
};

} //namespace ffmpegdirect