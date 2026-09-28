cmake_minimum_required(VERSION 3.5)
project(mingw)

# Currently unused (our mirrors not have newest)
if(0)
  foreach(repo msys mingw32 mingw64)
    if(${repo} STREQUAL msys)
      file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.d/mirrorlist.${repo} "Server = http://mirrors.kodi.tv/build-deps/win32/msys2/repos/${repo}2/$arch\n")
    else()
      file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.d/mirrorlist.${repo} "Server = http://mirrors.kodi.tv/build-deps/win32/msys2/repos/${repo}\n")
    endif()
  endforeach()
endif()

# This recipe downloads the x86_64 MSYS2 host environment.
set(MINGW_PATH "mingw64")

execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe -lc
    " " ERROR_QUIET)
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe -lc
    "pacman --noconfirm -Syuu")
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe -lc
    "pacman --noconfirm -Syuu \
                           gcc \
                           make \
                           pkg-config \
                           nasm \
                           perl \
                           patch")

file(GLOB_RECURSE shared_libs ${CMAKE_CURRENT_SOURCE_DIR}/${MINGW_PATH}/*.dll.a)
if(shared_libs)
  file(REMOVE ${shared_libs})
endif()

configure_file(MinGWConfig.cmake.in MinGWConfig.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MinGWConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/caller.sh DESTINATION ${CMAKE_INSTALL_PREFIX})
