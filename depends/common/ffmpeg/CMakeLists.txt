cmake_minimum_required(VERSION 3.5)
project(ffmpeg)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

# Init base values given on build to ffmpeg
set(STANDARD_CONF "")
set(EXTRA_CONF "")
set(EXTRA_CFLAGS "")
set(EXTRA_LDFLAGS "")

# Read version for ffmpeg to define on extra version
file(STRINGS "${CMAKE_SOURCE_DIR}/RELEASE" FFMPEG_VER)

# Set the basic ffmpeg configure options (same on all OS)
list(APPEND STANDARD_CONF --extra-version=kodi-${FFMPEG_VER}
                          --pkg-config-flags=--static
                          --disable-devices
                          --disable-doc
                          --disable-ffplay
                          --disable-ffmpeg
                          --disable-ffprobe
                          --enable-gpl
                          --enable-runtime-cpudetect
                          --enable-postproc
                          --enable-muxer=spdif
                          --enable-muxer=adts
                          --enable-muxer=asf
                          --enable-muxer=ipod
                          --enable-encoder=ac3
                          --enable-encoder=aac
                          --enable-encoder=wmav2
                          --enable-protocol=http
                          --enable-encoder=png
                          --enable-encoder=mjpeg
                          --enable-demuxer=dash
                          --enable-libxml2
                          --enable-zlib
                          --enable-static)

# Determinate debug or release and set ffmpeg for
if(CMAKE_BUILD_TYPE STREQUAL Release)
  list(APPEND EXTRA_CONF --disable-debug)
else()
  list(APPEND EXTRA_CONF --enable-debug)
endif()

# Run the builds
include(ExternalProject)
if(NOT WIN32)
  if(ENABLE_CCACHE AND CCACHE_PROGRAM)
    list(APPEND EXTRA_CONF "--cc=${CCACHE_PROGRAM} ${CMAKE_C_COMPILER}" "--cxx=${CCACHE_PROGRAM} ${CMAKE_CXX_COMPILER}")
  else()
    list(APPEND EXTRA_CONF --cc=${CMAKE_C_COMPILER} --cxx=${CMAKE_CXX_COMPILER})
  endif()

  if(CMAKE_CROSSCOMPILING)
    list(APPEND EXTRA_CONF --enable-cross-compile --arch=${CPU})
    list(APPEND EXTRA_CONF --ar=${CMAKE_AR} --strip=${CMAKE_STRIP})
    message(STATUS "CROSS: ${EXTRA_CONF}")
  endif()

  if(CMAKE_C_FLAGS)
    list(APPEND EXTRA_CONF --extra-cflags=${CMAKE_C_FLAGS})
  endif()

  if(CMAKE_CXX_FLAGS)
    list(APPEND EXTRA_CONF --extra-cxxflags=${CMAKE_CXX_FLAGS})
  endif()

  if(CMAKE_EXE_LINKER_FLAGS)
    if(CORE_SYSTEM_NAME STREQUAL android)
      string(APPEND CMAKE_EXE_LINKER_FLAGS " -L${CMAKE_INSTALL_PREFIX}/lib")
    endif()
    list(APPEND EXTRA_CONF --extra-ldflags=${CMAKE_EXE_LINKER_FLAGS})
  endif()

  if(ENABLE_NEON)
    list(APPEND EXTRA_CONF --enable-neon)
  endif()

  if(CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd)
    if(CORE_PLATFORM_NAME STREQUAL rbpi)
      list(APPEND EXTRA_CONF --cpu=${CPU} --disable-vaapi --disable-vdpau)
    else()
      list(APPEND EXTRA_CONF --enable-pic)
      if(ENABLE_VAAPI)
        list(APPEND EXTRA_CONF --enable-vaapi)
      else()
        list(APPEND EXTRA_CONF --disable-vaapi)
      endif()
      if(ENABLE_VDPAU)
        list(APPEND EXTRA_CONF --enable-vdpau)
      else()
        list(APPEND EXTRA_CONF --disable-vdpau)
      endif()
    endif()
  elseif(CORE_SYSTEM_NAME STREQUAL android)
    if(CPU MATCHES arm64)
      list(APPEND EXTRA_CONF --cpu=cortex-a53 --arch=aarch64)
    elseif(CPU MATCHES arm)
      list(APPEND EXTRA_CONF --cpu=cortex-a9)
    else()
      list(APPEND EXTRA_CONF --cpu=i686 --disable-mmx)
    endif()
    list(APPEND EXTRA_CONF --target-os=android --extra-libs=-liconv --disable-linux-perf)
  elseif(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    if(NOT CPU MATCHES arm64)
      list(APPEND EXTRA_CONF --cpu=cortex-a8)
    else()
      list(APPEND EXTRA_CONF "--as=${CMAKE_INSTALL_PREFIX}/bin/gas-preprocessor.pl -arch aarch64 -- ${CMAKE_C_COMPILER}")
      list(APPEND EXTRA_CONF --arch=aarch64)
      list(APPEND EXTRA_CONF --disable-filter=yadif_videotoolbox)
    endif()
    list(APPEND EXTRA_CONF --disable-decoder=mpeg_xvmc --disable-crystalhd --enable-videotoolbox
                           --target-os=darwin)
  elseif(CORE_SYSTEM_NAME STREQUAL osx)
    list(APPEND EXTRA_CONF --disable-decoder=mpeg_xvmc --disable-crystalhd --enable-videotoolbox
                           --target-os=darwin
                           --disable-securetransport)
  endif()

  if(CPU MATCHES arm OR CORE_PLATFORM_NAME STREQUAL rbpi)
    list(APPEND EXTRA_CONF --enable-pic --disable-armv5te --disable-armv6t2)
  elseif(CPU MATCHES mips)
    list(APPEND EXTRA_CONF --disable-mips32r2 --disable-mipsdsp --disable-mipsdspr2)
  endif()

  message(STATUS "FFMPEG_CONF (Standard): ${STANDARD_CONF}")
  message(STATUS "FFMPEG_CONF (Extra, OS dependend): ${EXTRA_CONF}")

  externalproject_add(ffmpeg
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}
                      CONFIGURE_COMMAND PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig ${CMAKE_SOURCE_DIR}/configure
                        --prefix=${CMAKE_INSTALL_PREFIX}
                        --enable-pthreads
                        --enable-gnutls
                        --enable-libzvbi
                        ${STANDARD_CONF}
                        ${EXTRA_CONF})

  if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    externalproject_add_step(ffmpeg
                            RunSedCommand
                            COMMAND echo "Setting HAVE_CLOCK_GETTIME to 0 for darwin_embedded using sed"
                            COMMAND sed -i -- "s/HAVE_CLOCK_GETTIME 1/HAVE_CLOCK_GETTIME 0/g" ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-prefix/src/ffmpeg-build/config.h
                            DEPENDEES configure)
  endif()
