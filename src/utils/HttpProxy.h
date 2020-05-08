/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>

namespace ffmpegdirect
{
  class HttpProxy
  {
  public:
    HttpProxy() {};
    HttpProxy(const std::string& host, uint16_t port, const std::string& user, const std::string& password)
      : m_host(host), m_port(port), m_user(user), m_password(password) {};

    const std::string& GetProxyHost() const { return m_host; }
    void SetProxyHost(const std::string& value) { m_host = value; }

    uint16_t GetProxyPort() const { return m_port; }
    void SetProxyPort(uint16_t value) { m_port = value; }

    const std::string& GetProxyUser() const { return m_user; }
    void SetProxyUser(const std::string& value) { m_user = value; }

    const std::string& GetProxyPassword() const { return m_password; }
    void SetProxyPassword(const std::string& value) { m_password = value; }

  private:
    std::string m_host;
    uint16_t m_port;
    std::string m_user;
    std::string m_password;
  };
} //namespace ffmpegdirect