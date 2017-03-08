cmake_minimum_required(VERSION 2.8)
PROJECT ( DEVA_FILTER )

# SET ( CMAKE_VERBOSE_MAKEFILE ON )

SET ( CMAKE_C_FLAGS "-Wall -g -O2" )

INCLUDE ( ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FindFFTW.cmake )

INCLUDE_DIRECTORIES (
	${FFTW_INCLUDE_DIR}
	)

ADD_DEFINITIONS ( -DDEVA_USE_FFTW3_ALLOCATORS )

ADD_EXECUTABLE ( deva-filter deva-filter-commandline.c
	deva-filter.c
	KLT-filter.c
	ChungLeggeCSF.c
	dilate.c
	deva-image.c
	deva-utils.c
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
TARGET_LINK_LIBRARIES ( deva-filter
	${FFTW_LIBRARIES}
	-lm
	)