else()
  find_package(MinGW REQUIRED)

  if(CMAKE_C_FLAGS)
    list(APPEND EXTRA_CFLAGS ${CMAKE_C_FLAGS})
  endif()

  if(CMAKE_CXX_FLAGS)
    list(APPEND EXTRA_CXXFLAGS ${CMAKE_CXX_FLAGS})
  endif()

  if(CMAKE_EXE_LINKER_FLAGS)
    list(APPEND EXTRA_LDFLAGS ${CMAKE_EXE_LINKER_FLAGS})
  endif()

  # Replace slash with minus, mingw does interpret a slash wrong!
  MinGW_ChangeSlashToMinus(${EXTRA_CFLAGS})
  set(EXTRA_CFLAGS ${FLAGS})
  MinGW_ChangeSlashToMinus(${EXTRA_CXXFLAGS})
  set(EXTRA_CXXFLAGS ${FLAGS})
  MinGW_ChangeSlashToMinus(${EXTRA_LDFLAGS})
  set(EXTRA_LDFLAGS ${FLAGS})

  list(APPEND EXTRA_CFLAGS -I${CMAKE_INSTALL_PREFIX}/include
                           -I${CMAKE_INSTALL_PREFIX}/include/libxml2
                           -FS)
  list(APPEND EXTRA_CXXFLAGS -I${CMAKE_INSTALL_PREFIX}/include
                             -I${CMAKE_INSTALL_PREFIX}/include/libxml2
                             -FS)
  list(APPEND EXTRA_LDFLAGS -LIBPATH:${CMAKE_INSTALL_PREFIX}/lib)
  if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    list(APPEND EXTRA_CONF --enable-cross-compile)
    list(APPEND EXTRA_CFLAGS -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WIN32_WINNT=0x0A00)
    list(APPEND EXTRA_LDFLAGS -APPCONTAINER WindowsApp.lib)
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL Release)
    list(APPEND EXTRA_CFLAGS -MD -wd4828)
    list(APPEND EXTRA_CXXFLAGS -MD -wd4828)
  else()
    list(APPEND EXTRA_CFLAGS -MDd)
    list(APPEND EXTRA_CXXFLAGS -MDd)
    list(APPEND EXTRA_LDFLAGS -NODEFAULTLIB:libcmt)
  endif()

  include(CheckSymbolExists)
  check_symbol_exists(_X86_ "Windows.h" _X86_)
  check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)
  check_symbol_exists(_ARM_ "Windows.h" _ARM_)
  check_symbol_exists(_ARM64_ "Windows.h" _ARM64_)

  if(_X86_)
    set(EXTRA_ARCH "x86")
    list(APPEND EXTRA_CONF --target-os=win32 --cpu=i686)
  elseif(_AMD64_)
    set(EXTRA_ARCH "x86_64")
    list(APPEND EXTRA_CONF --target-os=win64)
  elseif(_ARM_)
    set(EXTRA_ARCH "arm")
    list(APPEND EXTRA_CONF --target-os=win32 --cpu=armv7)
  elseif(_ARM64_)
    set(EXTRA_ARCH "aarch64")
    list(APPEND EXTRA_CONF --target-os=win32 --cpu=aarch64)
  else()
    message(FATAL_ERROR "Unsupported architecture")
  endif()

  externalproject_add(ffmpeg
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}
                      CONFIGURE_COMMAND ${MINGW_CALL}
                        ./configure
                          --prefix=[DQ]${CMAKE_INSTALL_PREFIX}[DQ]
                          --arch=${EXTRA_ARCH}
                          --toolchain=msvc
                          --enable-w32threads
                          --enable-openssl
                          --enable-nonfree
                          --disable-gnutls
                          --disable-libzvbi
                          ${STANDARD_CONF}
                          ${EXTRA_CONF}
                          --extra-cflags=[DQ]${EXTRA_CFLAGS}[DQ]
                          --extra-cxxflags=[DQ]${EXTRA_CXXFLAGS}[DQ]
                          --extra-ldflags=[DQ]${EXTRA_LDFLAGS}[DQ]
                      BUILD_COMMAND ${MINGW_CALL}
                        make -j$ENV{NUMBER_OF_PROCESSORS}
                      INSTALL_COMMAND ${MINGW_CALL}
                        make install)
endif()

install(CODE "Message(Done)")
