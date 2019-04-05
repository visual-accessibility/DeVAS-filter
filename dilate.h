#ifndef __DeVAS_DILATE_H
#define __DeVAS_DILATE_H

#include <stdlib.h>
#include <stdio.h>
#include "devas-image.h"
#include "devas-license.h"	/* DeVAS open source license */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_gray_image  *DeVAS_gray_dilate ( DeVAS_gray_image *input, double radius );
void		  DeVAS_gray_dilate_2 ( DeVAS_gray_image *input,
		        DeVAS_gray_image *output, double radius );
DeVAS_float_image *dt_euclid_sq ( DeVAS_gray_image *input );
void    	  dt_euclid_sq_2 ( DeVAS_gray_image *input,
			DeVAS_float_image *output );

#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_DILATE_H */

