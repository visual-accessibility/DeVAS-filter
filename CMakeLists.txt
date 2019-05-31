cmake_minimum_required(VERSION 3.0)
PROJECT ( DeVAS_FILTER )

message ( STATUS "CMAKE_SYSTEM_NAME = " ${CMAKE_SYSTEM_NAME} )

if ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  # Linux
  option (DeVAS_FILTER_USE_CAIRO "Build using Cairo library" ON )
elseif ( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
  # MacOS
  option (DeVAS_FILTER_USE_CAIRO "Build using Cairo library" OFF )
elseif ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  # Windows
  option (DeVAS_FILTER_USE_CAIRO "Build using Cairo library" OFF )
else ( )
  message ( FATAL_ERROR "unknown CMAKE_SYSTEM_NAME (" ${CMAKE_SYSTEM_NAME} ")" )
endif ( )

# SET ( CMAKE_VERBOSE_MAKEFILE ON )

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  # Radiance tries to redefine things it shouldn't with mignw64 cross-compiling
  set ( CMAKE_C_FLAGS "-Wall -g -O3 -D_mingw64_cross" )
else ( )
  set ( CMAKE_C_FLAGS "-Wall -g -O3" )
endif ( )

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindPNG_Windows.cmake )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindFFTW_Windows.cmake )
elseif ( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindPNG_Mac.cmake )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindFFTW_Mac.cmake )
elseif ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  find_package ( PNG REQUIRED )
  if ( DeVAS_FILTER_USE_CAIRO )
    include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindCairo.cmake )
  endif ( )
  include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindFFTW.cmake )
else ( )
  message ( FATAL_ERROR "unknown CMAKE_SYSTEM_NAME (" ${CMAKE_SYSTEM_NAME} ")" )
endif ( )

if ( DeVAS_FILTER_USE_CAIRO )
  INCLUDE_DIRECTORIES (
  ${FFTW_INCLUDE_DIR}
  ${PNG_INCLUDE_DIR}
  ${CAIRO_INCLUDE_DIR}
  )
else ( )
  INCLUDE_DIRECTORIES (
  ${FFTW_INCLUDE_DIR}
  ${PNG_INCLUDE_DIR}
  )
endif ( )

ADD_EXECUTABLE ( devas-filter devas-commandline.c
	devas-filter.c
	ChungLeggeCSF.c
	dilate.c
	devas-image.c
	devas-utils.c
	devas-margin.c
	radianceIO.c
	radiance-header.c
	acuity-conversion.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	)
TARGET_COMPILE_DEFINITIONS ( devas-filter PRIVATE DeVAS_USE_FFTW3_ALLOCATORS )
TARGET_LINK_LIBRARIES ( devas-filter
	${FFTW_LIBRARIES}
	-lm
	)

if ( DeVAS_FILTER_USE_CAIRO )

    ADD_EXECUTABLE ( devas-visibility devas-commandline.c
	devas-filter.c
	ChungLeggeCSF.c
	devas-image.c
	devas-margin.c
	devas-utils.c
	dilate.c
	devas-canny.c
	devas-gblur.c
	devas-gblur-fft.c
	radianceIO.c
	radiance-header.c
	acuity-conversion.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	read-geometry.c
	devas-visibility.c
	geometry-discontinuities.c
	directional-maxima.c
	visualize-hazards.c
	devas-sRGB.c
	devas-png.c
	devas-add-text.c
	)
    TARGET_COMPILE_DEFINITIONS ( devas-visibility PRIVATE DeVAS_VISIBILITY )
    TARGET_COMPILE_DEFINITIONS ( devas-visibility PRIVATE DeVAS_USE_FFTW3_ALLOCATORS )
    TARGET_COMPILE_DEFINITIONS ( devas-visibility PRIVATE DeVAS_USE_CAIRO )
    TARGET_LINK_LIBRARIES ( devas-visibility
	${FFTW_LIBRARIES}
	${PNG_LIBRARIES}
	${CAIRO_LIBRARIES}
	-lm
	)

