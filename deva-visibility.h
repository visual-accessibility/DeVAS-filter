#ifndef	__DEVA_VISABILITY_H
#define	__DEVA_VISABILITY_H

#include <stdlib.h>
#include <math.h>
#include "deva-image.h"
#include "deva-filter.h"
#include "read-geometry.h"
#include "visualize-hazards.h"
#include "deva-license.h"       /* DEVA open source license */

#define	CANNY_ST_DEV		M_SQRT2	/* may want to change this! */
#define	HAZARD_NO_EDGE		(-1.0)
#define	HAZARD_NO_EDGE_GRAY	(0)

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_float_image    *deva_visibility ( DEVA_xyY_image *filtered_image,
			DEVA_coordinates *coordinates,
			DEVA_XYZ_image *xyz, DEVA_float_image *dist,
			DEVA_XYZ_image *nor,
			int position_patch_size, int orientation_patch_size,
			int position_threshold, int orientation_threshold,
			DEVA_gray_image **luminance_boundaries,
			DEVA_gray_image **geometry_boundaries,
			DEVA_float_image **false_positives );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_VISABILITY_H */
