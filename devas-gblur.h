#ifndef __DeVAS_GBLUR_DeVAS_H
#define __DeVAS_GBLUR_DeVAS_H

#include "devas-image.h"

#define	GBLUR_STD_DEV_MIN	0.5	/* can't deal with standard */
					/* deviations smaller than this */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif
DeVAS_float_image   *devas_float_gblur ( DeVAS_float_image *input,
		        float st_dev );

/* expose this, since some application routines may care */
int		    DeVAS_float_gblur_kernel_size ( float st_deviation );


#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_GBLUR_DeVAS_H */
