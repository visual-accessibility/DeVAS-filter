#ifndef __DeVAS_GBLUR_FFT_H
#define __DeVAS_GBLUR_FFT_H

#include "devas-image.h"

#define	STD_DEV_MIN	0.5		/* can't deal with standard */
					/* deviations smaller than this */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void		    DeVAS_float_gblur2_fft ( DeVAS_float_image *input,
			DeVAS_float_image *output, float st_dev );
DeVAS_float_image    *DeVAS_float_gblur_fft ( DeVAS_float_image *input,
			float st_dev );
void		    DeVAS_gblur_fft_destroy ( void );


#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_GBLUR_FFT_H */
