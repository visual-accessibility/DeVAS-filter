/*
 * Canny edge detector for float luminance images.
 */

#ifndef	__DeVAS_CANNY_H
#define	__DeVAS_CANNY_H

#include <stdlib.h>
#include <stdio.h>
#include "devas-image.h"
#include "devas-license.h"       /* DeVAS open source license */

/*
 * Auto-thresholding is based on setting the high threshold at a value that
 * includes a chosen percentile of the gradient magnitude values at local
 * maxima.  Defining PERCENTILE_ALL sets the high threshold at a value that
 * includes a chosen percentile of all of the gradient magnitude values,
 * whether or not they are local maxima.
 */
/* #define	PERCENTILE_ALL */

/*
 * Gradient-based edge detectors, not so surprisingly, are based on the
 * gradient of image intensity.  The gradient is a difference operator, and is
 * usually inappropriate when applied to luminance images.  The reason for this
 * is that the human visual system's sensitivity to luminance differences
 * decreases as the average luminance increases.  As a result, most definitions
 * of image contrast at a boundary use some for of intensity ratios rather than
 * intensity differences.  A gradient-based edge detector can approximate this
 * if it is applied to the logarithm of image intensity.  Definining
 * CANNY_LOG_MAGNITUDE causes this formulation to be used.  Note that this is
 * not an issue when gradient-based edge detectors are applied to images that
 * are sRGB or gamma encoded, since the pixel values for these images are
 * approximately linear in perceived brightness.
 */
#define	CANNY_LOG_MAGNITUDE

/* old value was 0.3 */
#define	PERCENTILE_EDGE_PIXELS	0.4	/* percentile of gradient magnitude */
					/* values considered to be likely */
					/* edges (see note about */
					/* PERCENTILE_ALL) */

/* old value was 0.4 */
#define	LOW_THRESHOLD_MULTIPLE	0.6	/* ratio of low to high threshold */
					/* (for auto-level hysteresis */
					/* thresholding) */

#define	MAGNITUDE_HIST_NBINS	1000	/* needs to be large enought to deal */
					/* with gradient magnitude histograms */
					/* having narrow peaks. */

/*
 * The following values are specified as defines rather than as enums,
 * since they have to be assignable to gray_image pixels.
 */
#define	CANNY_NO_EDGE		0	/* definitely not an edge */
#define CANNY_MARKED_EDGE	1	/* above all relevant thresholds, */
					/* but may violate local directional */
					/* maxima constraint */
#define CANNY_POSSIBLE_EDGE	2	/* above some relevant thresholds, */
					/* but may violate local directional */
					/* maxima constraint */
#define CANNY_CERTAIN_EDGE	3	/* above all relevant thresholds, */
					/* but may violate local directional */
					/* maxima constraint (used for */
					/* hysteresis thresholding). */

#define	CANNY_MAG_NO_EDGE       0.0
#define	CANNY_DIR_NO_EDGE       -1.0

#define	CANNY_STDDEV_MIN	0.5	/* can't Gaussian blur with standard */
					/* devastion less than this */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_gray_image    *devas_canny ( DeVAS_float_image *input, double st_dev,
		        double high_threshold, double low_threshold,
		        DeVAS_float_image **magnitude_p,
		        DeVAS_float_image **orientation_p );

DeVAS_gray_image    *devas_canny_autothresh ( DeVAS_float_image *input,
			double st_dev,
		        DeVAS_float_image **magnitude_p,
		        DeVAS_float_image **orientation_p );

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_CANNY_H */
