# - Find PThreads
# Find the native pthread for windows includes and library
#
#   PTHREADS_FOUND        - True if pthread for windows found.
#   PTHREADS_INCLUDE_DIRS - where to find pthread.h, etc.
#   PTHREADS_LIBRARIES    - List of libraries when using pthread for windows.
#

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_PTHREADS pthreads QUIET)
endif()

find_path(PTHREADS_INCLUDE_DIRS NAMES pthread.h
                                  PATHS ${PC_PTHREADS_INCLUDEDIR})
find_library(PTHREADS_LIBRARIES NAMES pthreads
                                  PATHS ${PC_PTHREADS_LIBDIR})

include("FindPackageHandleStandardArgs")
find_package_handle_standard_args(PThreads REQUIRED_VARS PTHREADS_INCLUDE_DIRS PTHREADS_LIBRARIES)

mark_as_advanced(PTHREADS_INCLUDE_DIRS PTHREADS_LIBRARIES)
