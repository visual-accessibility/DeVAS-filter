#ifndef __DEVA_MARGIN_H
#define __DEVA_MARGIN_H

#include <stdlib.h>
#include "deva-image.h"
#include "deva-license.h"	/* DEVA open source license */

#define DEVA_MARGIN_REFLECT
			/* If defined, reflect portion of image */
			/* inside the boundary to the margin, */
			/* rather than copying nearest edge pixel. */

/* #define DEVA_MARGIN_AVERAGE_ALL */
			/* If defined, blend outer part of margin */
			/* to average of all pixels (DC value), */
			/* rather that average of border elements. */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_xyY_image	*DEVA_xyY_add_margin ( int v_margin, int h_margin,
		    DEVA_xyY_image *original );

DEVA_xyY_image	*DEVA_xyY_strip_margin ( int v_margin, int h_margin,
		    DEVA_xyY_image *with_margin );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_MARGIN_H */

