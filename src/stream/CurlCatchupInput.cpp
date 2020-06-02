/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CurlCatchupInput.h"

#include "../utils/Log.h"

using namespace ffmpegdirect;

void CurlCatchupInput::Reset()
{
  if (m_pFile)
  {
    Log(LOGLEVEL_DEBUG, "%s - Closing and opening stream", __FUNCTION__);
    Close();    
    Open(m_filename, m_mimeType, m_flags);
  }
}
