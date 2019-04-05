#ifndef	__DeVAS_VISABILITY_H
#define	__DeVAS_VISABILITY_H

#include <stdlib.h>
#include <math.h>
#include "devas-image.h"
#include "devas-filter.h"
#include "read-geometry.h"
#include "visualize-hazards.h"
#include "devas-license.h"       /* DeVAS open source license */

#define	CANNY_ST_DEV		M_SQRT2	/* may want to change this! */
#define	HAZARD_NO_EDGE		(-1.0)
#define	HAZARD_NO_EDGE_GRAY	(0)

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_float_image    *devas_visibility ( DeVAS_xyY_image *filtered_image,
			DeVAS_coordinates *coordinates,
			DeVAS_XYZ_image *xyz, DeVAS_float_image *dist,
			DeVAS_XYZ_image *nor,
			int position_patch_size, int orientation_patch_size,
			int position_threshold, int orientation_threshold,
			DeVAS_gray_image **luminance_boundaries,
			DeVAS_gray_image **geometry_boundaries,
			DeVAS_float_image **false_positives );

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_VISABILITY_H */
