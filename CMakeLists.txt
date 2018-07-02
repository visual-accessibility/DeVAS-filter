cmake_minimum_required(VERSION 3.0)
PROJECT ( DEVA_FILTER )

if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
# Linux, MacOS
option (DEVA_FILTER_USE_CAIRO "Build using Cairo library" ON )
else ( )
# Windows
option (DEVA_FILTER_USE_CAIRO "Build using Cairo library" OFF )
endif ( )

message ( STATUS "CMAKE_SYSTEM_NAME = " ${CMAKE_SYSTEM_NAME} )

# SET ( CMAKE_VERBOSE_MAKEFILE ON )

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
# Radiance tries to redefine things it shouldn't with mignw64
# cross-compiling
set ( CMAKE_C_FLAGS "-Wall -g -O3 -D_mingw64_cross" )
else ( )
set ( CMAKE_C_FLAGS "-Wall -g -O3" )
endif ( )

if ( CMAKE_SYSTEM_NAME STREQUAL "Windows" )
include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindPNG_Windows.cmake )
include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindFFTW_Windows.cmake )
else ( )
find_package ( PNG REQUIRED )
if ( DEVA_FILTER_USE_CAIRO )
include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindCairo.cmake )
endif ( )
include ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindFFTW.cmake )
endif ( )

if ( DEVA_FILTER_USE_CAIRO )
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

ADD_EXECUTABLE ( deva-filter deva-commandline.c
	deva-filter.c
	ChungLeggeCSF.c
	dilate.c
	deva-image.c
	deva-utils.c
	deva-margin.c
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
TARGET_COMPILE_DEFINITIONS ( deva-filter PRIVATE DEVA_USE_FFTW3_ALLOCATORS )
TARGET_LINK_LIBRARIES ( deva-filter
	${FFTW_LIBRARIES}
	-lm
	)

ADD_EXECUTABLE ( deva-visibility deva-commandline.c
	deva-filter.c
	ChungLeggeCSF.c
	deva-image.c
	deva-margin.c
	deva-utils.c
	dilate.c
	deva-canny.c
	deva-gblur.c
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
	deva-visibility.c
	geometry-discontinuities.c
	directional-maxima.c
	visualize-hazards.c
	DEVA-sRGB.c
	DEVA-png.c
	)
TARGET_COMPILE_DEFINITIONS ( deva-visibility PRIVATE DEVA_VISIBILITY )
TARGET_COMPILE_DEFINITIONS ( deva-visibility PRIVATE DEVA_USE_FFTW3_ALLOCATORS )
TARGET_LINK_LIBRARIES ( deva-visibility
	${FFTW_LIBRARIES}
	${PNG_LIBRARIES}
	-lm
	)

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

ADD_EXECUTABLE ( deva-visualize-geometry deva-visualize-geometry.c
	deva-image.c
	read-geometry.c
	DEVA-sRGB.c
	DEVA-png.c
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
TARGET_LINK_LIBRARIES ( deva-visualize-geometry
	${PNG_LIBRARIES}
	-lm
	)

if ( DEVA_FILTER_USE_CAIRO )

ADD_EXECUTABLE ( deva-compare-boundaries deva-compare-boundaries.c
	deva-image.c
	read-geometry.c
	dilate.c
	visualize-hazards.c
	deva-add-text.c
	DEVA-sRGB.c
	DEVA-png.c
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
TARGET_COMPILE_DEFINITIONS ( deva-compare-boundaries PRIVATE DEVA_USE_CAIRO )
TARGET_LINK_LIBRARIES ( deva-compare-boundaries
	${PNG_LIBRARIES}
	${CAIRO_LIBRARIES}
	-lm
	)

else ( )

ADD_EXECUTABLE ( deva-compare-boundaries deva-compare-boundaries.c
	deva-image.c
	read-geometry.c
	dilate.c
	visualize-hazards.c
	DEVA-sRGB.c
	DEVA-png.c
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
TARGET_LINK_LIBRARIES ( deva-compare-boundaries
	${PNG_LIBRARIES}
	-lm
	)

endif ( )

ADD_EXECUTABLE ( geometry-boundaries geometry-boundaries.c
	deva-image.c
	read-geometry.c
	geometry-discontinuities.c
	directional-maxima.c
	deva-utils.c
	DEVA-png.c
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
	deva-image.c
	deva-canny.c
	deva-gblur.c
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
	DEVA-sRGB.c
	DEVA-png.c
	)
TARGET_LINK_LIBRARIES ( luminance-boundaries
	${PNG_LIBRARIES}
	-lm
	)
