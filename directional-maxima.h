#ifndef	__FIND_DIRECTIONAL_MAX_H
#define	__FIND_DIRECTIONAL_MAX_H

#include <stdlib.h>
#include "devas-image.h"
#include "devas-license.h"       /* DeVAS open source license */

/* Search over four directions unless EIGHT_CONNECTED is defined */
/* #define EIGHT_CONNECTED */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_gray_image    *find_directional_maxima ( int patch_size, double threshold,
		        DeVAS_float_image *values );
DeVAS_float_image   *gblur_3x3 ( DeVAS_float_image *values );
DeVAS_XYZ_image	    *gblur_3x3_3d ( DeVAS_XYZ_image *values );

#ifdef __cplusplus
}
#endif

#endif	/* __FIND_DIRECTIONAL_MAX_H */
