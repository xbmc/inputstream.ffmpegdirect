cmake_minimum_required(VERSION 3.5)
project(gnutls)

# Preserve the target architecture, SDK and deployment flags supplied by Kodi.
list(APPEND gnutls_conf "CC=${CMAKE_C_COMPILER}"
                       "CXX=${CMAKE_CXX_COMPILER}"
                       "CFLAGS=${CMAKE_C_FLAGS}"
                       "CXXFLAGS=${CMAKE_CXX_FLAGS}"
                       "CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include $ENV{CPPFLAGS}"
                       "LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_EXE_LINKER_FLAGS} $ENV{LDFLAGS}"
                       "PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
if(HOST)
  list(APPEND gnutls_conf --host=${HOST})
endif()

# Keep optional compression dependencies linked into the static add-on.
list(APPEND gnutls_conf gnutls_cv_dlopen_soname_works=no)
if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  list(APPEND gnutls_conf ac_cv_func_vfork_works=no ac_cv_func_fork=no)
endif()

include(ExternalProject)
externalproject_add(gnutls
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --enable-static
                      --disable-shared
                      --without-p11-kit
                      --without-idn
                      --disable-nls
                      --with-included-unistring
                      --with-included-libtasn1
                      --without-nettle-mini
                      --disable-doc
                      --disable-tests
                      --disable-tools
                      --disable-maintainer-mode
                      --with-pic
                      ${gnutls_conf})

install(CODE "Message(Done)")
