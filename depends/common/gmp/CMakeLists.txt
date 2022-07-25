cmake_minimum_required(VERSION 3.5)
project(gmp)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(ExternalProject)

if(CORE_SYSTEM_NAME STREQUAL osx OR
   CORE_SYSTEM_NAME STREQUAL darwin_embedded)

  if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    set(EXTRA_ARGS "--disable-assembly")
    set(EXTRA_ARGS "${EXTRA_ARGS} --host=aarch64-apple-darwin")
  else()
    set(EXTRA_ARGS "--with-pic")
    if(CPU STREQUAL "arm64")
      set(EXTRA_ARGS "${EXTRA_ARGS} --host=aarch64-apple-darwin")
    else()
      set(EXTRA_ARGS "${EXTRA_ARGS} --host=x86_64-apple-darwin")
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(EXTRA_ARGS "${EXTRA_ARGS} ABI=64")
    else()
      set(EXTRA_ARGS "${EXTRA_ARGS} ABI=32")
    endif()
  endif()

  externalproject_add(gmp
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}
                      CONFIGURE_COMMAND
                          bash --login -c
                          "./configure \
                          --prefix=${CMAKE_BINARY_DIR}/gmpbuild \
                          CC='${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS}' \
                          LDFLAGS='${CMAKE_EXE_LINKER_FLAGS}' \
                          CPP='${CMAKE_C_COMPILER} -E' \
                          CPPFLAGS='${CMAKE_C_FLAGS}' \
                          ${EXTRA_ARGS} \
                          --enable-static \
                          --disable-shared"
                      BUILD_COMMAND  make
                      INSTALL_COMMAND
                        make install &&
                        # Is installed to different place to become no conflict during configure with
                        # "./share/config.site" on addon depends binary folder
                        cmake -E copy_directory ${CMAKE_BINARY_DIR}/gmpbuild ${CMAKE_INSTALL_PREFIX}
                      BUILD_IN_SOURCE TRUE)
else()
  if(CORE_SYSTEM_NAME MATCHES "linux" AND CPU MATCHES "i.86")
      list(APPEND gmp_conf ABI=32)
  endif()

  externalproject_add(gmp
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}
                      CONFIGURE_COMMAND <SOURCE_DIR>/configure
                        --prefix=${CMAKE_INSTALL_PREFIX}
                        --disable-assembly
                        --enable-static
                        --disable-shared
                        --with-pic
                        ${gmp_conf})
endif()

install(CODE "Message(Done)")
