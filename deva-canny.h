/*
 * Canny edge detector for float luminance images.
 */

#ifndef	__DEVA_CANNY_H
#define	__DEVA_CANNY_H

#include <stdlib.h>
#include <stdio.h>
#include "deva-image.h"
#include "deva-license.h"       /* DEVA open source license */

/*
 * Auto-thresholding is based on setting the high threshold at a value that
 * includes a chosen percentile of the gradient magnitude values at local
 * maxima.  Defining PERCENTILE_ALL sets the high threshold at a value that
 * includes a chosen percentile of all of the gradient magnitude values,
 * whether or not they are local maxima.
 */
#define	PERCENTILE_ALL

#define	PERCENTILE_EDGE_PIXELS	0.3	/* percentile of gradient magnitude */
					/* values considered to be likely */
					/* edges (see note about */
					/* PERCENTILE_ALL) */
#define	LOW_THRESHOLD_MULTIPLE	0.4	/* ratio of low to high threshold */
					/* (for auto-level hysteresis */
					/* thresholding) */

#define	MAGNITUDE_HIST_NBINS	1000	/* needs to be large enought to deal */
					/* with gradient magnitude histograms */
					/* having narrow peaks. */

/*
 * The following values are specified as defines rather than as enums,
 * since they have to be assignable to gray_image pixels.
 */
#define	CANNY_NO_EDGE		0
#define CANNY_MARKED_EDGE	1
#define CANNY_POSSIBLE_EDGE	2
#define CANNY_CERTAIN_EDGE	3

#define	CANNY_MAG_NO_EDGE       0.0
#define	CANNY_DIR_NO_EDGE       -1.0

#define	CANNY_STDDEV_MIN	0.5	/* can't Gaussian blur with standard */
					/* devation less than this */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_gray_image	*deva_canny ( DEVA_float_image *input, double st_dev,
		    double high_threshold, double low_threshold,
		    DEVA_float_image **magnitude_p,
		    DEVA_float_image **orientation_p );

DEVA_gray_image	*deva_canny_autothresh ( DEVA_float_image *input, double st_dev,
		    DEVA_float_image **magnitude_p,
		    DEVA_float_image **orientation_p );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_CANNY_H */
