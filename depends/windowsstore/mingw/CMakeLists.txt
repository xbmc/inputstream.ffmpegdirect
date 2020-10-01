cmake_minimum_required(VERSION 3.5)
project(mingw)

# Currently unused (our mirrors bring some download problems for Msys2)
if(1)
  foreach(repo msys mingw32 mingw64)
    if(${repo} STREQUAL msys)
      file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.d/mirrorlist.${repo} "Server = http://mirrors.kodi.tv/build-deps/win32/msys2/repos/${repo}2/$arch\n")
    else()
      file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.d/mirrorlist.${repo} "Server = http://mirrors.kodi.tv/build-deps/win32/msys2/repos/${repo}\n")
    endif()
  endforeach()
endif()

if($ENV{PROCESSOR_ARCHITECTURE} MATCHES "AMD64" OR
   $ENV{PROCESSOR_ARCHITECTURE} MATCHES "IA64")
   set(HOST mingw-w64-x86_64)
   set(MINGW_PATH "mingw64")
else()
   set(HOST mingw-w64-i686)
   set(MINGW_PATH "mingw32")
endif()

execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe --login -c
    "pacman-key --init" ERROR_QUIET)
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe --login -c
    "pacman --noconfirm -S mingw-w64-cross-binutils \
                           ${HOST}-gcc ${HOST}-lua \
                           ${HOST}-headers \
                           make \
                           autoconf \
                           automake \
                           libtool \
                           pkg-config \
                           nasm \
                           yasm \
                           patch")

file(GLOB_RECURSE shared_libs ${CMAKE_CURRENT_SOURCE_DIR}/${MINGW_PATH}/*.dll.a)
if(shared_libs)
  file(REMOVE ${shared_libs})
endif()

configure_file(MinGWConfig.cmake.in MinGWConfig.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MinGWConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/caller.sh DESTINATION ${CMAKE_INSTALL_PREFIX})
