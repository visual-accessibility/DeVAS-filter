#ifndef __DEVA_GBLUR_DEVA_H
#define __DEVA_GBLUR_DEVA_H

#include "deva-image.h"

#define	GBLUR_STD_DEV_MIN	0.5	/* can't deal with standard */
					/* deviations smaller than this */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void		    deva_float_gblur2 ( DEVA_float_image *input,
			DEVA_float_image *output, float st_dev );
DEVA_float_image    *deva_float_gblur ( DEVA_float_image *input, float st_dev );
void		    deva_gblur_destroy ( void );

/* expose this, since some application routines may care */
int		    DEVA_float_gblur_kernel_size ( float st_deviation );


#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_GBLUR_DEVA_H */
