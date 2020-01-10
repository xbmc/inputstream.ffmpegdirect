project(bzip2)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

set(cflags ${CMAKE_C_FLAGS} -I${OUTPUT_DIR}/include)

include(ExternalProject)
externalproject_add(bzip2
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND ""
                    BUILD_COMMAND cd <SOURCE_DIR>
                                  && make
                                  PREFIX=${CMAKE_INSTALL_PREFIX}
                                  CFLAGS=${cflags} CC=${CMAKE_C_COMPILER} AR=${CMAKE_AR}
                    INSTALL_COMMAND "")

install(CODE "execute_process(COMMAND make install
                                      PREFIX=${CMAKE_INSTALL_PREFIX}
                                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})")

#install (SCRIPT "${CMAKE_SOURCE_DIR}/PostInstall.cmake")
