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

# pkgconf is absent from Kodi's mirror. Download the exact package and signature
# from the snapshot's bundled msys.db for direct installation without a refresh.
file(DOWNLOAD https://repo.msys2.org/msys/x86_64/pkgconf-2.5.0-2-x86_64.pkg.tar.zst
     ${CMAKE_CURRENT_SOURCE_DIR}/var/cache/pacman/pkg/pkgconf-2.5.0-2-x86_64.pkg.tar.zst
     EXPECTED_HASH SHA256=4cc1a938d316800a7361be533497543fd173b4d68b3ddb6df9212ef4bf1ea9b2)
file(DOWNLOAD https://repo.msys2.org/msys/x86_64/pkgconf-2.5.0-2-x86_64.pkg.tar.zst.sig
     ${CMAKE_CURRENT_SOURCE_DIR}/var/cache/pacman/pkg/pkgconf-2.5.0-2-x86_64.pkg.tar.zst.sig
     EXPECTED_HASH SHA256=cc12ef0a443d02d35a3d86c8d53729ff2d080d65794e0a24a591227b8242167a)

# This recipe downloads the x86_64 MSYS2 host environment.
set(MINGW_PATH "mingw64")

# Use the Windows shell to keep GPG helpers from inheriting MSBuild's pipes.
function(msys2_setup name command)
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/tmp/${name}.log" "")
  file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/tmp/${name}.bat"
       "@echo off\r\n"
       "\"${CMAKE_CURRENT_SOURCE_DIR}/usr/bin/bash.exe\" --login -c \"${command}\""
       " > \"${CMAKE_CURRENT_SOURCE_DIR}/tmp/${name}.log\" 2>&1\r\n")
  # Wait for the setup process, not its daemon descendants.
  execute_process(COMMAND powershell -NoProfile -NonInteractive -Command
      "try {
         $p = Start-Process -FilePath './tmp/${name}.bat' -WindowStyle Hidden -PassThru -ErrorAction Stop
         # Retain the process handle so ExitCode remains available after waiting.
         $null = $p.Handle
         $p.WaitForExit()
         exit $p.ExitCode
       } catch {
         Write-Error \"MSYS2 ${name} launcher failed: $_\"
         exit 1
       }"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      RESULT_VARIABLE result)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/tmp/${name}.log" output)
  message("${output}")
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "MSYS2 ${name} failed: ${result}")
  endif()
endfunction()

msys2_setup(pacman-key "pacman-key --init")
msys2_setup(pacman-install "pacman --noconfirm --needed -S make nasm perl")
msys2_setup(pkgconf-install "pacman --noconfirm --needed -U /var/cache/pacman/pkg/pkgconf-2.5.0-2-x86_64.pkg.tar.zst")

file(GLOB_RECURSE shared_libs ${CMAKE_CURRENT_SOURCE_DIR}/${MINGW_PATH}/*.dll.a)
if(shared_libs)
  file(REMOVE ${shared_libs})
endif()

configure_file(MinGWConfig.cmake.in MinGWConfig.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MinGWConfig.cmake DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/caller.sh DESTINATION ${CMAKE_INSTALL_PREFIX})
