#ifndef __DEVA_GBLUR_FFT_H
#define __DEVA_GBLUR_FFT_H

#include "deva-image.h"

#define	STD_DEV_MIN	0.5		/* can't deal with standard */
					/* deviations smaller than this */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void		    DEVA_float_gblur2_fft ( DEVA_float_image *input,
			DEVA_float_image *output, float st_dev );
DEVA_float_image    *DEVA_float_gblur_fft ( DEVA_float_image *input,
			float st_dev );
void		    DEVA_gblur_fft_destroy ( void );


#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_GBLUR_FFT_H */
