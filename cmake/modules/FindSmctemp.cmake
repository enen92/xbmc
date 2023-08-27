#.rst:
# FindSmctemp
# -------
# Finds the smctemp library
#
# This will define the following variables::
#
# SMCTEMP_FOUND - system has smctemp
# SMCTEMP_INCLUDE_DIRS - the smctemp include directory
# SMCTEMP_LIBRARIES - the smctemp libraries
# SMCTEMP_DEFINITIONS - the smctemp definitions
#
# and the following imported targets::
#
#   SMCTEMP::SMCTEMP   - The smctemp library

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SMCTEMP libsmctemp QUIET)
endif()

find_path(SMCTEMP_INCLUDE_DIR NAMES smctemp.h
                           PATHS ${PC_SMCTEMP_INCLUDEDIR})
find_library(SMCTEMP_LIBRARY NAMES smctemp
                          PATHS ${PC_SMCTEMP_LIBDIR})

set(SMCTEMP_VERSION ${PC_SMCTEMP_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Smctemp
                                  REQUIRED_VARS SMCTEMP_LIBRARY SMCTEMP_INCLUDE_DIR
                                  VERSION_VAR SMCTEMP_VERSION)

if(SMCTEMP_FOUND)
  set(SMCTEMP_LIBRARIES ${SMCTEMP_LIBRARY})
  set(SMCTEMP_INCLUDE_DIRS ${SMCTEMP_INCLUDE_DIR})

  if(NOT TARGET SMCTEMP::SMCTEMP)
    add_library(SMCTEMP::SMCTEMP UNKNOWN IMPORTED)
    set_target_properties(SMCTEMP::SMCTEMP PROPERTIES
                                   IMPORTED_LOCATION "${SMCTEMP_LIBRARY}"
                                   INTERFACE_INCLUDE_DIRECTORIES "${SMCTEMP_INCLUDE_DIR}")
  endif()
endif()

mark_as_advanced(SMCTEMP_INCLUDE_DIR SMCTEMP_LIBRARY)
