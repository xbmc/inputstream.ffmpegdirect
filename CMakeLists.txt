cmake_minimum_required(VERSION 3.5)
project(inputstream.ffmpegdirect)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(PkgConfig)
if(WIN32)
  find_package(LibXml2 REQUIRED)
  find_package(Iconv REQUIRED)
  find_package(OpenSSL REQUIRED)
endif()
find_package(Kodi REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

set(FFMPEGDIRECT_SOURCES src/StreamManager.cpp
                         src/stream/DemuxStream.cpp
                         src/stream/FFmpegCatchupStream.cpp
                         src/stream/FFmpegLog.cpp
                         src/stream/FFmpegStream.cpp
                         src/stream/CurlCatchupInput.cpp
                         src/stream/CurlInput.cpp
                         src/stream/TimeshiftBuffer.cpp
                         src/stream/TimeshiftSegment.cpp
                         src/stream/TimeshiftStream.cpp
                         src/stream/url/URL.cpp
                         src/stream/url/UrlOptions.cpp
                         src/stream/url/Variant.cpp
                         src/utils/DiskUtils.cpp
                         src/utils/FilenameUtils.cpp)

set(FFMPEGDIRECT_HEADERS src/StreamManager.h
                         src/stream/BaseStream.h
                         src/stream/DemuxStream.h
                         src/stream/FFmpegCatchupStream.h
                         src/stream/FFmpegLog.h
                         src/stream/FFmpegStream.h
                         src/stream/CurlCatchupInput.h
                         src/stream/CurlInput.h
                         src/stream/IManageDemuxPacket.h
                         src/stream/TimeshiftBuffer.h
                         src/stream/TimeshiftSegment.h
                         src/stream/TimeshiftStream.h
                         src/utils/HttpProxy.h
                         src/utils/DiskUtils.h
                         src/utils/FilenameUtils.h
                         src/utils/Log.h
                         src/utils/Properties.h
                         src/utils/TimeUtils.h
                         src/stream/url/URL.h
                         src/stream/url/UrlOptions.h
                         src/stream/url/Variant.h)

include_directories(${INCLUDES}
                    ${FFMPEG_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                    ${BZIP2_INCLUDE_DIRS}
                    ${OPENSSL_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}/..) # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)

# to see linker output
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")

# We don't add FFMPEG_LIBRARIES here as we handle them at the end
list(APPEND DEPLIBS ${ZLIB_LIBRARIES}
                    ${BZIP2_LIBRARIES})

if(WIN32)
  # Windows does need to know about FFMPEG_LIBRARIES here
  list(APPEND DEPLIBS ${ICONV_LIBRARIES}
                      ${OPENSSL_LIBRARIES}
                      ${LIBXML2_LIBRARIES}
                      ${FFMPEG_LIBRARIES}
                      Winmm.lib
                      WS2_32.lib
                      Bcrypt.lib
                      Secur32.lib
                      Mfplat.lib
                      Mfuuid.lib
                      Strmiids.lib)
elseif(CORE_SYSTEM_NAME STREQUAL osx OR
       CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  find_library(AUDIOTOOLBOX AudioToolbox)
  find_library(COREFOUNDATION CoreFoundation)
  find_library(COREMEDIA CoreMedia)
  find_library(COREVIDEO CoreVideo)
  find_library(VIDEOTOOLBOX VideoToolbox)
  find_library(SECURITY Security)

  include_directories(${AUDIOTOOLBOX}
                      ${COREFOUNDATION}
                      ${COREMEDIA}
                      ${COREVIDEO}
                      ${VIDEOTOOLBOX}
                      ${SECURITY})

  list(APPEND DEPLIBS ${AUDIOTOOLBOX} ${COREFOUNDATION} ${COREMEDIA} ${COREVIDEO} ${VIDEOTOOLBOX} ${SECURITY})
else()
  list(APPEND DEPLIBS ${FFMPEG_LIBRARIES})
endif()

addon_version(inputstream.ffmpegdirect FFMPEGDIRECT)
add_definitions(-DFFMPEGDIRECT_VERSION=${FFMPEGDIRECT_VERSION})

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  # TODO: This is required so FFmpeg's av_init_packet() deprecation does not break windows store builds
  #       Once this function is no longer used this can and should be removed.
  add_compile_options(/wd4996)
endif()

build_addon(inputstream.ffmpegdirect FFMPEGDIRECT DEPLIBS)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Due to a bug in CMake and frameworks on OSX we strip them from FFMPEG_LDFLAGS
  string(REGEX REPLACE "-framework;([A-Za-z0-9_]+);?" "" FFMPEG_LDFLAGS "${FFMPEG_LDFLAGS}")
endif()
# Now remove ';''s for all platforms and allow CMake to add them back
string(REGEX REPLACE ";" " " FFMPEG_LDFLAGS "${FFMPEG_LDFLAGS}")
set_target_properties(inputstream.ffmpegdirect PROPERTIES LINK_FLAGS "${FFMPEG_LDFLAGS}")

if(CORE_SYSTEM_NAME STREQUAL windowsstore)
  # fix linking
  set_target_properties(inputstream.ffmpegdirect PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/LTCG /defaultlib:vccorlib.lib /defaultlib:msvcrt.lib")
  set_target_properties(inputstream.ffmpegdirect PROPERTIES LINK_FLAGS_RELEASE "/LTCG /defaultlib:vccorlib.lib /defaultlib:msvcrt.lib")
  set_target_properties(inputstream.ffmpegdirect PROPERTIES LINK_FLAGS_DEBUG "/LTCG /defaultlib:vccorlibd.lib /defaultlib:msvcrtd.lib")
endif()

include(CPack)
