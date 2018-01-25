#ifndef __DEVA_GEOMETRY_DISCONTINUITIES_H
#define __DEVA_GEOMETRY_DISCONTINUITIES_H

#include <stdlib.h>
#include "deva-image.h"
#include "read-geometry.h"
#include "deva-license.h"	/* DEVA open source license */

#define DMAX_PATCH_SIZE		3	/* interval over which to evaluate */
					/* local maxima test */

/*
 * Set SMOOTH_ORIENTATION to apply slight smoothing to orientation values
 * before looking for orientation discontinuities.
 */
/* #define SMOOTH_ORIENTATION */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_gray_image *
    geometry_discontinuities ( DEVA_coordinates *coordinates,
	    DEVA_XYZ_image *xyz, DEVA_float_image *dist, DEVA_XYZ_image *nor,
	    int position_patch_size, int orientation_patch_size,
	    int position_threshold, int orientation_threshold );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_GEOMETRY_DISCONTINUITIES_H */
