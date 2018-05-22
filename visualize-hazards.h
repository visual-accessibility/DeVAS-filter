#ifndef __DEVA_VIS_HAZ_H
#define __DEVA_VIS_HAZ_H

#include <stdlib.h>
#include <stdio.h>
#include "deva-image.h"
#include "deva-visibility.h"
#include "DEVA-sRGB.h"

/*
 * Measurement types:
 */
typedef	enum {
    unknown_measure,
    reciprocal_measure,	/* 1 - ( scale / ( visual_angle + scale ) ) */
    linear_measure,	/* min(visual_angle,max_hazard ) / max_hazard */
    Gaussian_measure	/* 1 - exp ( -0.5 * ( x / sigma_hazard ) ^ 2 ) */
} Measurement_type;

/*
 * Visualization types:
 */
typedef	enum {
    unknown_vis_type,
    red_gray_type,	/* redish => hazard, grayish => probably OK */
    red_green_type	/* redish => hazard, greenish => probably OK */
} Visualization_type;


/* minimal hazard for red_green_type */
#define COLOR_MIN_RED_RG	0.1
#define COLOR_MIN_GREEN_RG	0.4
#define COLOR_MIN_BLUE_RG	0.1

/* minimal hazard for red_gray_type */
#define COLOR_MIN_RED_RO	0.15
#define COLOR_MIN_GREEN_RO	0.15
#define COLOR_MIN_BLUE_RO	0.15

/* maximum hazard for both types */
#define COLOR_MAX_RED           1.0
#define COLOR_MAX_GREEN         0.0
#define COLOR_MAX_BLUE          0.0

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_RGB_image	*visualize_hazards ( DEVA_float_image *hazards,
       		    Measurement_type measurement_type, double scale_parameter,
		    Visualization_type visualization_type );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_VIS_HAZ_H */
