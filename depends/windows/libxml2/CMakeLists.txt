cmake_minimum_required(VERSION 3.5)
project(xml2)

enable_language(C)

find_package(ZLIB REQUIRED)
list(APPEND CMAKE_REQUIRED_INCLUDES ${ZLIB_INCLUDE_DIRS})

if(NOT WIN32)
  include(CheckIncludeFile)
  include(CheckFunctionExists)
  include(CheckStructHasMember)
  include(CheckSymbolExists)

  file(READ config.h.in FILE_CONTENT)
  string(REPLACE "#undef" "#cmakedefine" MODIFIED_FILE_CONTENT "${FILE_CONTENT}")
  file(WRITE config.h.cmake "${MODIFIED_FILE_CONTENT}")

  #/* Type cast for the gethostbyname() argument */
  #undef GETHOSTBYNAME_ARG_CAST

  #/* Define to 1 if you have the <ansidecl.h> header file. */
  check_include_file(ansidecl.h HAVE_ANSIDECL_H)

  #/* Define to 1 if you have the <arpa/inet.h> header file. */
  check_include_file(arpa/inet.h HAVE_ARPA_INET_H)

  #/* Define to 1 if you have the <arpa/nameser.h> header file. */
  check_include_file(arpa/nameser.h HAVE_ARPA_NAMESER_H)

  #/* Whether struct sockaddr::__ss_family exists */
  check_struct_has_member("struct sockaddr" __ss_family sys/socket.h HAVE_BROKEN_SS_FAMILY)

  #/* Define to 1 if you have the `class' function. */
  check_function_exists(class HAVE_CLASS)

  #/* Define to 1 if you have the <ctype.h> header file. */
  check_include_file(ctype.h HAVE_CTYPE_H)

  #/* Define to 1 if you have the <dirent.h> header file. */
  check_include_file(dirent.h HAVE_DIRENT_H)

  #/* Define to 1 if you have the <dlfcn.h> header file. */
  check_include_file(dlfcn.h HAVE_DLFCN_H)

  #/* Have dlopen based dso */
  #undef HAVE_DLOPEN

  #/* Define to 1 if you have the <dl.h> header file. */
  check_include_file(dl.h HAVE_DL_H)

  #/* Define to 1 if you have the <errno.h> header file. */
  check_include_file(errno.h HAVE_ERRNO_H)

  #/* Define to 1 if you have the <fcntl.h> header file. */
  check_include_file(fcntl.h HAVE_FCNTL_H)

  #/* Define to 1 if you have the `finite' function. */
  check_function_exists(finite HAVE_FINITE)

  #/* Define to 1 if you have the <float.h> header file. */
  check_include_file(float.h HAVE_FLOAT_H)

  #/* Define to 1 if you have the `fpclass' function. */
  check_function_exists(fpclass HAVE_FPCLASS)

  #/* Define to 1 if you have the `fprintf' function. */
  check_function_exists(fprintf HAVE_FPRINTF)

  #/* Define to 1 if you have the `fp_class' function. */
  check_function_exists(fp_class HAVE_FP_CLASS)

  #/* Define to 1 if you have the <fp_class.h> header file. */
  check_include_file(fp_class.h HAVE_FP_CLASS_H)

  #/* Define to 1 if you have the `ftime' function. */
  check_function_exists(ftime HAVE_FTIME)

  #/* Define if getaddrinfo is there */
  check_function_exists(getaddrinfo HAVE_GETADDRINFO)

  #/* Define to 1 if you have the `gettimeofday' function. */
  check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)

  #/* Define to 1 if you have the <ieeefp.h> header file. */
  check_include_file(ieeefp.h HAVE_IEEEFP_H)

  #/* Define to 1 if you have the <inttypes.h> header file. */
  check_include_file(inttypes.h HAVE_INTTYPES_H)

  #/* Define to 1 if you have the `isascii' function. */
  check_function_exists(isascii HAVE_ISASCII)

  #/* Define if isinf is there */
  check_function_exists(isinf HAVE_ISINF)

  #/* Define if isnan is there */
  check_function_exists(isnan HAVE_ISNAN)

  #/* Define to 1 if you have the `isnand' function. */
  check_function_exists(isnand HAVE_ISNAND)

  #/* Define if history library is there (-lhistory) */
  #undef HAVE_LIBHISTORY

  #/* Have compression library */
  #undef HAVE_LIBLZMA

  #/* Define if pthread library is there (-lpthread) */
  #undef HAVE_LIBPTHREAD

  #/* Define if readline library is there (-lreadline) */
  #undef HAVE_LIBREADLINE

  #/* Have compression library */
  #undef HAVE_LIBZ

  #/* Define to 1 if you have the <limits.h> header file. */
  check_include_file(limits.h HAVE_LIMITS_H)

  #/* Define to 1 if you have the `localtime' function. */
  check_function_exists(localtime HAVE_LOCALTIME)

  #/* Define to 1 if you have the <lzma.h> header file. */
  check_include_file(lzma.h HAVE_LZMA_H)

  #/* Define to 1 if you have the <malloc.h> header file. */
  check_include_file(malloc.h HAVE_MALLOC_H)

  #/* Define to 1 if you have the <math.h> header file. */
  check_include_file(math.h HAVE_MATH_H)

  #/* Define to 1 if you have the <memory.h> header file. */
  check_include_file(memory.h HAVE_MEMORY_H)

  #/* Define to 1 if you have the `mmap' function. */
  check_function_exists(mmap HAVE_MMAP)

  #/* Define to 1 if you have the `munmap' function. */
  check_function_exists(munmap HAVE_MUNMAP)

  #/* mmap() is no good without munmap() */
  #if defined(HAVE_MMAP) && !defined(HAVE_MUNMAP)
  #  undef /**/ HAVE_MMAP
  #endif

  #/* Define to 1 if you have the <nan.h> header file. */
  check_include_file(nan.h HAVE_NAN_H)

  #/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
  check_include_file(ndir.h HAVE_NDIR_H)

  #/* Define to 1 if you have the <netdb.h> header file. */
  check_include_file(netdb.h HAVE_NETDB_H)

  #/* Define to 1 if you have the <netinet/in.h> header file. */
  check_include_file(netinet/in.h HAVE_NETINET_IN_H)

  #/* Define to 1 if you have the <poll.h> header file. */
  check_include_file(poll.h HAVE_POLL_H)

  #/* Define to 1 if you have the `printf' function. */
  check_function_exists(printf HAVE_PRINTF)

  #/* Define if <pthread.h> is there */
  #undef HAVE_PTHREAD_H

  #/* Define to 1 if you have the `putenv' function. */
  check_function_exists(putenv HAVE_PUTENV)

  #/* Define to 1 if you have the `rand' function. */
  check_function_exists(rand HAVE_RAND)

  #/* Define to 1 if you have the `rand_r' function. */
  check_function_exists(rand_r HAVE_RAND_R)

  #/* Define to 1 if you have the <resolv.h> header file. */
  check_include_file(resolv.h HAVE_RESOLV_H)

  #/* Have shl_load based dso */
  #undef HAVE_SHLLOAD

  #/* Define to 1 if you have the `signal' function. */
  check_function_exists(signal HAVE_SIGNAL)

  #/* Define to 1 if you have the <signal.h> header file. */
  check_include_file(signal.h HAVE_SIGNAL_H)

  #/* Define to 1 if you have the `snprintf' function. */
  #check_function_exists(snprintf HAVE_SNPRINTF)
  check_symbol_exists(snprintf stdio.h HAVE_SNPRINTF)

  #/* Define to 1 if you have the `sprintf' function. */
  check_function_exists(sprintf HAVE_SPRINTF)

  #/* Define to 1 if you have the `srand' function. */
  check_function_exists(srand HAVE_SRAND)

  #/* Define to 1 if you have the `sscanf' function. */
  check_function_exists(sscanf HAVE_SSCANF)

  #/* Define to 1 if you have the `stat' function. */
  check_function_exists(stat HAVE_STAT)

  #/* Define to 1 if you have the <stdarg.h> header file. */
  check_include_file(stdarg.h HAVE_STDARG_H)

  #/* Define to 1 if you have the <stdint.h> header file. */
  check_include_file(stdint.h HAVE_STDINT_H)

  #/* Define to 1 if you have the <stdlib.h> header file. */
  check_include_file(stdlib.h HAVE_STDLIB_H)

  #/* Define to 1 if you have the `strdup' function. */
  check_function_exists(strdup HAVE_STRDUP)

  #/* Define to 1 if you have the `strerror' function. */
  check_function_exists(strerror HAVE_STRERROR)

  #/* Define to 1 if you have the `strftime' function. */
  check_function_exists(strftime HAVE_STRFTIME)

  #/* Define to 1 if you have the <strings.h> header file. */
  check_include_file(strings.h HAVE_STRINGS_H)

  #/* Define to 1 if you have the <string.h> header file. */
  check_include_file(string.h HAVE_STRING_H)

  #/* Define to 1 if you have the `strndup' function. */
  check_function_exists(strndup HAVE_STRNDUP)

  #/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
  #   */
  check_include_file(sys/dir.h HAVE_SYS_DIR_H)

  #/* Define to 1 if you have the <sys/mman.h> header file. */
  check_include_file(sys/mman.h HAVE_SYS_MMAN_H)

  #/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
  #   */
  check_include_file(sys/ndir.h HAVE_SYS_NDIR_H)

  #/* Define to 1 if you have the <sys/select.h> header file. */
  check_include_file(sys/select.h HAVE_SYS_SELECT_H)

  #/* Define to 1 if you have the <sys/socket.h> header file. */
  check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)

  #/* Define to 1 if you have the <sys/stat.h> header file. */
  check_include_file(sys/stat.h HAVE_SYS_STAT_H)

  #/* Define to 1 if you have the <sys/timeb.h> header file. */
  check_include_file(sys/timeb.h HAVE_SYS_TIMEB_H)

  #/* Define to 1 if you have the <sys/time.h> header file. */
  check_include_file(sys/time.h HAVE_SYS_TIME_H)

  #/* Define to 1 if you have the <sys/types.h> header file. */
  check_include_file(sys/types.h HAVE_SYS_TYPES_H)

  #/* Define to 1 if you have the `time' function. */
  check_function_exists(time HAVE_TIME)

  #/* Define to 1 if you have the <time.h> header file. */
  check_include_file(time.h HAVE_TIME_H)

  #/* Define to 1 if you have the <unistd.h> header file. */
  check_include_file(unistd.h HAVE_UNISTD_H)

  #/* Whether va_copy() is available */
  check_symbol_exists(va_copy stdarg.h HAVE_VA_COPY)

  #/* Define to 1 if you have the `vfprintf' function. */
  check_function_exists(vfprintf HAVE_VFPRINTF)

  #/* Define to 1 if you have the `vsnprintf' function. */
  check_function_exists(vsnprintf HAVE_VSNPRINTF)

  #/* Define to 1 if you have the `vsprintf' function. */
  check_function_exists(vsprintf HAVE_VSPRINTF)

  #/* Define to 1 if you have the <zlib.h> header file. */
  check_include_file(zlib.h HAVE_ZLIB_H)

  #/* Define to 1 if you have the `_stat' function. */
  check_function_exists(_stat HAVE__STAT)

  #/* Whether __va_copy() is available */
  check_symbol_exists(__va_copy stdarg.h HAVE___VA_COPY)

  #/* Define as const if the declaration of iconv() needs const. */
  #undef ICONV_CONST
  set(ICONV_CONST const)

  #/* Define to the sub-directory where libtool stores uninstalled libraries. */
  #undef LT_OBJDIR

  #/* Name of package */
  #undef PACKAGE

  #/* Define to the address where bug reports for this package should be sent. */
  #undef PACKAGE_BUGREPORT

  #/* Define to the full name of this package. */
  #undef PACKAGE_NAME

  #/* Define to the full name and version of this package. */
  #undef PACKAGE_STRING

  #/* Define to the one symbol short name of this package. */
  #undef PACKAGE_TARNAME

  #/* Define to the home page for this package. */
  #undef PACKAGE_URL

  #/* Define to the version of this package. */
  #undef PACKAGE_VERSION

  #/* Type cast for the send() function 2nd arg */
  #undef SEND_ARG2_CAST

  #/* Define to 1 if you have the ANSI C header files. */
  #undef STDC_HEADERS

  #/* Support for IPv6 */
  #undef SUPPORT_IP6

  #/* Define if va_list is an array type */
  check_c_source_compiles("#include <stdarg.h>\n int main() {va_list list1, list2; list1 = list2;}" NOT_VA_LIST_IS_ARRAY)
  if(NOT NOT_VA_LIST_IS_ARRAY)
    set(VA_LIST_IS_ARRAY 1)
  endif()

  #/* Version number of package */
  #undef VERSION

  #/* Determine what socket length (socklen_t) data type is */
  #undef XML_SOCKLEN_T

  #/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
  #   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
     #define below would cause a syntax error. */
  #undef _UINT32_T

  #/* Using the Win32 Socket implementation */
  #undef _WINSOCKAPI_

  #/* ss_family is not defined here, use __ss_family instead */
  #undef ss_family

  #/* Define to the type of an unsigned integer type of width exactly 32 bits if
  #   such a type exists and the standard includes do not define it. */
  #undef uint32_t
