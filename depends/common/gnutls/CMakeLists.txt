project(gmp)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

list(APPEND gnutls_conf PKG_CONFIG_PATH=${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
list(APPEND gnutls_conf CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include)
list(APPEND gnutls_conf LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib)
list(APPEND gnutls_conf LIBS=-lgmp)

include(ExternalProject)
externalproject_add(gnutls
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      --without-p11-kit
                      --disable-nls
                      --with-included-unistring
                      --with-included-libtasn1
                      --enable-local-libopts
                      --disable-doc
                      --disable-tests
                      --disable-guile
                      ${gnutls_conf})

install(CODE "Message(Done)")

