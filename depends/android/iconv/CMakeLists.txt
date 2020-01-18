cmake_minimum_required(VERSION 3.5)
project(iconv)

include(ExternalProject)
externalproject_add(iconv
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND ./configure --disable-shared --prefix=${OUTPUT_DIR} --enable-extra-encodings --with-pic
                    BUILD_COMMAND make
                    INSTALL_COMMAND ""
                    BUILD_IN_SOURCE 1)

install(CODE "execute_process(COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")
