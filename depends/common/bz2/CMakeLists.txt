cmake_minimum_required(VERSION 3.5)
project(bzip2)

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")
elseif(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
endif()

set(SOURCES blocksort.c
            huffman.c
            crctable.c
            randtable.c
            compress.c
            decompress.c
            bzlib.c)

add_library(bz2 ${SOURCES})
set_property(TARGET bz2 PROPERTY POSITION_INDEPENDENT_CODE ON)

install(TARGETS bz2 DESTINATION lib)
install(FILES bzlib.h DESTINATION include)
