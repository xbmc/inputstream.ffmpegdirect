project(libiconv)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(ExternalProject)
externalproject_add(libiconv
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      --enable-extra-encodings
                      ${libiconv_conf})

install(CODE "Message(Done)")

