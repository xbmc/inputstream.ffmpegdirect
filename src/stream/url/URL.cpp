/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "URL.h"
#include "../../utils/Log.h"

//#include "utils/URIUtils.h"
// #include "Util.h"
// #include "filesystem/File.h"
// #include "FileItem.h"
// #include "filesystem/StackDirectory.h"
// #include "network/Network.h"
// #include "ServiceBroker.h"
#ifndef TARGET_POSIX
#include <sys\stat.h>
#endif

#include <string>
#include <vector>

#include <kodi/tools/StringUtils.h>

using namespace kodi::tools;
//using namespace ADDON;

namespace
{

bool IsURL(const std::string& strFile)
{
  return strFile.find("://") != std::string::npos;
}

bool IsDOSPath(const std::string& path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  // windows network drives
  if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
    return true;

  return false;
}

std::string ValidatePath(const std::string &path)//, bool bFixDoubleSlashes /* = false */)
{
  std::string result = path;

  // Don't do any stuff on URLs containing %-characters or protocols that embed
  // filenames. NOTE: Don't use IsInZip or IsInRar here since it will infinitely
  // recurse and crash XBMC
  if (IsURL(path) && path.find('%') != std::string::npos)
    return result;

  // check the path for incorrect slashes
#ifdef TARGET_WINDOWS
  if (IsDOSPath(path))
  {
    StringUtils::Replace(result, '/', '\\');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    // if (bFixDoubleSlashes && !result.empty())
    // {
    //   // Fixup for double back slashes (but ignore the \\ of unc-paths)
    //   for (size_t x = 1; x < result.size() - 1; x++)
    //   {
    //     if (result[x] == '\\' && result[x+1] == '\\')
    //       result.erase(x, 1);
    //   }
    // }
  }
  else if (path.find("://") != std::string::npos || path.find(":\\\\") != std::string::npos)
#endif
  {
    StringUtils::Replace(result, '\\', '/');
    /* The double slash correction should only be used when *absolutely*
       necessary! This applies to certain DLLs or use from Python DLLs/scripts
       that incorrectly generate double (back) slashes.
    */
    // if (bFixDoubleSlashes && !result.empty())
    // {
    //   // Fixup for double forward slashes(/) but don't touch the :// of URLs
    //   for (size_t x = 2; x < result.size() - 1; x++)
    //   {
    //     if ( result[x] == '/' && result[x + 1] == '/' && !(result[x - 1] == ':' || (result[x - 1] == '/' && result[x - 2] == ':')) )
    //       result.erase(x, 1);
    //   }
    // }
  }
  return result;
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
std::string URIGetFileName(const std::string& strFileNameAndPath)
{
  if(IsURL(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    return URIGetFileName(url.GetFileName());
  }

  /* find the last slash */
  const size_t slash = strFileNameAndPath.find_last_of("/\\");
  return strFileNameAndPath.substr(slash+1);
}

std::string URIGetFileName(const CURL& url)
{
  return URIGetFileName(url.GetFileName());
}

bool HasSlashAtEnd(const std::string& strFile)
{
  return HasSlashAtEnd(strFile);
}

bool HasSlashAtEnd(const std::string& strFile, bool checkURL /* = false */)
{
  if (strFile.empty()) return false;
  if (checkURL && IsURL(strFile))
  {
    CURL url(strFile);
    std::string file = url.GetFileName();
    return file.empty() || HasSlashAtEnd(file, false);
  }
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void RemoveSlashAtEnd(std::string& strFolder)
{
  if (IsURL(strFolder))
  {
    CURL url(strFolder);
    std::string file = url.GetFileName();
    if (!file.empty() && file != strFolder)
    {
      RemoveSlashAtEnd(file);
      url.SetFileName(file);
      strFolder = url.Get();
      return;
    }
    if(url.GetHostName().empty())
      return;
  }

  while (HasSlashAtEnd(strFolder))
    strFolder.erase(strFolder.size()-1, 1);
}

} // unnames namespace

CURL::~CURL() = default;

void CURL::Reset()
{
  m_strHostName.clear();
  m_strDomain.clear();
  m_strUserName.clear();
  m_strPassword.clear();
  m_strShareName.clear();
  m_strFileName.clear();
  m_strProtocol.clear();
  m_strFileType.clear();
  m_strOptions.clear();
  m_strProtocolOptions.clear();
  m_options.Clear();
  m_protocolOptions.Clear();
  m_iPort = 0;
}

void CURL::Parse(const std::string& strURL1)
{
  Reset();
  // start by validating the path
  std::string strURL = ValidatePath(strURL1);

  // strURL can be one of the following:
  // format 1: protocol://[username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file
  // format 3: drive:directoryandfile
  //
  // first need 2 check if this is a protocol or just a normal drive & path
  if (!strURL.size()) return ;
  if (strURL == "?") return;

  // form is format 1 or 2
  // format 1: protocol://[domain;][username:password]@hostname[:port]/directoryandfile
  // format 2: protocol://file

  // decode protocol
  size_t iPos = strURL.find("://");
  if (iPos == std::string::npos)
  {
    // This is an ugly hack that needs some work.
    // example: filename /foo/bar.zip/alice.rar/bob.avi
    // This should turn into zip://rar:///foo/bar.zip/alice.rar/bob.avi
    iPos = 0;
    bool is_apk = (strURL.find(".apk/", iPos) != std::string::npos);
    while (true)
    {
      if (is_apk)
        iPos = strURL.find(".apk/", iPos);
      else
        iPos = strURL.find(".zip/", iPos);

      int extLen = 3;
      if (iPos == std::string::npos)
      {
        /* set filename and update extension*/
        SetFileName(strURL);
        return ;
      }
      iPos += extLen + 1;
//       std::string archiveName = strURL.substr(0, iPos);
//       struct __stat64 s;
//       if (XFILE::CFile::Stat(archiveName, &s) == 0)
//       {
// #ifdef TARGET_POSIX
//         if (!S_ISDIR(s.st_mode))
// #else
//         if (!(s.st_mode & S_IFDIR))
// #endif
//         {
//           archiveName = Encode(archiveName);
//           if (is_apk)
//           {
//             CURL c("apk://" + archiveName + "/" + strURL.substr(iPos + 1));
//             *this = c;
//           }
//           else
//           {
//             CURL c("zip://" + archiveName + "/" + strURL.substr(iPos + 1));
//             *this = c;
//           }
//           return;
//         }
//       }
    }
  }
  else
  {
    SetProtocol(strURL.substr(0, iPos));
    iPos += 3;
  }

  // virtual protocols
  // why not handle all format 2 (protocol://file) style urls here?
  // ones that come to mind are iso9660, cdda, musicdb, etc.
  // they are all local protocols and have no server part, port number, special options, etc.
  // this removes the need for special handling below.
  if (
    IsProtocol("stack") ||
    IsProtocol("virtualpath") ||
    IsProtocol("multipath") ||
    IsProtocol("special") ||
    IsProtocol("resource")
    )
  {
    SetFileName(strURL.substr(iPos));
    return;
  }

  if (IsProtocol("udf"))
  {
    std::string lower(strURL);
    StringUtils::ToLower(lower);
    size_t isoPos = lower.find(".iso\\", iPos);
    if (isoPos == std::string::npos)
      isoPos = lower.find(".udf\\", iPos);
    if (isoPos != std::string::npos)
    {
      strURL = strURL.replace(isoPos + 4, 1, "/");
    }
  }

  // check for username/password - should occur before first /
  if (iPos == std::string::npos) iPos = 0;

  // for protocols supporting options, chop that part off here
  // maybe we should invert this list instead?
  size_t iEnd = strURL.length();
  const char* sep = NULL;

  //! @todo fix all Addon paths
  std::string strProtocol2 = GetTranslatedProtocol();
  if(IsProtocol("rss") ||
     IsProtocol("rsss") ||
     IsProtocol("rar") ||
     IsProtocol("apk") ||
     IsProtocol("xbt") ||
     IsProtocol("zip") ||
     IsProtocol("addons") ||
     IsProtocol("image") ||
     IsProtocol("videodb") ||
     IsProtocol("musicdb") ||
     IsProtocol("androidapp") ||
     IsProtocol("pvr"))
    sep = "?";
  else
  if(  IsProtocolEqual(strProtocol2, "http")
    || IsProtocolEqual(strProtocol2, "https")
    || IsProtocolEqual(strProtocol2, "plugin")
    || IsProtocolEqual(strProtocol2, "addons")
    || IsProtocolEqual(strProtocol2, "rtsp"))
    sep = "?;#|";
  else if(IsProtocolEqual(strProtocol2, "ftp")
       || IsProtocolEqual(strProtocol2, "ftps"))
    sep = "?;|";

  if(sep)
  {
    size_t iOptions = strURL.find_first_of(sep, iPos);
    if (iOptions != std::string::npos)
    {
      // we keep the initial char as it can be any of the above
      size_t iProto = strURL.find_first_of("|",iOptions);
      if (iProto != std::string::npos)
      {
        SetProtocolOptions(strURL.substr(iProto+1));
        SetOptions(strURL.substr(iOptions,iProto-iOptions));
      }
      else
        SetOptions(strURL.substr(iOptions));
      iEnd = iOptions;
    }
  }

  size_t iSlash = strURL.find("/", iPos);
  if(iSlash >= iEnd)
    iSlash = std::string::npos; // was an invalid slash as it was contained in options

  // also skip parsing username:password@ for udp/rtp as it not valid
  // and conflicts with the following example: rtp://sourceip@multicastip
  if (!IsProtocol("iso9660") && !IsProtocol("udp") && !IsProtocol("rtp"))
  {
    size_t iAlphaSign = strURL.find("@", iPos);
    if (iAlphaSign != std::string::npos && iAlphaSign < iEnd && (iAlphaSign < iSlash || iSlash == std::string::npos))
    {
      // username/password found
      std::string strUserNamePassword = strURL.substr(iPos, iAlphaSign - iPos);

      // first extract domain, if protocol is smb
      if (IsProtocol("smb"))
      {
        size_t iSemiColon = strUserNamePassword.find(";");

        if (iSemiColon != std::string::npos)
        {
          m_strDomain = strUserNamePassword.substr(0, iSemiColon);
          strUserNamePassword.erase(0, iSemiColon + 1);
        }
      }

      // username:password
      size_t iColon = strUserNamePassword.find(":");
      if (iColon != std::string::npos)
      {
        m_strUserName = strUserNamePassword.substr(0, iColon);
        m_strPassword = strUserNamePassword.substr(iColon + 1);
      }
      // username
      else
      {
        m_strUserName = strUserNamePassword;
      }

      iPos = iAlphaSign + 1;
      iSlash = strURL.find("/", iAlphaSign);

      if(iSlash >= iEnd)
        iSlash = std::string::npos;
    }
  }

  std::string strHostNameAndPort = strURL.substr(iPos, (iSlash == std::string::npos) ? iEnd - iPos : iSlash - iPos);
  // check for IPv6 numerical representation inside [].
  // if [] found, let's store string inside as hostname
  // and remove that parsed part from strHostNameAndPort
  size_t iBrk = strHostNameAndPort.rfind("]");
  if (iBrk != std::string::npos && strHostNameAndPort.find("[") == 0)
  {
    m_strHostName = strHostNameAndPort.substr(1, iBrk-1);
    strHostNameAndPort.erase(0, iBrk+1);
  }

  // detect hostname:port/ or just :port/ if previous step found [IPv6] format
  size_t iColon = strHostNameAndPort.rfind(":");
  if (iColon != std::string::npos && iColon == strHostNameAndPort.find(":"))
  {
    if (m_strHostName.empty())
      m_strHostName = strHostNameAndPort.substr(0, iColon);
    m_iPort = atoi(strHostNameAndPort.substr(iColon + 1).c_str());
  }

  // if we still don't have hostname, the strHostNameAndPort substring
  // is 'just' hostname without :port specification - so use it as is.
  if (m_strHostName.empty())
    m_strHostName = strHostNameAndPort;

  if (iSlash != std::string::npos)
  {
    iPos = iSlash + 1;
    if (iEnd > iPos)
      m_strFileName = strURL.substr(iPos, iEnd - iPos);
  }

  // iso9960 doesnt have an hostname;-)
  if (IsProtocol("iso9660")
   || IsProtocol("musicdb")
   || IsProtocol("videodb")
   || IsProtocol("sources")
   || IsProtocol("pvr"))
  {
    if (m_strHostName != "" && m_strFileName != "")
    {
      m_strFileName = StringUtils::Format("%s/%s", m_strHostName.c_str(), m_strFileName.c_str());
      m_strHostName = "";
    }
    else
    {
      if (!m_strHostName.empty() && strURL[iEnd-1]=='/')
        m_strFileName = m_strHostName + "/";
      else
        m_strFileName = m_strHostName;
      m_strHostName = "";
    }
  }

  StringUtils::Replace(m_strFileName, '\\', '/');

  /* update extension + sharename */
  SetFileName(m_strFileName);

  /* decode urlencoding on this stuff */
  // if(URIUtils::HasEncodedHostname(*this))
  // {
  //   m_strHostName = Decode(m_strHostName);
  //   SetHostName(m_strHostName);
  // }

  m_strUserName = Decode(m_strUserName);
  m_strPassword = Decode(m_strPassword);
}

void CURL::SetFileName(const std::string& strFileName)
{
  m_strFileName = strFileName;

  size_t slash = m_strFileName.find_last_of(GetDirectorySeparator());
  size_t period = m_strFileName.find_last_of('.');
  if(period != std::string::npos && (slash == std::string::npos || period > slash))
    m_strFileType = m_strFileName.substr(period+1);
  else
    m_strFileType = "";

  slash = m_strFileName.find_first_of(GetDirectorySeparator());
  if(slash == std::string::npos)
    m_strShareName = m_strFileName;
  else
    m_strShareName = m_strFileName.substr(0, slash);

  StringUtils::Trim(m_strFileType);
  StringUtils::ToLower(m_strFileType);
}

void CURL::SetProtocol(const std::string& strProtocol)
{
  m_strProtocol = strProtocol;
  StringUtils::ToLower(m_strProtocol);
}

void CURL::SetOptions(const std::string& strOptions)
{
  m_strOptions.clear();
  m_options.Clear();
  if( strOptions.length() > 0)
  {
    if(strOptions[0] == '?' ||
       strOptions[0] == '#' ||
       strOptions[0] == ';' ||
       strOptions.find("xml") != std::string::npos)
    {
      m_strOptions = strOptions;
      m_options.AddOptions(m_strOptions);
    }
    else
      Log(LOGLEVEL_WARNING, "%s - Invalid options specified for url %s", __FUNCTION__, strOptions.c_str());
  }
}

void CURL::SetProtocolOptions(const std::string& strOptions)
{
  m_strProtocolOptions.clear();
  m_protocolOptions.Clear();
  if (strOptions.length() > 0)
  {
    if (strOptions[0] == '|')
      m_strProtocolOptions = strOptions.substr(1);
    else
      m_strProtocolOptions = strOptions;
    m_protocolOptions.AddOptions(m_strProtocolOptions);
  }
}

const std::string CURL::GetTranslatedProtocol() const
{
  if (IsProtocol("shout")
   || IsProtocol("dav")
   || IsProtocol("rss"))
    return "http";

  if (IsProtocol("davs")
   || IsProtocol("rsss"))
    return "https";

  return GetProtocol();
}

const std::string CURL::GetFileNameWithoutPath() const
{
  // *.zip and *.rar store the actual zip/rar path in the hostname of the url
  // if ((IsProtocol("rar")  ||
  //      IsProtocol("zip")  ||
  //      IsProtocol("xbt")  ||
  //      IsProtocol("apk")) &&
  //      m_strFileName.empty())
  //   return URIUtils::GetFileName(m_strHostName);

  // otherwise, we've already got the filepath, so just grab the filename portion
  std::string file(m_strFileName);
  RemoveSlashAtEnd(file);
  return URIGetFileName(file);
}

inline
void protectIPv6(std::string &hn)
{
  if (!hn.empty() && hn.find(":") != hn.rfind(":")
   && hn.find(":") != std::string::npos)
  {
    hn = '[' + hn + ']';
  }
}

char CURL::GetDirectorySeparator() const
{
#ifndef TARGET_POSIX
  //We don't want to use IsLocal here, it can return true
  //for network protocols that matches localhost or hostname
  //we only ever want to use \ for win32 local filesystem
  if ( m_strProtocol.empty() )
    return '\\';
  else
#endif
    return '/';
}

std::string CURL::Get() const
{
  if (m_strProtocol.empty())
    return m_strFileName;

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  std::string strURL;
  strURL.reserve(sizeneed);

  strURL = GetWithoutOptions();

  if( !m_strOptions.empty() )
    strURL += m_strOptions;

  if (!m_strProtocolOptions.empty())
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutOptions() const
{
  if (m_strProtocol.empty())
    return m_strFileName;

  std::string strGet = GetWithoutFilename();

  // Prevent double slash when concatenating host part and filename part
  if (m_strFileName.size() && (m_strFileName[0] == '/' || m_strFileName[0] == '\\') && HasSlashAtEnd(strGet))
    RemoveSlashAtEnd(strGet);

  return strGet + m_strFileName;
}

std::string CURL::GetWithoutUserDetails(bool redact) const
{
  std::string strURL;

  // if (IsProtocol("stack"))
  // {
  //   CFileItemList items;
  //   XFILE::CStackDirectory dir;
  //   dir.GetDirectory(*this,items);
  //   std::vector<std::string> newItems;
  //   for (int i=0;i<items.Size();++i)
  //   {
  //     CURL url(items[i]->GetPath());
  //     items[i]->SetPath(url.GetWithoutUserDetails(redact));
  //     newItems.push_back(items[i]->GetPath());
  //   }
  //   dir.ConstructStackPath(newItems, strURL);
  //   return strURL;
  // }

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strHostName.length()
                        + m_strFileName.length()
                        + m_strOptions.length()
                        + m_strProtocolOptions.length()
                        + 10;

  if (redact && !m_strUserName.empty())
  {
    sizeneed += sizeof("USERNAME");
    if (!m_strPassword.empty())
      sizeneed += sizeof(":PASSWORD@");
    if (!m_strDomain.empty())
      sizeneed += sizeof("DOMAIN;");
  }

  strURL.reserve(sizeneed);

  if (m_strProtocol.empty())
    return m_strFileName;

  strURL = m_strProtocol;
  strURL += "://";

  if (redact && !m_strUserName.empty())
  {
    if (!m_strDomain.empty())
      strURL += "DOMAIN;";
    strURL += "USERNAME";
    if (!m_strPassword.empty())
      strURL += ":PASSWORD";
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string strHostName;

    // if (URIUtils::HasParentInHostname(*this))
    //   strHostName = CURL(m_strHostName).GetWithoutUserDetails();
    // else
      strHostName = m_strHostName;

    // if (URIUtils::HasEncodedHostname(*this))
    strHostName = Encode(strHostName);

    if ( HasPort() )
    {
      protectIPv6(strHostName);
      strURL += strHostName + StringUtils::Format(":%i", m_iPort);
    }
    else
      strURL += strHostName;

    strURL += "/";
  }
  strURL += m_strFileName;

  if( m_strOptions.length() > 0 )
    strURL += m_strOptions;
  if( m_strProtocolOptions.length() > 0 )
    strURL += "|"+m_strProtocolOptions;

  return strURL;
}

std::string CURL::GetWithoutFilename() const
{
  if (m_strProtocol.empty())
    return "";

  unsigned int sizeneed = m_strProtocol.length()
                        + m_strDomain.length()
                        + m_strUserName.length()
                        + m_strPassword.length()
                        + m_strHostName.length()
                        + 10;

  std::string strURL;
  strURL.reserve(sizeneed);

  strURL = m_strProtocol;
  strURL += "://";

  if (!m_strUserName.empty())
  {
    if (!m_strDomain.empty())
    {
      strURL += Encode(m_strDomain);
      strURL += ";";
    }
    strURL += Encode(m_strUserName);
    if (!m_strPassword.empty())
    {
      strURL += ":";
      strURL += Encode(m_strPassword);
    }
    strURL += "@";
  }

  if (!m_strHostName.empty())
  {
    std::string hostname;

    // if( URIUtils::HasEncodedHostname(*this) )
    //   hostname = Encode(m_strHostName);
    // else
      hostname = m_strHostName;

    if (HasPort())
    {
      protectIPv6(hostname);
      strURL += hostname + StringUtils::Format(":%i", m_iPort);
    }
    else
      strURL += hostname;

    strURL += "/";
  }

  return strURL;
}

std::string CURL::GetRedacted() const
{
  return GetWithoutUserDetails(true);
}

std::string CURL::GetRedacted(const std::string& path)
{
  return CURL(path).GetRedacted();
}

// bool CURL::IsLocal() const
// {
//   return (m_strProtocol.empty() || IsLocalHost() || IsProtocol("win-lib"));
// }

// bool CURL::IsLocalHost() const
// {
//   return CServiceBroker::GetNetwork().IsLocalHost(m_strHostName);
// }

bool CURL::IsFileOnly(const std::string &url)
{
  return url.find_first_of("/\\") == std::string::npos;
}

bool CURL::IsFullPath(const std::string &url)
{
  if (url.size() && url[0] == '/') return true;     //   /foo/bar.ext
  if (url.find("://") != std::string::npos) return true;                 //   foo://bar.ext
  if (url.size() > 1 && url[1] == ':') return true; //   c:\\foo\\bar\\bar.ext
  if (StringUtils::StartsWith(url, "\\\\")) return true;    //   \\UNC\path\to\file
  return false;
}

std::string CURL::Decode(const std::string& strURLData)
//modified to be more accommodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  std::string strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        std::string strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp.c_str(), "%x", (unsigned int *)&dec_num);
        if (dec_num<0 || dec_num>255)
          strResult += kar;
        else
        {
          strResult += (char)dec_num;
          i += 2;
        }
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }

  return strResult;
}

std::string CURL::Encode(const std::string& strURLData)
{
  std::string strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (size_t i = 0; i < strURLData.size(); ++i)
  {
    const char kar = strURLData[i];

    // Don't URL encode "-_.!()" according to RFC1738
    //! @todo Update it to "-_.~" after Gotham according to RFC3986
    if (StringUtils::IsAsciiAlphaNum(kar) || kar == '-' || kar == '.' || kar == '_' || kar == '!' || kar == '(' || kar == ')')
      strResult.push_back(kar);
    else
      strResult += StringUtils::Format("%%%2.2x", (unsigned int)((unsigned char)kar));
  }

  return strResult;
}

bool CURL::IsProtocolEqual(const std::string &protocol, const char *type)
{
  /*
   NOTE: We're currently using == here as m_strProtocol is assigned as lower-case in SetProtocol(),
   and we've assumed all other callers are calling with protocol lower-case otherwise.
   We possibly shouldn't do this (as CURL(foo).Get() != foo, though there are other reasons for this as well)
   but it handles the requirements of RFC-1738 which allows the scheme to be case-insensitive.
   */
  if (type)
    return protocol == type;
  return false;
}

void CURL::GetOptions(std::map<std::string, std::string> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_options.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)
    options[option->first] = option->second.asString();
}

bool CURL::HasOption(const std::string &key) const
{
  return m_options.HasOption(key);
}

bool CURL::GetOption(const std::string &key, std::string &value) const
{
  CVariant valueObj;
  if (!m_options.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

std::string CURL::GetOption(const std::string &key) const
{
  std::string value;
  if (!GetOption(key, value))
    return "";

  return value;
}

void CURL::SetOption(const std::string &key, const std::string &value)
{
  m_options.AddOption(key, value);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::RemoveOption(const std::string &key)
{
  m_options.RemoveOption(key);
  SetOptions(m_options.GetOptionsString(true));
}

void CURL::GetProtocolOptions(std::map<std::string, std::string> &options) const
{
  CUrlOptions::UrlOptions optionsMap = m_protocolOptions.GetOptions();
  for (CUrlOptions::UrlOptions::const_iterator option = optionsMap.begin(); option != optionsMap.end(); option++)
    options[option->first] = option->second.asString();
}

bool CURL::HasProtocolOption(const std::string &key) const
{
  return m_protocolOptions.HasOption(key);
}

bool CURL::GetProtocolOption(const std::string &key, std::string &value) const
{
  CVariant valueObj;
  if (!m_protocolOptions.GetOption(key, valueObj))
    return false;

  value = valueObj.asString();
  return true;
}

std::string CURL::GetProtocolOption(const std::string &key) const
{
  std::string value;
  if (!GetProtocolOption(key, value))
    return "";

  return value;
}

void CURL::SetProtocolOption(const std::string &key, const std::string &value)
{
  m_protocolOptions.AddOption(key, value);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}

void CURL::RemoveProtocolOption(const std::string &key)
{
  m_protocolOptions.RemoveOption(key);
  m_strProtocolOptions = m_protocolOptions.GetOptionsString(false);
}