else ( )

   ADD_EXECUTABLE ( devas-visibility devas-commandline.c
	devas-filter.c
	ChungLeggeCSF.c
	devas-image.c
	devas-margin.c
	devas-utils.c
	dilate.c
	devas-canny.c
	devas-gblur.c
	devas-gblur-fft.c
	radianceIO.c
	radiance-header.c
	acuity-conversion.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	read-geometry.c
	devas-visibility.c
	geometry-discontinuities.c
	directional-maxima.c
	visualize-hazards.c
	devas-sRGB.c
	devas-png.c
	)
    TARGET_COMPILE_DEFINITIONS ( devas-visibility PRIVATE DeVAS_VISIBILITY )
    TARGET_COMPILE_DEFINITIONS ( devas-visibility PRIVATE DeVAS_USE_FFTW3_ALLOCATORS )
    TARGET_LINK_LIBRARIES ( devas-visibility
	${FFTW_LIBRARIES}
	${PNG_LIBRARIES}
	-lm
	)

endif ( )

ADD_EXECUTABLE ( make-coordinates-file make-coordinates-file.c
	radiance-header.c
	radiance/badarg.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/words.c
	radiance/timegm.c
	)
TARGET_LINK_LIBRARIES ( make-coordinates-file
	-lm
	)

# if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
    ADD_EXECUTABLE ( devas-add-res-to-ASCII devas-add-res-to-ASCII.c
	radiance-header.c
	radiance/badarg.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/words.c
	radiance/timegm.c
	)
    TARGET_LINK_LIBRARIES ( devas-add-res-to-ASCII
	-lm
	)
# endif ( )

ADD_EXECUTABLE ( devas-visualize-geometry devas-visualize-geometry.c
	devas-image.c
	read-geometry.c
	devas-sRGB.c
	devas-png.c
	radiance-header.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	)
TARGET_LINK_LIBRARIES ( devas-visualize-geometry
	${PNG_LIBRARIES}
	-lm
	)

if ( DeVAS_FILTER_USE_CAIRO )

  ADD_EXECUTABLE ( devas-compare-boundaries devas-compare-boundaries.c
	devas-image.c
	read-geometry.c
	dilate.c
	visualize-hazards.c
	devas-add-text.c
	devas-sRGB.c
	devas-png.c
	radiance-header.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	)
  TARGET_COMPILE_DEFINITIONS ( devas-compare-boundaries PRIVATE DeVAS_USE_CAIRO )
  TARGET_LINK_LIBRARIES ( devas-compare-boundaries
	${PNG_LIBRARIES}
	${CAIRO_LIBRARIES}
	-lm
	)

else ( )

  ADD_EXECUTABLE ( devas-compare-boundaries devas-compare-boundaries.c
	devas-image.c
	read-geometry.c
	dilate.c
	visualize-hazards.c
	devas-sRGB.c
	devas-png.c
	radiance-header.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	)
  TARGET_LINK_LIBRARIES ( devas-compare-boundaries
	${PNG_LIBRARIES}
	-lm
	)

endif ( )

ADD_EXECUTABLE ( geometry-boundaries geometry-boundaries.c
	devas-image.c
	read-geometry.c
	geometry-discontinuities.c
	directional-maxima.c
	devas-utils.c
	devas-png.c
	radiance-header.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	)
TARGET_LINK_LIBRARIES ( geometry-boundaries
	${PNG_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( luminance-boundaries luminance-boundaries.c
	devas-image.c
	devas-canny.c
	devas-gblur.c
	radianceIO.c
	radiance-header.c
	acuity-conversion.c
	radiance/badarg.c
	radiance/color.c
	radiance/fputword.c
	radiance/fvect.c
	radiance/header.c
	radiance/image.c
	radiance/resolu.c
	radiance/spec_rgb.c
	radiance/words.c
	radiance/timegm.c
	devas-sRGB.c
	devas-png.c
	)
TARGET_LINK_LIBRARIES ( luminance-boundaries
	${PNG_LIBRARIES}
	-lm
	)
