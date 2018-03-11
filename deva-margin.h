#ifndef __DEVA_MARGIN_H
#define __DEVA_MARGIN_H

#include <stdlib.h>
#include "deva-image.h"
#include "deva-license.h"	/* DEVA open source license */

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

