#ifndef __DeVAS_MARGIN_H
#define __DeVAS_MARGIN_H

#include <stdlib.h>
#include "devas-image.h"
#include "devas-license.h"	/* DeVAS open source license */

#define DeVAS_MARGIN_REFLECT
			/* If defined, reflect portion of image */
			/* inside the boundary to the margin, */
			/* rather than copying nearest edge pixel. */

/* #define DeVAS_MARGIN_AVERAGE_ALL */
			/* If defined, blend outer part of margin */
			/* to average of all pixels (DC value), */
			/* rather that average of border elements. */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_float_image    *DeVAS_float_add_margin ( int v_margin, int h_margin,
			DeVAS_float_image *original );

DeVAS_float_image    *DeVAS_float_strip_margin ( int v_margin, int h_margin,
			DeVAS_float_image *with_margin );

DeVAS_xyY_image	    *DeVAS_xyY_add_margin ( int v_margin, int h_margin,
			DeVAS_xyY_image *original );

DeVAS_xyY_image	    *DeVAS_xyY_strip_margin ( int v_margin, int h_margin,
			DeVAS_xyY_image *with_margin );

#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_MARGIN_H */
