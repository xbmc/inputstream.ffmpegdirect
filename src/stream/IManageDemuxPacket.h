/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <kodi/addon-instance/Inputstream.h>

class IManageDemuxPacket
{
public:
  virtual ~IManageDemuxPacket() = default;

  virtual DemuxPacket* AllocateDemuxPacketFromInputStreamAPI(int dataSize) = 0;
  virtual DemuxPacket* AllocateEncryptedDemuxPacketFromInputStreamAPI(int dataSize, unsigned int encryptedSubsampleCount) = 0;
  virtual void FreeDemuxPacketFromInputStreamAPI(DemuxPacket* packet) = 0;
};