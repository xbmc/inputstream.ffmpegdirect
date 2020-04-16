[![License: GPL-2.0-or-later](https://img.shields.io/badge/License-GPL%20v2+-blue.svg)](LICENSE.md)
[![Build Status](https://travis-ci.org/xbmc/inputstream.ffmpegdirect.svg?branch=Matrix)](https://travis-ci.org/xbmc/inputstream.ffmpegdirect/branches)
[![Build Status](https://dev.azure.com/teamkodi/binary-addons/_apis/build/status/xbmc.inputstream.ffmpegdirect?branchName=Matrix)](https://dev.azure.com/teamkodi/binary-addons/_build/latest?definitionId=30&branchName=Matrix)
[![Build Status](https://jenkins.kodi.tv/view/Addons/job/xbmc/job/inputstream.ffmpegdirect/job/Matrix/badge/icon)](https://jenkins.kodi.tv/blue/organizations/jenkins/xbmc%2Finputstream.ffmpegdirect/branches/)

# inputstream.ffmpegdirect addon for Kodi

This is a [Kodi](http://kodi.tv) input stream addon for streams that can be opened by FFmpeg's libavformat, such as plain TS, HLS and DASH streams. Note that the only DASH streams supported are those without DRM.

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

### HTTP Proxy
Settings for configuring the HTTP Proxy

* **Use HTTP proxy**: Whether or not a proxy should be used.
* **Server**: Configure the proxy server address.
* **Port**: Configure the proxy server port.
* **Username**: Configure the proxy server username.
* **Password**: Configure the proxy server password.

## Using the addon

The addon can be accessed like any other inputstream in Kodi. The following example will show how to manually choose this addon for playback when using IPTV Simple Client with the following entry in the M3U file (Note that the IPTV Simple Client will in fact automatcially detect a catchup stream and use the addon based on configured settings).

```
#KODIPROP:inputstreamclass=inputstream.ffmpegdirect
#KODIPROP:inputstream.ffmpegdirect.mime_type=video/mp2t
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

If enabling archive/catchup support there are a number of other properties that needs to be set as shown in this example.

```
#KODIPROP:inputstreamclass=inputstream.ffmpegdirect
#KODIPROP:inputstream.ffmpegdirect.mime_type=application/x-mpegURL
#KODIPROP:inputstream.ffmpegdirect.is_realtime_stream=true
#KODIPROP:inputstream.ffmpegdirect.is_catchup_stream=catchup
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

- `default_url`: The URL to use if a catchup URL cannot be generated for any reason.
- `playback_as_live`: Should the playback be considerd as live tv, allowing skipping from one programme to the next over the entire catchup window, if so set to `true`. Otherwise set to `false` to treat all programmes as videos.
- `programme_start_time`: The unix time in seconds of the start of the programme being streamed - optional.
- `programme_end_time`: The unix time in seconds of the end of the programme being streamed - optional.
- `catchup_url_format_string`: The URL including format specifiers to use to generate catchup URLs when seeking the stream.
- `catchup_url_near_live_format_string`: Some providers require a different catchup URL when close to live playback so the stream does not stop.
- `catchup_buffer_start_time`: The unix time in seconds of the start of catchup window.
- `catchup_buffer_end_time`: The unix time in seconds of the end of catchup window.
- `catchup_buffer_offset`: The offset from the catchup buffer start time where playback should begin.
- `timezone_shift`: The value in seconds to shift the catchup times by for your timezone. Valid values range from -43200 to 50400 (from -12 hours to +14 hours).
- `default_programme_duration`: If the programme duration is unknown use this default value in seconds instead. If this value is not provided 4 hours (14,400 secs) will be used  will be used.
- `programme_catchup_id`: For providers that require a programme specifc id the following value can be used in the url format string.

Note: setting `playback_as_live` to `true` only makes sense when the catchup start and end times are set to the size of the catchup windows (e.g. 3 days). If the catchup start and end times are set to the programme times then `playback_as_live` will have little effect.

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