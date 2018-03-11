#ifndef __DEVA_GBLUR_DEVA_H
#define __DEVA_GBLUR_DEVA_H

#include "deva-image.h"

#define	GBLUR_STD_DEV_MIN	0.5	/* can't deal with standard */
					/* deviations smaller than this */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_float_image    *deva_float_gblur ( DEVA_float_image *input, float st_dev );

/* expose this, since some application routines may care */
int		    DEVA_float_gblur_kernel_size ( float st_deviation );


#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_GBLUR_DEVA_H */
