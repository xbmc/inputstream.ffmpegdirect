project(zvbi)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

list(APPEND zvbi_conf CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND zvbi_conf LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib)

include(ExternalProject)
externalproject_add(zvbi
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --enable-shared=no
                      --enable-static
                      --disable-dvb
                      --disable-bktr
                      --disable-nls
                      --disable-proxy
                      --without-doxygen
                      --without-x
                      ${zvbi_ldflags}
                      ${zvbi_conf})

install(CODE "Message(Done)")
