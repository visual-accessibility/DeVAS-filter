#ifndef __DeVAS_UTILS_H
#define __DeVAS_UTILS_H

#include <stdlib.h>
#include <string.h>
#include "devas-image.h"
#include "devas-license.h"	/* DeVAS open source license */

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

char		   *strcat_safe ( char *dest, char *src );
DeVAS_float_image  *DeVAS_float_image_dup ( DeVAS_float_image *original_image );
void		   DeVAS_float_image_addto ( DeVAS_float_image *i1,
			DeVAS_float_image *i2 );
void		   DeVAS_float_image_scalarmult ( DeVAS_float_image *i,
			float m );
double		   degree2radian ( double degrees );
double		   radian2degree ( double radians );
int		   imax ( int x, int y );
int		    imin ( int x, int y );

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_UTILS_H */
