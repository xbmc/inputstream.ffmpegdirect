cmake_minimum_required(VERSION 3.5)
project(libzvbi)

# Keep the target architecture, SDK and deployment flags supplied by Kodi.
list(APPEND zvbi_conf "CC=${CMAKE_C_COMPILER}"
                      "CXX=${CMAKE_CXX_COMPILER}"
                      "CFLAGS=${CMAKE_C_FLAGS}"
                      "CXXFLAGS=${CMAKE_CXX_FLAGS}"
                      "CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include $ENV{CPPFLAGS}"
                      "LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_EXE_LINKER_FLAGS} $ENV{LDFLAGS}")
if(HOST)
  list(APPEND zvbi_conf --host=${HOST})
endif()
if(CORE_SYSTEM_NAME STREQUAL android OR CMAKE_CXX_COMPILER MATCHES arm-webos)
  list(APPEND zvbi_conf "LIBS=-liconv $ENV{LIBS}")
endif()

# The upstream archive uses README.md rather than Automake's GNU README name.
find_program(ZVBI_AUTOMAKE automake)
include(ExternalProject)
externalproject_add(libzvbi
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    PATCH_COMMAND ${CMAKE_COMMAND} -E env
                      "AUTOMAKE=${ZVBI_AUTOMAKE} --foreign" autoreconf -vif
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      --enable-static
                      --disable-v4l
                      --disable-dvb
                      --disable-bktr
                      --disable-nls
                      --disable-proxy
                      --disable-tests
                      --disable-examples
                      --without-doxygen
                      --with-pic
                      --without-x
                      ${zvbi_conf})

install(CODE "Message(Done)")
