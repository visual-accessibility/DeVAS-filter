# - Find FFTW
# Find the native FFTW includes and library
# This module defines
#  FFTW_INCLUDE_DIR, where to find fftw3.h, etc.
#  FFTW_LIBRARIES, the libraries needed to use FFTW.
#  FFTW_FOUND, If false, do not try to use FFTW.
# also defined, but not for general use are
#  FFTW_LIBRARY, where to find the FFTW library.

#=============================================================================
# based on FindJPEG.cmake
#
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

find_path ( FFTW_INCLUDE_DIR fftw3.h )

set ( FFTW_NAMES ${FFTW_NAMES} fftw3 )
find_library ( FFTW_LIBRARY NAMES ${FFTW_NAMES} )

set (FFTWF_NAMES ${FFTWF_NAMES} fftw3f )
find_library ( FFTWF_LIBRARY NAMES ${FFTWF_NAMES} )

# handle the QUIETLY and REQUIRED arguments and set FFTW_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS ( FFTW
	DEFAULT_MSG
	FFTWF_LIBRARY
	FFTW_INCLUDE_DIR
	)

if ( FFTW_FOUND )
  set ( FFTW_LIBRARIES ${FFTW_LIBRARY} ${FFTWF_LIBRARY} )
endif ( )

# Deprecated declarations.
set (NATIVE_FFTW_INCLUDE_PATH ${FFTW_INCLUDE_DIR} )
if(FFTW_LIBRARY)
  get_filename_component (NATIVE_FFTW_LIB_PATH ${FFTW_LIBRARY} PATH)
endif()

mark_as_advanced(FFTW_LIBRARY FFTW_INCLUDE_DIR )
