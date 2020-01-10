project(nettle)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(CORE_SYSTEM_NAME STREQUAL ios OR CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  list(APPEND nettle_conf -disable-assembler)
endif()

list(APPEND nettle_conf CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND nettle_conf LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib)
list(APPEND nettle_conf LIBS=-lgmp)

include(ExternalProject)
externalproject_add(nettle
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      --disable-openssl
                      CCPIC=
                      ${nettle_conf})

install(CODE "Message(Done)")
