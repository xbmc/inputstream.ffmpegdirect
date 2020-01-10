# - Try to find bzip2
# Once done this will define
#
# BZIP2_FOUND - system has bzip2
# BZIP2_INCLUDE_DIRS - the bzip2 include directory
# BZIP2_LIBRARIES - The bzip2 libraries

include(FindPkgConfig)
find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_BZIP2 bzip2 QUIET)
endif()

find_path(BZIP2_INCLUDE_DIRS bzlib.h PATHS ${PC_BZIP2_INCLUDEDIR})
find_library(BZIP2_LIBRARIES bz2 PATHS ${PC_BZIP2_LIBDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gmp DEFAULT_MSG BZIP2_INCLUDE_DIRS BZIP2_LIBRARIES)

mark_as_advanced(BZIP2_INCLUDE_DIRS BZIP2_LIBRARIES BZIP2_DEFINITIONS)
#mark_as_advanced(BZIP2_DEFINITIONS)

