# DEVA version of FindPNG that supports Windows cross-build

if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  message ( FATAL_ERROR "FindZLIB_Windows: not Windows cross-compile!" )
endif ( )

set_property ( GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON )

INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindZLIB_Windows.cmake )
if(ZLIB_FOUND)

  set ( CMAKE_FIND_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external-libs )
  find_path ( PNG_INCLUDE_DIR png.h )

  find_library ( PNG_LIBRARY libpng.a )

  # handle the QUIETLY and REQUIRED arguments and set PNG_FOUND to TRUE if
  # all listed variables are TRUE
  include ( FindPackageHandleStandardArgs )
  FIND_PACKAGE_HANDLE_STANDARD_ARGS ( PNG
    DEFAULT_MSG
    PNG_LIBRARY
    PNG_INCLUDE_DIR
    )

  if ( PNG_FOUND )
    set ( PNG_INCLUDE_DIRS ${PNG_PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} )
    set ( PNG_INCLUDE_DIR ${PNG_INCLUDE_DIRS} ) # for backward compatiblity
    set ( PNG_LIBRARIES ${PNG_LIBRARY} ${ZLIB_LIBRARY} )
  endif ( )

  # Deprecated declarations.
  set ( NATIVE_PNG_INCLUDE_PATH ${PNG_INCLUDE_DIR} )
  if ( PNG_LIBRARY )
    get_filename_component ( NATIVE_PNG_LIB_PATH ${PNG_LIBRARY} PATH )
  endif()

endif()

mark_as_advanced ( PNG_LIBRARY PNG_INCLUDE_DIR )
