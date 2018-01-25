#ifndef __DEVA_VIS_HAZ_H
#define __DEVA_VIS_HAZ_H

#include <stdlib.h>
#include <stdio.h>
#include "deva-image.h"
#include "deva-visibility.h"
#include "DEVA-sRGB.h"

#define	MAX_HAZARD	2.0	/* Degrees.  This is rather arbitrary! */

/*
 * Visualization types:
 */
#define	DEVA_VIS_HAZ_RED_ONLY	1	/* red => visibility hazard */
					/* gray => probably OK */
#define	DEVA_VIS_HAZ_RED_GREEN	2	/* red => visibility hazard */
					/* green => probably OK */

#define COLOR_MIN_RED_RG	0.1
#define COLOR_MIN_GREEN_RG	0.4
#define COLOR_MIN_BLUE_RG	0.1

#define COLOR_MIN_RED_RO	0.15
#define COLOR_MIN_GREEN_RO	0.15
#define COLOR_MIN_BLUE_RO	0.15

#define COLOR_MAX_RED           1.0
#define COLOR_MAX_GREEN         0.0
#define COLOR_MAX_BLUE          0.0

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_RGB_image	*visualize_hazards ( double max_hazard,
		    DEVA_float_image *hazards, int visualization_type );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_VIS_HAZ_H */
