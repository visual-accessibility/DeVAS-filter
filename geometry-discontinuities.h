#ifndef __DeVAS_GEOMETRY_DISCONTINUITIES_H
#define __DeVAS_GEOMETRY_DISCONTINUITIES_H

#include <stdlib.h>
#include "devas-image.h"
#include "read-geometry.h"
#include "devas-license.h"	/* DeVAS open source license */

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

DeVAS_gray_image *
    geometry_discontinuities ( DeVAS_coordinates *coordinates,
	    DeVAS_XYZ_image *xyz, DeVAS_float_image *dist, DeVAS_XYZ_image *nor,
	    int position_patch_size, int orientation_patch_size,
	    int position_threshold, int orientation_threshold );

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_GEOMETRY_DISCONTINUITIES_H */
