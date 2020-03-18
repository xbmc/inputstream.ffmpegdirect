project(nettle)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
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
                      --libdir=${CMAKE_INSTALL_PREFIX}/lib
                      --disable-shared
                      --disable-openssl
                      --disable-documentation
                      --enable-pic
                      CCPIC=
                      ${nettle_conf}
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)

install(CODE "execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