endif()

set(LIBXML_MAJOR_VERSION 2)
set(LIBXML_MINOR_VERSION 9)
set(LIBXML_MICRO_VERSION 3)
set(LIBXML_MICRO_VERSION_SUFFIX)
set(LIBXML_VERSION ${LIBXML_MAJOR_VERSION}.${LIBXML_MINOR_VERSION}.${LIBXML_MICRO_VERSION}${LIBXML_MICRO_VERSION_SUFFIX})
math(EXPR LIBXML_VERSION_INFO "${LIBXML_MAJOR_VERSION} + ${LIBXML_MINOR_VERSION}")
set(LIBXML_VERSION_INFO ${LIBXML_VERSION_INFO}:${LIBXML_MICRO_VERSION}:${LIBXML_MINOR_VERSION})
math(EXPR LIBXML_VERSION_NUMBER "${LIBXML_MAJOR_VERSION} * 10000 + ${LIBXML_MINOR_VERSION} * 100 + ${LIBXML_MICRO_VERSION}")
set(VERSION ${LIBXML_VERSION})
set(LIBXML_VERSION_EXTRA)

set(WITH_TRIO 0)
set(WITH_THREADS 0)
set(WITH_THREAD_ALLOC 0)
set(WITH_TREE 1)
set(WITH_OUTPUT 1)
set(WITH_PUSH 1)
set(WITH_READER 1)
set(WITH_PATTERN 0)
set(WITH_WRITER 0)
set(WITH_SAX1 0)
set(WITH_FTP 0)
set(WITH_HTTP 0)
set(WITH_VALID 0)
set(WITH_HTML 1)
set(WITH_LEGACY 0)
set(WITH_C14N 0)
set(WITH_CATALOG 0)
set(WITH_DOCB 0)
set(WITH_XPATH 0)
set(WITH_XPTR 0)
set(WITH_XINCLUDE 0)
set(WITH_ICONV 1)
set(WITH_ICU 0)
set(WITH_ISO8859X 1)
set(WITH_DEBUG 0)
set(WITH_MEM_DEBUG 0)
set(WITH_RUN_DEBUG 0)
set(WITH_REGEXPS 0)
set(WITH_SCHEMAS 0)
set(WITH_SCHEMATRON 0)
set(WITH_MODULES 0)
set(MODULE_EXTENSION 0)
set(WITH_ZLIB 1)
set(WITH_LZMA 0)

