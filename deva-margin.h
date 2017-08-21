#ifndef __DEVAMARGIN_H
#define __DEVAMARGIN_H

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
DEVA_gray_image	*DEVA_gray_strip_margin ( int v_margin, int h_margin,
		    DEVA_gray_image *with_margin );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVAMARGIN_H */
