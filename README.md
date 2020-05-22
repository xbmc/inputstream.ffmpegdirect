[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](LICENSE.md)
[![Build Status](https://travis-ci.org/xbmc/inputstream.ffmpegdirect.svg?branch=Matrix)](https://travis-ci.org/xbmc/inputstream.ffmpegdirect/branches)
[![Build Status](https://dev.azure.com/teamkodi/binary-addons/_apis/build/status/xbmc.inputstream.ffmpegdirect?branchName=Matrix)](https://dev.azure.com/teamkodi/binary-addons/_build/latest?definitionId=30&branchName=Matrix)
[![Build Status](https://jenkins.kodi.tv/view/Addons/job/xbmc/job/inputstream.ffmpegdirect/job/Matrix/badge/icon)](https://jenkins.kodi.tv/blue/organizations/jenkins/xbmc%2Finputstream.ffmpegdirect/branches/)

# inputstream.ffmpegdirect addon for Kodi

This is a [Kodi](https://kodi.tv) input stream addon for streams that can be opened by either FFmpeg's libavformat or Kodi's cURL. Common stream formats such as plain TS, HLS and DASH are supported as well as many others. Note that the only DASH streams supported are those without DRM.

The addon also has support for Archive/Catchup services where there is a replay window (usually in days) and can timeshift across that span.

## Build instructions

### Linux

1. `git clone --branch master https://github.com/xbmc/xbmc.git`
2. `git clone https://github.com/xbmc/inputstream.ffmpegdirect.git`
3. `cd inputstream.ffmpegdirect && mkdir build && cd build`
4. `cmake -DADDONS_TO_BUILD=inputstream.ffmpegdirect -DADDON_SRC_PREFIX=../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../../xbmc/build/addons -DPACKAGE_ZIP=1 ../../xbmc/cmake/addons`
5. `make`

The addon files will be placed in `../../xbmc/build/addons` so if you build Kodi from source and run it directly the addon will be available as a system addon.

### Mac OSX

In order to build the addon on mac the steps are different to Linux and Windows as the cmake command above will not produce an addon that will run in kodi. Instead using make directly as per the supported build steps for kodi on mac we can build the tools and just the addon on it's own. Following this we copy the addon into kodi. Note that we checkout kodi to a separate directory as this repo will only only be used to build the addon and nothing else.

#### Build tools and initial addon build

1. Get the repos
 * `cd $HOME`
 * `git clone https://github.com/xbmc/xbmc xbmc-addon`
 * `git clone https://github.com/xbmc/inputstream.ffmpegdirect`
2. Build the kodi tools
 * `cd $HOME/xbmc-addon/tools/depends`
 * `./bootstrap`
 * `./configure --host=x86_64-apple-darwin`
 * `make -j$(getconf _NPROCESSORS_ONLN)`
3. Build the addon
 * `cd $HOME/xbmc-addon`
 * `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="inputstream.ffmpegdirect" ADDON_SRC_PREFIX=$HOME`

Note that the steps in the following section need to be performed before the addon is installed and you can run it in Kodi.

#### To rebuild the addon and copy to kodi after changes (after the initial addon build)

1. `cd $HOME/inputstream.ffmpegdirect`
2. `./build-install-mac.sh ../xbmc-addon`

If you would prefer to run the rebuild steps manually instead of using the above helper script check the appendix [here](#manual-steps-to-rebuild-the-addon-on-macosx)

## Settings

### FFmpeg HTTP Proxy
Contains the settings for how the proxy is configured when opening with FFmpeg. Note that the setting has no effect when opening using Kodi's cURL. In that case it will use the proxy configured in Kodi.

* **Use HTTP proxy when opening with FFmpeg**: Whether or not a proxy should be used when opening with FFmpeg.
* **Server**: Configure the proxy server address.
* **Port**: Configure the proxy server port.
* **Username**: Configure the proxy server username.
* **Password**: Configure the proxy server password.

### Timeshift
This category contains the settings for timeshift. Timeshifting allows you to pause live TV as well as move back and forward from your current position similar to playing back a recording.

* **Timeshift buffer path**: The path used to store the timeshift buffer. The default is the `addon_data/inputstream.ffmpegdirect/timeshift` folder in userdata. Note that this folder will be cleared of timeshift files on Kodi startup. Only relevant when `inputstream.ffmpegdirect.stream_mode=timeshift" property is passed to the addon.

## Using the addon

The addon can be accessed like any other inputstream in Kodi. The following example will show how to manually choose this addon for playback when using IPTV Simple Client with the following entry in the M3U file (Note that the IPTV Simple Client will in fact automatcially detect a catchup stream and use the addon based on configured settings).

```
#KODIPROP:inputstream=inputstream.ffmpegdirect
#KODIPROP:mimetype=video/mp2t
#KODIPROP:inputstream.ffmpegdirect.program_number=2154
#KODIPROP:inputstream.ffmpegdirect.is_realtime_stream=true
#EXTINF:-1,MyChannel
http://127.0.0.1:3002/mystream.ts
```

Note that the appropriate mime type should always be set. Here are the some common ones:
- TS: `video/mp2t`
- HLS: `application/x-mpegURL` or `application/vnd.apple.mpegurl`
- Dash: `application/dash+xml`

The field `program_number` can be used to indicate the Program ID to use for TS streams.

If enabling **timeshift** support for live streams here is an example set of properties.

```
#KODIPROP:inputstream=inputstream.ffmpegdirect
#KODIPROP:mimetype=application/x-mpegURL
#KODIPROP:inputstream.ffmpegdirect.is_realtime_stream=true
#KODIPROP:inputstream.ffmpegdirect.stream_mode=timeshift
#KODIPROP:inputstream.ffmpegdirect.manifest_type=hls
#EXTINF:-1,MyChannel
http://127.0.0.1:3002/mystream.m3u8
```

If enabling **archive/catchup** support there are a number of other properties that needs to be set as shown in this example.

```
#KODIPROP:inputstream=inputstream.ffmpegdirect
#KODIPROP:mimetype=application/x-mpegURL
#KODIPROP:inputstream.ffmpegdirect.is_realtime_stream=true
#KODIPROP:inputstream.ffmpegdirect.stream_mode=catchup
#KODIPROP:inputstream.ffmpegdirect.open_mode=ffmpeg
#KODIPROP:inputstream.ffmpegdirect.manifest_type=hls
#KODIPROP:inputstream.ffmpegdirect.default_url=http://mysite.com/streamX
#KODIPROP:inputstream.ffmpegdirect.playback_as_live=true
#KODIPROP:inputstream.ffmpegdirect.programme_start_time=1111111
#KODIPROP:inputstream.ffmpegdirect.programme_end_time=2111111
#KODIPROP:inputstream.ffmpegdirect.catchup_url_format_string=http://mysite.com/streamX?cutv={Y}-{m}-{d}T{H}:{M}:{S}
#KODIPROP:inputstream.ffmpegdirect.catchup_buffer_start_time=1111111
#KODIPROP:inputstream.ffmpegdirect.catchup_buffer_end_time=1111111
#KODIPROP:inputstream.ffmpegdirect.catchup_buffer_offset=1111111
#KODIPROP:inputstream.ffmpegdirect.timezone_shift=0
#KODIPROP:inputstream.ffmpegdirect.default_programme_duration=3600
#EXTINF:-1,MyChannel
http://127.0.0.1:3002/mystream.m3u8
```

- `stream_mode`: If the value `timeshift` is supplied the live stream will have a local timeshift buffer. If `catchup` is supplied the inputstream will start in catchup mode. Any other value or if omitted will open as a regular stream.
- `open_mode`: If the value `ffmpeg` is supplied the inputstream will be opened with AVFormat. If the value `curl` is supplied the inputstream will be opened with cURL. If neither value is supplied the default is to open HLS, Dash and Smooth Streaming with AVFormat and anything else as a kodi file. Note that a `mimetype` or `manifest_type` property is required to be able to tell if a stream is HLS or Dash. If using Smooth streaming only a `manifest_type` property will work as Smooth Streaming does not have a mimetype.
- `manifest_type`: Allowed values are `hls` for HLS, `mpd` for Dash and `ism` for Smooth Streaming.
- `default_url`: The URL to use if a catchup URL cannot be generated for any reason.
- `playback_as_live`: Should the playback be considerd as live tv, allowing skipping from one programme to the next over the entire catchup window, if so set to `true`. Otherwise set to `false` to treat all programmes as videos.
- `programme_start_time`: The unix time in seconds of the start of the programme being streamed - optional.
- `programme_end_time`: The unix time in seconds of the end of the programme being streamed - optional.
- `catchup_url_format_string`: The URL including format specifiers to use to generate catchup URLs when seeking the stream.
- `catchup_url_near_live_format_string`: Some providers require a different catchup URL when close to live playback so the stream does not stop.
- `catchup_buffer_start_time`: The unix time in seconds of the start of catchup window.
- `catchup_buffer_end_time`: The unix time in seconds of the end of catchup window.
- `catchup_buffer_offset`: The offset from the catchup buffer start time where playback should begin.
- `catchup_terminates`: Indicates with a value of `true` or `false` if a catchup stream will specify an end time and will stop eventually. Essentially means that the provider does not support delayed live streams. Value used by the addon to try and restart with a new end time near the end of the stream.
- `catchup_granularity`: The granularity the catchup source can seek to in seconds. Generally a value of 1 or 60 is used.
- `timezone_shift`: The value in seconds to shift the catchup times by for your timezone. Valid values range from -43200 to 50400 (from -12 hours to +14 hours).
- `default_programme_duration`: If the programme duration is unknown use this default value in seconds instead. If this value is not provided 4 hours (14,400 secs) will be used  will be used.
- `programme_catchup_id`: For providers that require a programme specifc id the following value can be used in the url format string.

**Notes:**
- Setting `playback_as_live` to `true` only makes sense when the catchup start and end times are set to the size of the catchup windows (e.g. 3 days). If the catchup start and end times are set to the programme times then `playback_as_live` will have little effect.
- For catchup streams that terminate there is a minimum distance from live a stream can seek to/from. For streams with a 1 second granularity it's 1 minute, and for stream with a 60 second granularity it's 2 minutes. The reason for this window on terminating streams is to allow the restart functionality to work.
- For all catchup streams any seek to within 10 seconds of live would be considered live and will switch to the default URL. For this reason seeking from live to less than 10 seconds has no effect.

## Appendix

### Manual Steps to rebuild the addon on MacOSX

The following steps can be followed manually instead of using the `build-install-mac.sh` in the root of the addon repo after the [initial addon build](#build-tools-and-initial-addon-build) has been completed.

**To rebuild the addon after changes**

1. `rm tools/depends/target/binary-addons/.installed-macosx*`
2. `make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/binary-addons ADDONS="inputstream.ffmpegdirect" ADDON_SRC_PREFIX=$HOME`

or

1. `cd tools/depends/target/binary-addons/macosx*`
2. `make`

**Copy the addon to the Kodi addon directory on Mac**

1. `rm -rf "$HOME/Library/Application Support/Kodi/addons/inputstream.ffmpegdirect"`
2. `cp -rf $HOME/xbmc-addon/addons/inputstream.ffmpegdirect "$HOME/Library/Application Support/Kodi/addons"`