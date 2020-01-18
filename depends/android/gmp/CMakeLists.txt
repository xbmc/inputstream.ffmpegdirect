project(gmp)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(ExternalProject)
externalproject_add(gmp
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      ${COMPILER_WITH_LIBTOOL_SYSROOT_APPLE}
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      --with-pic
                      ${gmp_conf})

install(CODE "Message(Done)")
