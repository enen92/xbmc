#.rst:
# FindICONV
# --------
# Finds the ICONV library
#
# This will define the following target:
#
#   Iconv::Iconv - The ICONV library

if(NOT TARGET Iconv::Iconv)
  find_path(ICONV_INCLUDE_DIR NAMES iconv.h
                              HINTS ${DEPENDS_PATH}/include)

  find_library(ICONV_LIBRARY NAMES iconv libiconv c
                             HINTS ${DEPENDS_PATH}/lib)

  set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
  check_function_exists(iconv HAVE_ICONV_FUNCTION)
  if(NOT HAVE_ICONV_FUNCTION)
    check_function_exists(libiconv HAVE_LIBICONV_FUNCTION2)
    set(HAVE_ICONV_FUNCTION ${HAVE_LIBICONV_FUNCTION2})
    unset(HAVE_LIBICONV_FUNCTION2)
  endif()
  unset(CMAKE_REQUIRED_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Iconv
                                    REQUIRED_VARS ICONV_LIBRARY ICONV_INCLUDE_DIR HAVE_ICONV_FUNCTION)

  if(ICONV_FOUND)
    # Libc causes grief for linux, so search if found library is libc.* and only
    # create imported TARGET if its not
    if(NOT ${ICONV_LIBRARY} MATCHES ".*libc\..*")
      add_library(Iconv::Iconv UNKNOWN IMPORTED)
      set_target_properties(Iconv::Iconv PROPERTIES
                                         IMPORTED_LOCATION "${ICONV_LIBRARY}"
                                         INTERFACE_INCLUDE_DIRECTORIES "${ICONV_INCLUDE_DIR}")
      set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP Iconv::Iconv)
    endif()
  endif()
endif()
