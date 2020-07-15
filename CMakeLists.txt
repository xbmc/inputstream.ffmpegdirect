cmake_minimum_required(VERSION 3.5)
project(inputstream.ffmpegdirect)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

find_package(PkgConfig)
if(WIN32)
  find_package(LibXml2 REQUIRED)
  find_package(Iconv REQUIRED)
  find_package(OpenSSL REQUIRED)
endif()
find_package(p8-platform REQUIRED)
find_package(Kodi REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)

set(CATCHUP_SOURCES src/StreamManager.cpp
                    src/stream/DemuxStream.cpp
                    src/stream/FFmpegCatchupStream.cpp
                    src/stream/FFmpegLog.cpp
                    src/stream/FFmpegStream.cpp
                    src/stream/CurlCatchupInput.cpp
                    src/stream/CurlInput.cpp
                    src/stream/commons/Exception.cpp
                    src/stream/threads/Atomics.cpp
                    src/stream/threads/Event.cpp
                    src/stream/threads/Thread.cpp
                    src/stream/threads/Timer.cpp
                    src/stream/threads/TimeUtils.cpp
                    src/stream/threads/SystemClock.cpp
                    src/stream/TimeshiftBuffer.cpp
                    src/stream/TimeshiftSegment.cpp
                    src/stream/TimeshiftStream.cpp
                    src/stream/url/URL.cpp
                    src/stream/url/UrlOptions.cpp
                    src/stream/url/Variant.cpp
                    src/utils/DiskUtils.cpp
                    src/utils/FilenameUtils.cpp)

set(CATCHUP_HEADERS src/StreamManager.h
                    src/stream/BaseStream.h
                    src/stream/DemuxStream.h
                    src/stream/FFmpegCatchupStream.h
                    src/stream/FFmpegLog.h
                    src/stream/FFmpegStream.h
                    src/stream/CurlCatchupInput.h
                    src/stream/CurlInput.h
                    src/stream/IManageDemuxPacket.h
                    src/stream/commons/Exception.h
                    src/stream/threads/Atomics.h
                    src/stream/threads/Condition.h
                    src/stream/threads/CriticalSection.h
                    src/stream/threads/Event.h
                    src/stream/threads/Helpers.h
                    src/stream/threads/Lockables.h
                    src/stream/threads/SharedSection.h
                    src/stream/threads/SingleLock.h
                    src/stream/threads/SystemClock.h
                    src/stream/threads/Thread.h
                    src/stream/threads/Timer.h
                    src/stream/threads/TimeUtils.h
                    src/stream/threads/platform/ThreadImpl.h
                    src/stream/TimeshiftBuffer.h
                    src/stream/TimeshiftSegment.h
                    src/stream/TimeshiftStream.h
                    src/utils/HttpProxy.h
                    src/utils/DiskUtils.h
                    src/utils/FilenameUtils.h
                    src/utils/Log.h
                    src/utils/Properties.h
                    src/stream/url/URL.h
                    src/stream/url/UrlOptions.h
                    src/stream/url/Variant.h)

if(NOT WIN32)
  list(APPEND CATCHUP_SOURCES src/stream/platform/posix/XTimeUtils.cpp)

  list(APPEND CATCHUP_HEADERS src/stream/platform/posix/PlatformDefs.h
                              src/stream/platform/posix/XTimeUtils.h)
endif()

include_directories(${p8-platform_INCLUDE_DIRS}
                    ${INCLUDES}
                    ${FFMPEG_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                    ${BZIP2_INCLUDE_DIRS}
                    ${OPENSSL_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIRS}
                    ${KODI_INCLUDE_DIR}/..) # Hack way with "/..", need bigger Kodi cmake rework to match right include ways (becomes done in future)

# to see linker output
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")

# We don't add FFMPEG_LIBRARIES here as we handle them at the end
list(APPEND DEPLIBS ${p8-platform_LIBRARIES}
                    ${ZLIB_LIBRARIES}
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
  find_library(VIDEOTOOLBOX VideoToolbox)
  find_library(SECURITY Security)

  include_directories(${AUDIOTOOLBOX}
                      ${COREFOUNDATION}
                      ${COREMEDIA}
                      ${VIDEOTOOLBOX}
                      ${SECURITY})

  list(APPEND DEPLIBS ${AUDIOTOOLBOX} ${COREFOUNDATION} ${COREMEDIA} ${VIDEOTOOLBOX} ${SECURITY})
else()
  list(APPEND DEPLIBS ${FFMPEG_LIBRARIES})
endif()

addon_version(inputstream.ffmpegdirect CATCHUP)
add_definitions(-DCATCHUP_VERSION=${CATCHUP_VERSION})

build_addon(inputstream.ffmpegdirect CATCHUP DEPLIBS)

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