if(NOT WIN32)
  configure_file(config.h.cmake ${PROJECT_SOURCE_DIR}/config.h)
else()
  file(COPY include/win32config.h DESTINATION ${PROJECT_SOURCE_DIR})
  file(COPY win32/VC10/config.h DESTINATION ${PROJECT_SOURCE_DIR})
endif()

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "$")
set(exec_prefix "${exec_prefix}{")
set(exec_prefix "${exec_prefix}prefix")
set(exec_prefix "${exec_prefix}}")

set(libdir "$")
set(libdir "${libdir}{")
set(libdir "${libdir}exec_prefix")
set(libdir "${libdir}}/lib")

set(includedir "$")
set(includedir "${includedir}{")
set(includedir "${includedir}prefix")
set(includedir "${includedir}}/include")

set(XML_INCLUDEDIR "-I$")
set(XML_INCLUDEDIR "${XML_INCLUDEDIR}{")
set(XML_INCLUDEDIR "${XML_INCLUDEDIR}includedir")
set(XML_INCLUDEDIR "${XML_INCLUDEDIR}}/libxml2")

set(Z_LIBS "-lz")
set(ICONV_LIBS "-liconv")

configure_file(include/libxml/xmlversion.h.in ${PROJECT_SOURCE_DIR}/include/libxml/xmlversion.h)
configure_file(libxml-2.0.pc.in ${PROJECT_SOURCE_DIR}/libxml-2.0.pc)

include_directories(${ZLIB_INCLUDE_DIRS} include)
file(GLOB SOURCES *.c)

file(GLOB SOURCES_EXCLUDE test*.c runsuite.c runtest.c runxmlconf.c trio*.c xmlcatalog.c)
list(REMOVE_ITEM SOURCES ${SOURCES_EXCLUDE})

add_library(xml2 STATIC ${SOURCES})
target_link_libraries(xml2 ${ZLIB_LIBRARIES})

if(NOT WIN32)
  set_target_properties(xml2 PROPERTIES COMPILE_FLAGS "-fPIC")
else()
  add_definitions(-DHAVE_ZLIB_H -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  set_target_properties(xml2 PROPERTIES COMPILE_FLAGS "/W1")
endif()

install(TARGETS xml2 DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY include/libxml DESTINATION ${CMAKE_INSTALL_PREFIX}/include/libxml2/)
install(FILES libxml-2.0.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
