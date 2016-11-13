#ifndef __DEVA_DILATE_H
#define __DEVA_DILATE_H

#include <stdlib.h>
#include <stdio.h>
#include "deva-image.h"
#include "deva-license.h"	/* DEVA open source license */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_gray_image	*DEVA_gray_dilate ( DEVA_gray_image *input, double radius );
void		DEVA_gray_dilate2 ( DEVA_gray_image *input,
		    DEVA_gray_image *output, double radius );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_DILATE_H */

