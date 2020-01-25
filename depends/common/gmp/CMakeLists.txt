cmake_minimum_required(VERSION 3.5)
project(gmp)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(ExternalProject)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(IS64BIT TRUE)
else()
  set(IS64BIT FALSE)
endif()

if(CORE_SYSTEM_NAME STREQUAL osx OR
   CORE_SYSTEM_NAME STREQUAL ios OR
   CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  execute_process(COMMAND xcodebuild -sdk ${CMAKE_OSX_SYSROOT} -version SDKVersion
      OUTPUT_VARIABLE SDK_VERSION
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(CORE_SYSTEM_NAME STREQUAL osx)
    set(SDK_NAME "macosx")
  else()
    if(CORE_PLATFORM_NAME STREQUAL ios)
      set(SDK_NAME "iphoneos")
    elseif(CORE_PLATFORM_NAME STREQUAL tvos)
      set(SDK_NAME "appletvos")
    else()
      message(SEND_ERROR "Unknown platform name: ${CORE_PLATFORM_NAME}")
    endif()
  endif()

  execute_process(COMMAND xcrun --sdk ${SDK_NAME} --show-sdk-platform-path
      OUTPUT_VARIABLE PLATFORM
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(CORE_SYSTEM_NAME STREQUAL ios OR CORE_SYSTEM_NAME STREQUAL darwin_embedded)
    if(CPU STREQUAL armv7)
      set(ARCH armv7s)
    elseif(CPU STREQUAL arm64)
      set(ARCH arm64)
    else()
      message(SEND_ERROR "Unknown CPU: ${CPU}")
    endif()
    set(EXTRAS "-arch ${ARCH}")
    set(EXTRAS "${EXTRAS} -miphoneos-version-min=${SDK_VERSION} -no-integrated-as -target ${ARCH}-apple-darwin")
    set(EXTRA_ARGS "--disable-assembly")
  else()
    set(EXTRAS "-arch ${CPU}")
    set(EXTRAS "${EXTRAS} -mmacosx-version-min=${SDK_VERSION}")
    set(EXTRA_ARGS "--with-pic")
    if(IS64BIT)
      set(EXTRA_ARGS "${EXTRA_ARGS} ABI=64")
    else()
      set(EXTRAS "${EXTRAS} -m32")
      set(EXTRA_ARGS "${EXTRA_ARGS} ABI=32")
    endif()
  endif()

  set(CLANG ${CMAKE_C_COMPILER})
  set(CLANGPP ${CMAKE_C_COMPILER} -E)
  set(CFLAGS "${BITCODE_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT} -Wno-error -Wno-implicit-function-declaration ${EXTRAS} -fvisibility=hidden")
  execute_process(COMMAND xcode-select --print-path
      OUTPUT_VARIABLE DEVELOPER
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)

  externalproject_add(gmp
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}
                      CONFIGURE_COMMAND
                          bash --login -c "PATH=${PLATFORM}/Developer/usr/bin:${DEVELOPER}/usr/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin \
                          ./configure \
                          --prefix=${CMAKE_BINARY_DIR}/gmpbuild \
                          CC='${CLANG} ${CFLAGS}' \
                          LDFLAGS='${CFLAGS}' \
                          CPP='${CLANG} -E' \
                          CPPFLAGS='${CFLAGS}' \
                          --host=x86_64-apple-darwin \
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
  if(CORE_SYSTEM_NAME MATCHES "linux")
    if (CPU MATCHES "i.86")
      list(APPEND gmp_conf ABI=32)
    endif()
  elseif(CORE_SYSTEM_NAME MATCHES "android")
    list(APPEND gmp_conf --with-pic)
  endif()

  externalproject_add(gmp
                      SOURCE_DIR ${CMAKE_SOURCE_DIR}
                      CONFIGURE_COMMAND <SOURCE_DIR>/configure
                        --prefix=${CMAKE_INSTALL_PREFIX}
                        --disable-assembly
                        --enable-static
                        --disable-shared
                        ${gmp_conf})
endif()

install(CODE "Message(Done)")