cmake_minimum_required(VERSION 3.5)
project(nettle)

# Preserve the target architecture, SDK and deployment flags supplied by Kodi.
list(APPEND nettle_conf "CC=${CMAKE_C_COMPILER}"
                       "CXX=${CMAKE_CXX_COMPILER}"
                       "CFLAGS=${CMAKE_C_FLAGS}"
                       "CXXFLAGS=${CMAKE_CXX_FLAGS}"
                       "CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include $ENV{CPPFLAGS}"
                       "LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_EXE_LINKER_FLAGS} $ENV{LDFLAGS}"
                       "PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
# Nettle invokes build-time table generators without separate host CFLAGS.
if(CC_FOR_BUILD)
  list(APPEND nettle_conf "CC_FOR_BUILD=${CC_FOR_BUILD} ${CFLAGS_FOR_BUILD} ${LDFLAGS_FOR_BUILD}")
endif()
if(HOST)
  list(APPEND nettle_conf --host=${HOST})
endif()

include(ExternalProject)
externalproject_add(nettle
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --libdir=${CMAKE_INSTALL_PREFIX}/lib
                      --enable-static
                      --disable-shared
                      --disable-openssl
                      --disable-documentation
                      --enable-pic
                      ${nettle_conf}
                    BUILD_COMMAND make all-here
                    INSTALL_COMMAND make install-here
                    BUILD_IN_SOURCE 1)

install(CODE "Message(Done)")
