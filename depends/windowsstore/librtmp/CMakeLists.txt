cmake_minimum_required(VERSION 3.5)

project(librtmp C CXX)

add_definitions(
	-DUSE_OPENSSL
	-D_CRT_NONSTDC_NO_DEPRECATE
	-D_CRT_SECURE_NO_WARNINGS
	-D_WINSOCK_DEPRECATED_NO_WARNINGS
)

find_package(OpenSSL REQUIRED)
find_package(ZLib REQUIRED)

include_directories(
	${CMAKE_SOURCE_DIR}/librtmp
	${OPENSSL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
)

add_library(librtmp
	librtmp/amf.h
	librtmp/amf.c
	librtmp/bytes.h
	librtmp/dh.h
	librtmp/dhgroups.h
	librtmp/handshake.h
	librtmp/hashswf.c
	librtmp/http.h
	librtmp/log.h
	librtmp/log.c
	librtmp/parseurl.c
	librtmp/rtmp.h
	librtmp/rtmp.c
	librtmp/rtmp_sys.h
)
if(MSVC AND CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
  target_sources(librtmp PRIVATE
    librtmp/uwpapi.cpp
  )
  set_source_files_properties(librtmp/uwpapi.cpp PROPERTIES COMPILE_FLAGS "/TP /ZW")
endif()

target_link_libraries(librtmp ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} Ws2_32 winmm gdi32)

target_compile_options(librtmp PRIVATE -DEXPORT)

install(TARGETS librtmp
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

install(FILES
	librtmp/amf.h
	librtmp/http.h
	librtmp/log.h
	librtmp/rtmp.h
	DESTINATION include/librtmp
)
