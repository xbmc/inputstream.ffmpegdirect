project(gmp)

cmake_minimum_required(VERSION 3.5)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

if(CORE_SYSTEM_NAME MATCHES "linux")
  if (CPU MATCHES "i.86")
    list(APPEND gmp_conf ABI=32)
  endif()
  list(APPEND gmp_conf --with-pic)
elseif(CORE_SYSTEM_NAME MATCHES "darwin_embedded")
  list(APPEND gmp_conf --disable-assembly)
elseif(CORE_SYSTEM_NAME MATCHES "osx")
  list(APPEND gmp_conf --with-pic)
endif()

#
# Darwin based OSes will use -isysroot, however libtool will expect --sysroot
# So we hack around this otherwise libttol won't be able to find the standard C headers
#
set (COMPILER_WITH_LIBTOOL_SYSROOT_APPLE "")
if(CORE_SYSTEM_NAME STREQUAL osx)# OR CORE_SYSTEM_NAME STREQUAL ios OR CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  set (COMPILER_WITH_LIBTOOL_SYSROOT_APPLE
  "CC_FOR_BUILD=${CMAKE_C_COMPILER} --sysroot ${CMAKE_OSX_SYSROOT}"
  "CPP_FOR_BUILD=${CMAKE_C_COMPILER} -E --sysroot ${CMAKE_OSX_SYSROOT}"
  )
elseif(CORE_SYSTEM_NAME STREQUAL ios OR CORE_SYSTEM_NAME STREQUAL darwin_embedded)
  set (COMPILER_WITH_LIBTOOL_SYSROOT_APPLE
  #"CC=${CMAKE_C_COMPILER} -isysroot /Applications/Xcode10.2.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS12.2.sdk"
  #"CPP=${CMAKE_C_COMPILER} -E -isysroot /Applications/Xcode10.2.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS12.2.sdk"
  "CC_FOR_BUILD=${CMAKE_C_COMPILER}"
  "CPP_FOR_BUILD=${CMAKE_C_COMPILER} -E"
  #"CFLAGS=-isysroot /Applications/Xcode10.2.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS12.2.sdk/ -Wno-error -Wno-implicit-function-declaration -miphoneos-version-min=11.0 -no-integrated-as"
  #"CFLAGS=-no-integrated-as"
  )
endif()

include(ExternalProject)
externalproject_add(gmp
                    SOURCE_DIR ${CMAKE_SOURCE_DIR}
                    CONFIGURE_COMMAND <SOURCE_DIR>/configure
                      ${COMPILER_WITH_LIBTOOL_SYSROOT_APPLE}
                      --prefix=${CMAKE_INSTALL_PREFIX}
                      --disable-shared
                      ${gmp_conf})

install(CODE "Message(Done)")

