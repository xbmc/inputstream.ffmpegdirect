project(libzvbi)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

list(APPEND zvbi_conf CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND zvbi_conf CFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND zvbi_conf LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib)
if (CORE_SYSTEM_NAME STREQUAL android OR CORE_SYSTEM_NAME STREQUAL darwin_embedded OR CORE_SYSTEM_NAME STREQUAL osx)
  list(APPEND zvbi_conf LIBS=-liconv)
endif()

include(ExternalProject)
externalproject_add(libzvbi
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    PATCH_COMMAND <SOURCE_DIR>/autogen.sh
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --enable-shared=no
                      --enable-static
                      --disable-dvb
                      --disable-bktr
                      --disable-nls
                      --disable-proxy
                      --without-doxygen
                      --with-pic
                      --without-x
                      ${zvbi_conf})

install(CODE "Message(Done)")
