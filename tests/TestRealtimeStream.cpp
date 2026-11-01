/*
 *  Copyright (C) 2026 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include <ctime>
#include <iostream>

#include "../src/utils/Properties.h"

int main()
{
  using ffmpegdirect::Properties;
  using ffmpegdirect::StreamMode;

  struct TestCase
  {
    StreamMode mode;
    bool defaultRealtime;
    const char* name;
  };

  const TestCase cases[] = {
      {StreamMode::NONE, false, "regular"},
      {StreamMode::CATCHUP, false, "catchup"},
      {StreamMode::TIMESHIFT, true, "timeshift"},
  };
  int failures = 0;
  for (const auto& test : cases)
  {
    Properties props;
    props.m_streamMode = test.mode;
    const auto expect = [&](bool expected, const char* overrideName)
    {
      if (props.IsRealTimeStream() != expected)
      {
        std::cerr << test.name << ": " << overrideName << " realtime override failed\n";
        ++failures;
      }
    };

    expect(test.defaultRealtime, "omitted");
    props.m_isRealTimeStream = true;
    expect(true, "true");
    props.m_isRealTimeStream = false;
    expect(false, "false");
  }
  if (failures == 0)
    std::cout << "All 9 realtime policy cases passed\n";
  return failures == 0 ? 0 : 1;
}
