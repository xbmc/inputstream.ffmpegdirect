/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CurlInput.h"

// #include "ServiceBroker.h"
// #include "filesystem/File.h"
// #include "filesystem/IFile.h"
// #include "settings/AdvancedSettings.h"
// #include "settings/SettingsComponent.h"
// #include "utils/URIUtils.h"
#include "../utils/Log.h"

using namespace ffmpegdirect;

CurlInput::CurlInput()
{
  m_pFile = nullptr;
  m_eof = true;
}

CurlInput::~CurlInput()
{
  Close();
}

bool CurlInput::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CurlInput::Open(const std::string& filename, const std::string& mimeType, unsigned int flags)
{
  m_filename = filename;
  m_mimeType = mimeType;
  m_flags = flags;

  m_pFile = new kodi::vfs::CFile();
  if (!m_pFile)
    return false;

  flags |= ADDON_READ_AUDIO_VIDEO;

  /*
   * There are 5 buffer modes available (configurable in as.xml)
   * 0) Buffer all internet filesystems (like 2 but additionally also ftp, webdav, etc.) (default)
   * 1) Buffer all filesystems (including local)
   * 2) Only buffer true internet filesystems (streams) (http, etc.)
   * 3) No buffer
   * 4) Buffer all non-local (remote) filesystems
   */
  // if (!URIUtils::IsOnDVD(m_item.GetDynPath()) && !URIUtils::IsBluray(m_item.GetDynPath())) // Never cache these
  // {
    // TODO: Support other buffer modes once IsLocalHost() is available for addons in koid::network
    //  if ((iCacheBufferMode == CACHE_BUFFER_MODE_INTERNET) && URIUtils::IsInternetStream(m_item.GetDynPath(), true))
    //  || (iCacheBufferMode == CACHE_BUFFER_MODE_TRUE_INTERNET && URIUtils::IsInternetStream(m_item.GetDynPath(), false))
    //  || (iCacheBufferMode == CACHE_BUFFER_MODE_REMOTE && URIUtils::IsRemote(m_item.GetDynPath()))
    //  || (iCacheBufferMode == CACHE_BUFFER_MODE_ALL))
    unsigned int iCacheBufferMode = CACHE_BUFFER_MODE_INTERNET; //CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cacheBufferMode;
    if (iCacheBufferMode == CACHE_BUFFER_MODE_INTERNET)
    {
      flags |= ADDON_READ_CACHED;
    }
  //}

  if (!(flags & ADDON_READ_CACHED))
    flags |= ADDON_READ_NO_CACHE; // Make sure CFile honors our no-cache hint

  std::string content = m_mimeType;

  if (content == "video/mp4" ||
      content == "video/x-msvideo" ||
      content == "video/avi" ||
      content == "video/x-matroska" ||
      content == "video/x-matroska-3d")
    flags |= ADDON_READ_MULTI_STREAM;

  // open file in binary mode
  if (!m_pFile->OpenFile(m_filename, flags))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }

  if ((content.empty() || content == "application/octet-stream"))
    m_content = m_pFile->GetPropertyValue(ADDON_FILE_PROPERTY_CONTENT_TYPE, "");

  m_eof = false;
  return true;
}

// close file and reset everything
void CurlInput::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  //CDVDInputStream::Close();
  m_pFile = NULL;
  m_eof = true;
}

int CurlInput::Read(uint8_t* buf, int buf_size)
{
  if (!m_pFile) return -1;

  ssize_t ret = m_pFile->Read(buf, buf_size);

  if (ret < 0)
    return -1; // player will retry read in case of error until playback is stopped

  /* we currently don't support non completing reads */
  if (ret == 0)
    m_eof = true;

  return (int)ret;
}

int64_t CurlInput::Seek(int64_t offset, int whence)
{
  if (!m_pFile) return -1;

  // TODO: Add once IoControl is available for addons in koid::vfs
  // if (whence == SEEK_POSSIBLE)
  //   return m_pFile->IoControl(VFS_IOCTRL_SEEK_POSSIBLE, nullptr);

  int64_t ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

int64_t CurlInput::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}

int CurlInput::GetBlockSize()
{
  if(m_pFile)
    return m_pFile->GetChunkSize();
  else
    return 0;
}

