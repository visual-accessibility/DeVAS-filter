#ifndef	__FIND_DIRECTIONAL_MAX_H
#define	__FIND_DIRECTIONAL_MAX_H

#include <stdlib.h>
#include "deva-image.h"
#include "deva-license.h"       /* DEVA open source license */

/* Search over four directions unless EIGHT_CONNECTED is defined */
/* #define EIGHT_CONNECTED */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_gray_image	    *find_directional_maxima ( int patch_size, double threshold,
		        DEVA_float_image *values );
DEVA_float_image    *gblur_3x3 ( DEVA_float_image *values );
DEVA_XYZ_image	    *gblur_3x3_3d ( DEVA_XYZ_image *values );

#ifdef __cplusplus
}
#endif

#endif	/* __FIND_DIRECTIONAL_MAX_H */
