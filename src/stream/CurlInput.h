/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <kodi/Filesystem.h>

#define CACHE_BUFFER_MODE_INTERNET      0
#define CACHE_BUFFER_MODE_ALL           1
#define CACHE_BUFFER_MODE_TRUE_INTERNET 2
#define CACHE_BUFFER_MODE_NONE          3
#define CACHE_BUFFER_MODE_REMOTE        4

#define SEEK_POSSIBLE 0x10 // flag used to check if protocol allows seeks

namespace ffmpegdirect
{

class CurlInput
{
public:
  explicit CurlInput();
  ~CurlInput();
  bool Open(const std::string& filename, const std::string& mimeType, unsigned int flags);
  void Close();
  int Read(uint8_t* buf, int buf_size);
  int64_t Seek(int64_t offset, int whence);
  bool IsEOF();
  int64_t GetLength();
  int GetBlockSize();
  std::string& GetContent() { return m_content; };
  std::string& GetFilename() { return m_filename; };
  void SetFilename(const std::string& filename) { m_filename = filename; };
  virtual void Reset() {};

protected:
  kodi::vfs::CFile* m_pFile = nullptr;
  bool m_eof = false;
  std::string m_filename;
  std::string m_mimeType;
  unsigned int m_flags = 0;
  std::string m_content;
};

} //namespace ffmpegdirect