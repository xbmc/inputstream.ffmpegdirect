project(gmp)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

list(APPEND gnutls_conf PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
list(APPEND gnutls_conf CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND gnutls_conf LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib)
list(APPEND gnutls_conf LIBS=-lgmp)

if(APPLE)
  list(APPEND gnutls_conf ac_cv_func_vfork_works=no)
  list(APPEND gnutls_conf ac_cv_func_fork=no)
endif()

include(ExternalProject)
externalproject_add(gnutls
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      --without-p11-kit
                      --without-idn
                      --disable-nls
                      --with-included-unistring
                      --with-included-libtasn1
                      --enable-local-libopts
                      --without-nettle-mini
                      --disable-doc
                      --disable-tests
                      --disable-guile
                      --with-pic
                      ${gnutls_conf})

install(CODE "Message(Done)")

