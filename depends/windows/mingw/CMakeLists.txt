cmake_minimum_required(VERSION 3.5)
project(mingw)

foreach(repo msys mingw32 mingw64)
  if(${repo} STREQUAL msys)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.d/mirrorlist.${repo} "Server = http://mirrors.kodi.tv/build-deps/win32/msys2/repos/${repo}2/$arch\n")
  else()
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.d/mirrorlist.${repo} "Server = http://mirrors.kodi.tv/build-deps/win32/msys2/repos/${repo}\n")
  endif()
endforeach()

# The snapshot uses a shared MinGW mirror list; select only Kodi's repositories.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.conf PACMAN_CONFIG)
foreach(repo mingw32 mingw64)
  string(REPLACE "[${repo}]\nInclude = /etc/pacman.d/mirrorlist.mingw\n"
                 "[${repo}]\nInclude = /etc/pacman.d/mirrorlist.${repo}\n"
                 PACMAN_CONFIG "${PACMAN_CONFIG}")
endforeach()
foreach(repo clangarm64 ucrt64 clang64)
  string(REPLACE "[${repo}]\nInclude = /etc/pacman.d/mirrorlist.mingw\n" ""
                 PACMAN_CONFIG "${PACMAN_CONFIG}")
endforeach()
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/etc/pacman.conf "${PACMAN_CONFIG}")

# These packages are absent from Kodi's mirror. Cache the exact versions and
# SHA-256 values from the snapshot's bundled msys.db without refreshing it.
file(DOWNLOAD https://repo.msys2.org/msys/x86_64/pkgconf-2.5.0-2-x86_64.pkg.tar.zst
     ${CMAKE_CURRENT_SOURCE_DIR}/var/cache/pacman/pkg/pkgconf-2.5.0-2-x86_64.pkg.tar.zst
     EXPECTED_HASH SHA256=4cc1a938d316800a7361be533497543fd173b4d68b3ddb6df9212ef4bf1ea9b2)
file(DOWNLOAD https://repo.msys2.org/msys/x86_64/patch-2.7.6-3-x86_64.pkg.tar.zst
     ${CMAKE_CURRENT_SOURCE_DIR}/var/cache/pacman/pkg/patch-2.7.6-3-x86_64.pkg.tar.zst
     EXPECTED_HASH SHA256=dd75ca0f715dd9c71a43af6a0ff3d068faeee1d768e02282d319671201cd5d45)

# This recipe downloads the x86_64 MSYS2 host environment.
set(MINGW_PATH "mingw64")

execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe --login -c
    "pacman-key --init")
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe --login -c
    "pacman --noconfirm --needed -S make pkgconf nasm perl patch"
    RESULT_VARIABLE PACMAN_RESULT)
if(NOT PACMAN_RESULT EQUAL 0)
  message(FATAL_ERROR "MSYS2 host-tool installation failed: ${PACMAN_RESULT}")
endif()

file(GLOB_RECURSE shared_libs ${CMAKE_CURRENT_SOURCE_DIR}/${MINGW_PATH}/*.dll.a)
if(shared_libs)
  file(REMOVE ${shared_libs})
endif()

configure_file(MinGWConfig.cmake.in MinGWConfig.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MinGWConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/caller.sh DESTINATION ${CMAKE_INSTALL_PREFIX})
