#ifndef __DEVAUTILS_H
#define __DEVAUTILS_H

#include <stdlib.h>
#include <string.h>
#include "deva-image.h"
#include "deva-license.h"	/* DEVA open source license */

#ifndef TRUE
#define TRUE            1
#endif	/* TRUE */
#ifndef FALSE
#define FALSE           0
#endif	/* FALSE */

/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

char *		    strcat_safe ( char *dest, char *src );
DEVA_float_image    *DEVA_float_image_dup ( DEVA_float_image *original_image );
void		    DEVA_float_image_addto ( DEVA_float_image *i1,
			DEVA_float_image *i2 );
void		    DEVA_float_image_scalarmult ( DEVA_float_image *i,
			float m );
double		    degree2radian ( double degrees );
double		    radian2degree ( double radians );
int		    imax ( int x, int y );
int		    imin ( int x, int y );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVAUTILS_H */
