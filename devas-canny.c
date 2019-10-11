/*
 * Canny edge detector, with modifications as described in M. Fleck,
 * "Some defects in finite-difference edge finders," IEEE PAMI, 14(3),
 * March 1992.
 *
 * Supports auto-thresholding if requested.  (Note that auto-thresholding
 * is not totally "auto", since it requires setting one threshold for the
 * percentile of pixel values considered above threshold, and a second
 * parameter indicating how generous to be in hysteresis thresholding.)
 * Auto-thresholding is based on setting the high threshold at a value that
 * includes a chosen percentile of the gradient magnitude values at local
 * maxima.  Defining PERCENTILE_ALL sets the high threshold at a value that
 * includes a chosen percentile of all of the gradient magnitude values,
 * whether or not they are local maxima.
 *
 * Gradient-based edge detectors, not so surprisingly, are based on the
 * gradient of image intensity.  The gradient is a difference operator, and
 * is usually inappropriate when applied to luminance images.  The reason
 * for this is that the human visual system's sensitivity to luminance
 * differences decreases as the average luminance increases.  As a result,
 * most definitions of image contrast at a boundary use some for of intensity
 * ratios rather than intensity differences.  A gradient-based edge detector
 * can approximate this if it is applied to the logarithm of image intensity.
 * Definining CANNY_LOG_MAGNITUDE in devas-canny.h causes this formulation to
 * be used.  Note that this is not an issue when gradient-based edge detectors
 * are applied to images that are sRGB or gamma encoded, since the pixel
 * values for these images are approximately linear in perceived brightness.
 */

/* #define	DeVAS_CHECK_BOUNDS */		/* debugging aid */
/* #define	PRINT_THRESHOLD_COUNTS */	/* debugging aid */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "devas-canny.h"
#include "devas-gblur.h"
#include "devas-image.h"

#define	SIMPLE		1	/* Used to flag what sort of thresholding */
#define	HYSTERESIS	2	/* has been requested.  (The type is implicit */
#define	NONE		3	/* in the values of high_threshold and */
				/* low_threshold) */

/*
 * The following are used to compute extra thresold values as utilized in
 * Fleck's extension to Canny's method.
 *
 * T1 in Fleck is high_threshold in this routine.  Fleck does not have an
 * equivalent of low_threshold, since she does not consider hysteresis
 * thresholding.
 */
#define	T2		0.0
#define	T3		0.0

#define	EPSILON		0.0001	/* Used to avoid a boundary condition in */
				/* computing percentile bin numbers. */

#define	SQR(x)	((x)*(x))

#ifdef CANNY_LOG_MAGNITUDE
#define	CANNY_LOG_EPSILON	0.1	/* avoid log(0.0) */
#endif

#ifdef PRINT_THRESHOLD_COUNTS
static int	promotion_count = 0;	/* needs to be global */
#endif	/* PRINT_THRESHOLD_COUNTS */

/*
 *	Static function prototypes
 */

static DeVAS_gray_image	*canny_base ( DeVAS_float_image *input, double st_dev,
			    double high_threshold, double low_threshold,
			    DeVAS_float_image **magnitude_p,
			    DeVAS_float_image **orientation_p, int auto_thresh );
static DeVAS_float_image	*gradient_magnitude ( DeVAS_float_image *image,
			    DeVAS_float_image **grad_Y_p,
			    DeVAS_float_image **grad_X_p );
static void		auto_thresh_values ( DeVAS_float_image *magnitude,
			    double *high_threshold, double *low_threshold );
static DeVAS_gray_image	*find_edges ( DeVAS_float_image *magnitude,
			    DeVAS_float_image *grad_Y,
			    DeVAS_float_image *grad_X,
			    double high_threshold, double low_threshold );
static DeVAS_float_image	*find_orientation ( DeVAS_gray_image *edge_map,
			    DeVAS_float_image *grad_Y,
			    DeVAS_float_image *grad_X );
static void		hysteresis ( DeVAS_gray_image* edges );
static void		clean_up_magnitude ( DeVAS_gray_image *edge_map,
			    DeVAS_float_image *magnitude );
static void		follow ( DeVAS_gray_image *edges, int row, int col);
#ifndef PERCENTILE_ALL
static void		hysteresis_label ( DeVAS_gray_image *edge_map,
			    DeVAS_float_image *magnitude,
			    double high_threshold, double low_threshold );
#endif	/* PERCENTILE_ALL */
static double		std_angle ( double degrees );
static double		radian2degree ( double radians );

DeVAS_gray_image *
devas_canny ( DeVAS_float_image *input, double st_dev, double high_threshold,
	double low_threshold, DeVAS_float_image **magnitude_p,
	DeVAS_float_image **orientation_p )
/*
 * Canny edge detection with explicitly specified threshold values.
 */
{
    return ( canny_base ( input, st_dev, high_threshold, low_threshold,
		magnitude_p, orientation_p, FALSE ) );
}

DeVAS_gray_image *
devas_canny_autothresh ( DeVAS_float_image *input, double st_dev,
    DeVAS_float_image **magnitude_p, DeVAS_float_image **orientation_p )
/*
 * Canny edge detection with automatically determined threshold values.
 */
{
    return ( canny_base ( input, st_dev, 0.0, 0.0, magnitude_p, orientation_p,
		TRUE ) );
}

static DeVAS_gray_image *
canny_base ( DeVAS_float_image *input, double st_dev, double high_threshold,
    double low_threshold, DeVAS_float_image **magnitude_p,
    DeVAS_float_image **orientation_p, int auto_thresh )
/*
 * input:	    Luminance magnitude
 * st_dev:	    Standard deviation of initial Gaussian blurring.  If
 * 		    negative, no initial smoothing is done.  If blurring is
 * 		    requested, value must be >= GBLUR_STD_DEV_MIN (0.5), since
 * 		    gblur can't handle smaller values.
 * high_threshold:  Primary threshold, applied to gradient magnitude.  No
 * 		    thresholding if value is <= 0.0.
 * low_threshold:   Secondary threshold, used for hysteresis thresholding.
 * 		    Ignored if value is <= 0.0.  Can't be larger than
 * 		    high_threshold.
 * magnitude_p:     Pointer to image object where gradient magnitude will
 * 		    be returned.  Ignored if NULL.
 * orientation_p:   Pointer to image object where gradient orientation will
 * 		    be returned.  Ignored if NULL.  Orientation is specified
 * 		    in degrees, with 0.0 corresponding to "left" and values
 * 		    increasing clockwise.  Orientation is set to
 * 		    CANNY_DIR_NO_EDGE if no edge corresponds to that pixel.
 * auto_thresh:	    Autothreshold if TRUE (uses a variant of the MATLAB
 * 		    percentile method).
 *
 * returned value:  8-bit uint image object, with pixel values equal to TRUE
 *		    for detected edges, FALSE elsewhere.
 */
{
    int			n_rows, n_cols;
    DeVAS_float_image	*blurred_input;	/* smoothed original */
    DeVAS_float_image	*magnitude;	/* gradient magnitude (A in Fleck) */
    DeVAS_float_image	*grad_Y, *grad_X;	/* X and Y in Fleck */
    DeVAS_gray_image	*edge_map;	/* detected edges */
#ifdef CANNY_LOG_MAGNITUDE
    int		      row, col;
#endif	/* CANNY_LOG_MAGNITUDE */

    n_rows = DeVAS_image_n_rows ( input );
    n_cols = DeVAS_image_n_cols ( input );

    if ( ( n_rows < 5 ) || ( n_cols < 5 ) ) {
	fprintf ( stderr, "canny: input image too small!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( ( high_threshold <= 0.0 ) && ( low_threshold > 0.0 ) ) {
	fprintf ( stderr, "canny: low_threshold ignored (warning).\n" );
    } else if ( low_threshold > high_threshold ) {
	fprintf ( stderr,
		"canny: can't have low_threshold > high_threshold!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( st_dev >= GBLUR_STD_DEV_MIN ) {
	blurred_input = devas_float_gblur ( input, st_dev );
#ifdef CANNY_LOG_MAGNITUDE
	for ( row = 0; row < n_rows; row++ ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( blurred_input, row, col ) =
		    log ( DeVAS_image_data ( blurred_input, row, col ) +
			    CANNY_LOG_EPSILON );
	    }
	}
#endif	/* CANNY_LOG_MAGNITUDE */
    } else if ( st_dev <= 0 ) {
	/* don't blur */
#ifdef CANNY_LOG_MAGNITUDE
	blurred_input = DeVAS_float_image_new ( n_rows, n_cols );
	for ( row = 0; row < n_rows; row++ ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( blurred_input, row, col ) =
		    log ( DeVAS_image_data ( input, row, col ) +
			    CANNY_LOG_EPSILON );
	    }
	}
#else
	blurred_input = input;
#endif	/* CANNY_LOG_MAGNITUDE */
    } else {
	fprintf ( stderr,
		"canny: can't handle small standard devastions (%f)!\n",
			st_dev );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    magnitude = gradient_magnitude ( blurred_input, &grad_Y, &grad_X );
    	/* Compute gradient magnitude, along with separate Y and X components */
    	/* of gradient as used in Fleck implementation of Canny algorithm */

#ifdef PERCENTILE_ALL
    /*
     * Consider gradient magnitude of *all* pixels when choosing auto
     * threshold values.
     */

    if ( auto_thresh ) {
	/*
	 * Ignore threshold values provided as arguments and estimate
	 * thresholds using MATLAB percentile method.
	 */
	auto_thresh_values ( magnitude, &high_threshold, &low_threshold );
	    /* Determing threshold values *before* finding directional */
	    /* local maxima of luminance gradient. */
    }

    edge_map = find_edges ( magnitude, grad_Y, grad_X, high_threshold,
	    low_threshold );
    		/* Find local maximuma and threshold in one step. */

#else
    /*
     * Consider gradient magnitude of *only* of directional local maxima
     * when choosing auto threshold values.
     */

    if ( auto_thresh ) {
	edge_map = find_edges ( magnitude, grad_Y, grad_X, 0.0, 0.0 );
	    /* Find *all* directional local maxima (no thresholding). */

	clean_up_magnitude ( edge_map, magnitude );	/* set magnitude of */
							/* non-edges to 0.0 */
	auto_thresh_values ( magnitude, &high_threshold, &low_threshold );
	    /*
	     * Compute thresholds values based only on directional local maxima.
	     */

	hysteresis_label ( edge_map, magnitude, high_threshold, low_threshold );
	    /* set up for hysteresis process */
	hysteresis ( edge_map );	/* actual hysteresis thresholding */
    } else {
	edge_map = find_edges ( magnitude, grad_Y, grad_X, high_threshold,
		low_threshold );
			/*
			 * Canny edge detection with explicitly specified
			 * threshold values.
			 */
    }

#endif	/* PERCENTILE_ALL */

#ifdef PRINT_THRESHOLD_COUNTS
    if ( ( high_threshold > 0.0) && (low_threshold > 0.0) ) {
	printf ( "promotion count = %d\n", promotion_count );
    }
#endif	/* PRINT_THRESHOLD_COUNTS */

    if ( orientation_p != NULL ) {
	*orientation_p = find_orientation ( edge_map, grad_Y, grad_X );
		/* only computer gradient orientation if value is requested */
    }

    if ( magnitude_p != NULL ) {
	*magnitude_p = magnitude;
    } else {
	DeVAS_float_image_delete ( magnitude );
    }

    /* clean up */

    DeVAS_float_image_delete ( grad_Y );
    DeVAS_float_image_delete ( grad_X );
#ifdef CANNY_LOG_MAGNITUDE
    if ( ( st_dev >= GBLUR_STD_DEV_MIN ) || ( st_dev <= 0.0 ) ) {
	DeVAS_float_image_delete ( blurred_input );
    }
#else
    if ( st_dev >= GBLUR_STD_DEV_MIN ) {
	DeVAS_float_image_delete ( blurred_input );
    }
#endif

    return ( edge_map );
}

static DeVAS_float_image *
gradient_magnitude ( DeVAS_float_image *image, DeVAS_float_image **grad_Y_p,
	DeVAS_float_image **grad_X_p )
/*
 *	Compute gradient magnitude.  Save row and col differences
 *	for later use.
 */
{
    int		    n_rows, n_cols;
    int		    row, col;
    double	    V;		/* vertical difference (+ is down) */
    double	    H;		/* horizontal difference (+ is right) */
    double	    D_1;	/* diagonal difference (+ is down-right */
    double	    D_2;	/* diagonal difference (+ is up-right */
    double	    X, Y;	/* x and y components of gradient */
    DeVAS_float_image  *magnitude;	/* gradient magnitude */
    DeVAS_float_image  *grad_Y;	/* Y component of gradient */
    DeVAS_float_image  *grad_X;	/* X component of gradient */

    n_rows = DeVAS_image_n_rows ( image );
    n_cols = DeVAS_image_n_cols ( image );

    magnitude = DeVAS_float_image_new ( n_rows, n_cols );
    grad_Y = DeVAS_float_image_new ( n_rows, n_cols );
    grad_X = DeVAS_float_image_new ( n_rows, n_cols );

    /* set values near edge to zero */
    for ( row = 0; row < n_rows; row++ ) {
	DeVAS_image_data ( magnitude, row, 0 ) = 0.0;
	DeVAS_image_data ( magnitude, row, n_cols - 1 ) = 0.0;

	DeVAS_image_data ( grad_Y, row, 0 ) = 0.0;
	DeVAS_image_data ( grad_Y, row, n_cols - 1 ) = 0.0;

	DeVAS_image_data ( grad_X, row, 0 ) = 0.0;
	DeVAS_image_data ( grad_X, row, n_cols - 1 ) = 0.0;
    }
    for ( col = 1; col < n_cols - 1; col++ ) {
	DeVAS_image_data ( magnitude, 0, col ) = 0.0;
	DeVAS_image_data ( magnitude, n_rows - 1, col ) = 0.0;

	DeVAS_image_data ( grad_Y, 0, col ) = 0.0;
	DeVAS_image_data ( grad_Y, n_rows - 1, col ) = 0.0;

	DeVAS_image_data ( grad_X, 0, col ) = 0.0;
	DeVAS_image_data ( grad_X, n_rows - 1, col ) = 0.0;
    }

    /* Fleck-style gradient computation.  See paper for details. */

    for ( row = 1; row < n_rows - 1; row++ ) {
	for ( col = 1; col < n_cols - 1; col++ ) {
	    V = ( DeVAS_image_data ( image, row + 1, col ) -
		    DeVAS_image_data ( image, row - 1, col ) );
	    H = ( DeVAS_image_data ( image, row, col + 1 ) -
		    DeVAS_image_data ( image, row, col - 1 ) );

	    D_1 = DeVAS_image_data ( image, row + 1, col + 1) -
		DeVAS_image_data ( image, row - 1, col - 1);
	    D_2 = DeVAS_image_data ( image, row - 1, col + 1) -
		DeVAS_image_data ( image, row + 1,col - 1 );

	    X = H + ( 0.5 * ( D_1 + D_2 ) );
	    Y = V + ( 0.5 * ( D_1 - D_2 ) );

	    DeVAS_image_data ( magnitude, row, col ) =
		( sqrt ( SQR ( X ) + SQR ( Y ) ) );
	    DeVAS_image_data ( grad_Y, row, col ) = Y;
	    DeVAS_image_data ( grad_X, row, col) = X;
	}
    }

    /* return values */
    if ( grad_Y_p != NULL ) {
	*grad_Y_p = grad_Y;
    } else {
	fprintf ( stderr, "canny: gradient_magnitude argument error!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( grad_X_p != NULL ) {
	*grad_X_p = grad_X;
    } else {
	fprintf ( stderr, "canny: gradient_magnitude argument error!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    return ( magnitude );
}

static void
auto_thresh_values ( DeVAS_float_image *magnitude, double *high_threshold,
	double *low_threshold )
/*    
 * Auto-thresholding is based on setting the high threshold at a value that
 * includes a chosen percentile of the gradient magnitude values at local
 * maxima.  Defining PERCENTILE_ALL sets the high threshold at a value that
 * includes a chosen percentile of all of the gradient magnitude values,
 * whether or not they are local maxima.
 *
 * Percentile cutoffs are computed using a histogram method.  A large number
 * of bins are used due to the high dynamic range involved and the nature
 * of the frequency distibution of gradient magnitudes.
 */
{
    unsigned int    magnitude_hist[MAGNITUDE_HIST_NBINS];
    int	    	    row, col;
    int	    	    n_rows, n_cols;
    int	    	    bin;
    int	    	    bin_index;
    double  	    magnitude_max;
    double  	    norm;
    double  	    percentile;
    unsigned int    total_count;

    n_rows = DeVAS_image_n_rows ( magnitude );
    n_cols = DeVAS_image_n_cols ( magnitude );

    for ( bin = 0; bin < MAGNITUDE_HIST_NBINS; bin++ ) {
	magnitude_hist[bin] = 0;
    }

    /* Compute maximum value of gradient, ignoring pixels at edge of image. */
    magnitude_max = -1.0;
    for ( row = 1; row < n_rows - 1; row++ ) {
	for ( col = 1; col < n_cols - 1; col++ ) {
	    if ( DeVAS_image_data ( magnitude, row, col ) < 0.0 ) {
		fprintf ( stderr,
			"canny_autothresh: gradient magnitude < 0.0!\n" );
		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
		exit ( EXIT_FAILURE );
	    }

	    if ( DeVAS_image_data ( magnitude, row, col ) > magnitude_max ) {
		magnitude_max = DeVAS_image_data ( magnitude, row, col );
	    }
	}
    }
    
    if ( magnitude_max < 0.0 ) {
	/* no contrast! */
	*high_threshold = *low_threshold = -1.0;
	return;
    }

    /* Spread out gradient magnitude values amoung histogram bins. */
    if ( magnitude_max > 0.0 ) {
	norm = 1.0 / magnitude_max;
	for ( row = 1; row < n_rows - 1; row++ ) {
	    for ( col = 1; col < n_cols - 1; col++ ) {
		bin_index = norm * DeVAS_image_data ( magnitude, row, col ) *
		    ( ( (double) MAGNITUDE_HIST_NBINS ) - EPSILON );
		magnitude_hist[bin_index]++;
	    }
	}
    } else {
	magnitude_hist[0]++;
    }

    percentile = 0.0;
#ifdef PERCENTILE_ALL
    total_count = ( n_rows - 1 ) * ( n_cols - 1 );
#else
    total_count = ( ( n_rows - 1 ) * ( n_cols - 1 ) ) - magnitude_hist[0];
    				/* remove non-local-maxima from count */
#endif	/* PERCENTILE_ALL */

    /* Search for bin index corresponding to requested percentile. */
    for ( bin = MAGNITUDE_HIST_NBINS - 1; bin >= 0; bin-- ) {
	percentile += ((double) magnitude_hist[bin] ) / ((double) total_count );

	if ( percentile > PERCENTILE_EDGE_PIXELS ) {
	    break;
	}
    }

    *high_threshold = ( ((double) bin ) + 0.5 ) /
	    ( ((double) MAGNITUDE_HIST_NBINS ) - EPSILON ) * magnitude_max;
    *low_threshold = *high_threshold * LOW_THRESHOLD_MULTIPLE;
}

static DeVAS_gray_image *
find_edges ( DeVAS_float_image *magnitude, DeVAS_float_image *grad_Y,
	DeVAS_float_image *grad_X, double high_threshold, double low_threshold )
/*
 * Find directional local maxima of gradient magnitude.  See Fleck paper
 * for the details.
 *
 * Does *not* compute values for the pixels at the image boundary or one in
 * from the image boundary!
 */
{
    int	    n_rows, n_cols;
    int	    row, col;
    double  center_magnitude;
    double  B, S;			/* larger/smaller of X and Y */
    double  A_hv_plus, A_hv_minus;	/* horizontal or vertical neighbors */
    double  A_d_plus, A_d_minus;	/* diagonal neighbors */
    double  A_g_plus, A_g_minus;
    double  A_2g_plus, A_2g_minus;
    double  grad_Y_v, grad_X_v;
    double  grad_Y_v_abs, grad_X_v_abs;
    int	    threshold_type;
    double  coordinate;
    double  coord_low_f, coord_high_f;
    int	    coord_low, coord_high;
    DeVAS_gray_image   *edge_map; 

#ifdef PRINT_THRESHOLD_COUNTS
    int	    T1_passed_count, T2_passed_count, T3_passed_count;
    T1_passed_count = T2_passed_count = T3_passed_count = 0;
#endif	/* PRINT_THRESHOLD_COUNTS */

    n_rows = DeVAS_image_n_rows ( magnitude );
    n_cols = DeVAS_image_n_cols ( magnitude );

    edge_map = DeVAS_gray_image_new ( n_rows, n_cols );
    /* set values near edge to zero */
    for ( row = 0; row < n_rows; row++ ) {
	DeVAS_image_data ( edge_map, row, 0 ) = CANNY_NO_EDGE;
	DeVAS_image_data ( edge_map, row, 1 ) = CANNY_NO_EDGE;
	DeVAS_image_data ( edge_map, row, n_cols - 2 ) = CANNY_NO_EDGE;
	DeVAS_image_data ( edge_map, row, n_cols - 1 ) = CANNY_NO_EDGE;
    }
    for ( col = 2; col < n_cols - 2; col++ ) {
	DeVAS_image_data ( edge_map, 0, col ) = CANNY_NO_EDGE;
	DeVAS_image_data ( edge_map, 1, col ) = CANNY_NO_EDGE;
	DeVAS_image_data ( edge_map, n_rows - 2, col ) = CANNY_NO_EDGE;
	DeVAS_image_data ( edge_map, n_rows - 1, col ) = CANNY_NO_EDGE;
    }

    if ( ( high_threshold > 0.0) && ( low_threshold <= 0.0 ) ) {
	threshold_type = SIMPLE;
    } else if ( ( high_threshold > 0.0) && (low_threshold > 0.0) ) {
	threshold_type = HYSTERESIS;
    } else {
	threshold_type = NONE;
    }

    for ( row = 2; row < n_rows - 2; row++ ) {
	for ( col = 2; col < n_cols - 2; col++ ) {

	    center_magnitude = DeVAS_image_data ( magnitude, row, col );

	    /*
	     * Compare gradient magnitude with thresholds to
	     * see if any more elaborate analysis is needed.
	     * Need to figure out if we have no thresholding,
	     * simple thresholding, or hysteresis thresholding.
	     * This first check does not screen for directional
	     * local maxima.
	     */

	    switch (threshold_type) {
		case SIMPLE:
		    if ( center_magnitude <= high_threshold ) {
			DeVAS_image_data ( edge_map, row, col) = CANNY_NO_EDGE;
			continue;
		    } else {
			DeVAS_image_data ( edge_map, row, col) =
			    CANNY_MARKED_EDGE;
		    }
		    break;

		case HYSTERESIS:
		    if ( center_magnitude <= low_threshold) {
			/* likely not an edge */
			DeVAS_image_data ( edge_map, row, col ) = CANNY_NO_EDGE;
			continue;
		    } else if ( center_magnitude > high_threshold) {
			/* pretty sure it's real */
			DeVAS_image_data ( edge_map, row, col) =
			    CANNY_CERTAIN_EDGE;
		    } else {
			/* not really all that certain! */
			DeVAS_image_data ( edge_map, row, col) =
			    CANNY_POSSIBLE_EDGE;
		    }
		    break;

		case NONE:
		    if ( center_magnitude <= 0.0) {
			/* At least insist on there being */
			/* SOME gradient! */
			DeVAS_image_data ( edge_map, row, col) = CANNY_NO_EDGE;
			continue;
		    } else {
			DeVAS_image_data ( edge_map, row, col) =
			    CANNY_MARKED_EDGE;
			/* assume it's good until proved */
			/* otherwise */
		    }
		    break;
	    }

	    /*
	     * We get here only if the DeVAS_image_data is above high_threshold
	     * if simple thresholding, low_threshold if hysteresis thresholding,
	     * or 0.0 if no thresholding.
	     */

#ifdef PRINT_THRESHOLD_COUNTS
	    T1_passed_count++;
#endif	/* PRINT_THRESHOLD_COUNTS */

	    /*
	     *  Check to see if gradient magnitude is a
	     *  directional local maxima.  Rather than just
	     *  looking at a fixed set of orientations,  the
	     *  neighboring gradient magnitude values in
	     *  the gradient direction are computed using
	     *  linear interpolation.  Note that this is
	     *  a bit tedious.
	     */

	    /* Find interpolated gradient magnitude values. */

	    grad_Y_v = DeVAS_image_data ( grad_Y, row, col );
	    grad_Y_v_abs = fabs (grad_Y_v );
	    grad_X_v = DeVAS_image_data ( grad_X, row, col );
	    grad_X_v_abs = fabs (grad_X_v );

	    if ( grad_Y_v_abs > grad_X_v_abs ) {
		/* vertically oriented gradient (horizontally oriented edge) */
		B = grad_Y_v_abs;
		S = grad_X_v_abs;

		if ( grad_Y_v > 0.0 ) {
		    /* gradient points down */
		    A_hv_plus = DeVAS_image_data ( magnitude, row + 1, col );
		    A_hv_minus = DeVAS_image_data ( magnitude, row - 1, col );
		    if ( grad_X_v > 0.0 ) {
			/* gradient points (slighly) right */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row + 1, col + 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row - 1, col - 1 );
		    } else {
			/* gradient points (slighly) left */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row + 1, col - 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row - 1, col + 1 );
		    }
		} else {
		    /* gradient points up */
		    A_hv_plus = DeVAS_image_data ( magnitude, row - 1, col );
		    A_hv_minus = DeVAS_image_data ( magnitude, row + 1, col );
		    if ( grad_X_v > 0.0 ) {
			/* gradient points (slighly) right */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row - 1, col + 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row + 1, col - 1 );
		    } else {
			/* gradient points (slighly) left */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row - 1, col - 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row + 1, col + 1 );
		    }
		}
	    } else {
		/* horizontally oriented gradient (vertically oriented edge) */
		B = grad_X_v_abs;
		S = grad_Y_v_abs;

		if ( grad_X_v > 0.0 ) {
		    /* gradient points right */
		    A_hv_plus = DeVAS_image_data ( magnitude, row, col + 1 );
		    A_hv_minus = DeVAS_image_data ( magnitude, row, col - 1 );
		    if ( grad_Y_v > 0.0 ) {
			/* gradient points (slighly) down */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row + 1, col + 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row - 1, col - 1 );
		    } else {
			/* gradient points (slighly) up */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row - 1, col + 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row + 1, col - 1 );
		    }
		} else {
		    /* gradient points left */
		    A_hv_plus = DeVAS_image_data ( magnitude, row, col - 1 );
		    A_hv_minus = DeVAS_image_data ( magnitude, row, col + 1 );
		    if ( grad_Y_v > 0.0 ) {
			/* gradient points (slighly) down */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row + 1, col - 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row - 1, col + 1 );
		    } else {
			/* gradient points (slighly) up */
			A_d_plus =
			    DeVAS_image_data ( magnitude, row - 1, col - 1 );
			A_d_minus =
			    DeVAS_image_data ( magnitude, row + 1, col + 1 );
		    }
		}
	    }

	    A_g_plus = ( ( ( B - S ) * A_hv_plus ) +
		    ( S * A_d_plus ) ) / B;
	    A_g_minus = ( ( ( B - S ) * A_hv_minus ) +
		    ( S * A_d_minus ) ) / B;

	    /* See if current value is *not* a local maxima */
	    /* in Fleck, check is *for* half-maxima and tests are and-ed */
	    /* use asymmetric test to break ties when T2 = 0 */
	    if ( ( ( A_g_plus - center_magnitude ) > T2) ||
		    ( ( A_g_minus - center_magnitude ) >= T2 ) ) {
		DeVAS_image_data ( edge_map, row, col) = CANNY_NO_EDGE;
		continue;
	    }

	    /*
	     * We get here only if the DeVAS_image_data is above high_threshold
	     * if simple thresholding, low_threshold if hysteresis thresholding,
	     * or 0.0 if no thresholding, and if distance 1 pseudo-cell check
	     * is also passed.
	     */

#ifdef PRINT_THRESHOLD_COUNTS
	    T2_passed_count++;
#endif	/* PRINT_THRESHOLD_COUNTS */

	    /*
	     * Additional local maxima check 2 pixels out .
	     *
	     * Use similar triangles to find real-numbered
	     * coorinate of DeVAS_image_data distance 2 away.  Use this
	     * to do the linear interpolation.
	     */

	    if ( grad_Y_v_abs > grad_X_v_abs ) {
		/* Vertically oriented gradient (horizontally oriented edge) */

		/* get x coordinate */
		coordinate =  2.0 * ( grad_X_v_abs / grad_Y_v_abs );
					/* always positive */

		coord_low_f = floor ( coordinate );
		coord_low = (int) coord_low_f;
		coord_high_f = coord_low_f + 1.0;
		coord_high = (int) coord_high_f;

		if ( ( grad_X_v * grad_Y_v ) > 0.0 ) {
		    /* down and right or up and left */

		    A_2g_plus = ( ( coordinate - coord_low_f ) *
			    DeVAS_image_data ( magnitude, row + 2,
				col + coord_low ) ) +
			( ( coord_high_f - coordinate ) *
			  DeVAS_image_data ( magnitude, row + 2,
			      col + coord_high) );

		    A_2g_minus = ( ( coord_high_f - coordinate) *
			    DeVAS_image_data ( magnitude, row - 2,
				col - coord_high) ) +
			( (coordinate - coord_low_f ) *
			  DeVAS_image_data ( magnitude, row - 2,
			      col - coord_low ) );
		} else {
		    /* down and left or up and right */

		    A_2g_plus = ( ( coordinate - coord_low_f ) *
			    DeVAS_image_data ( magnitude, row - 2,
				col + coord_low ) ) +
			( ( coord_high_f - coordinate ) *
			  DeVAS_image_data ( magnitude, row - 2,
			      col + coord_high) );

		    A_2g_minus = ( ( coord_high_f - coordinate) *
			    DeVAS_image_data ( magnitude, row + 2,
				col - coord_high) ) +
			( (coordinate - coord_low_f ) *
			  DeVAS_image_data ( magnitude, row + 2,
			      col - coord_low ) );
		}
	    } else if ( grad_Y_v_abs < grad_X_v_abs ) {
		/* Horizontally oriented gradient (vertically oriented edge) */

		/* get y coordinate */
		coordinate =  2.0 * ( grad_Y_v_abs / grad_X_v_abs );
					/* always positive */

		coord_low_f = floor ( coordinate );
		coord_low = (int) coord_low_f;
		coord_high_f = coord_low_f + 1.0;
		coord_high = (int) coord_high_f;

		if ( ( grad_X_v * grad_Y_v ) > 0.0 ) {
		    /* right and down or left and up */
		    A_2g_plus = (( coordinate - coord_low_f) *
			    DeVAS_image_data ( magnitude, row + coord_low,
				col + 2 ) ) +
			( (coord_high_f - coordinate ) *
			  DeVAS_image_data ( magnitude, row + coord_high,
			      col + 2 ) );

		    A_2g_minus = ( ( coord_high_f - coordinate ) *
			    DeVAS_image_data ( magnitude, row - coord_high,
				col - 2)) +
			( (coordinate - coord_low_f ) *
			  DeVAS_image_data ( magnitude, row - coord_low,
			      col - 2) );
		} else {
		    /* right and up or left and down */
		    A_2g_plus = (( coordinate - coord_low_f) *
			    DeVAS_image_data ( magnitude, row + coord_low,
				col - 2 ) ) +
			( (coord_high_f - coordinate ) *
			  DeVAS_image_data ( magnitude, row + coord_high,
			      col - 2 ) );

		    A_2g_minus = ( ( coord_high_f - coordinate ) *
			    DeVAS_image_data ( magnitude, row - coord_high,
				col + 2)) +
			( (coordinate - coord_low_f ) *
			  DeVAS_image_data ( magnitude, row - coord_low,
			      col + 2) );
		}
	    } else {
		/* exact diagonal */

		/* check if signs are the same */
		if ( ( grad_Y_v * grad_X_v ) >= 0.0 ) {
		    /* down-and-right diagonal */
		    A_2g_plus = DeVAS_image_data ( magnitude, row + 2, col + 2 );
		    A_2g_minus =
			DeVAS_image_data ( magnitude, row - 2, col - 2 );
		} else {
		    /* down-and-left diagonal */
		    A_2g_plus = DeVAS_image_data ( magnitude, row + 2, col - 2 );
		    A_2g_minus =
			DeVAS_image_data ( magnitude, row - 2, col + 2 );
		}
	    }

	    if ( ( ( center_magnitude - A_2g_plus ) <= T3) ||
		    ( (center_magnitude - A_2g_minus ) <= T3) ) {
		DeVAS_image_data ( edge_map, row, col) = CANNY_NO_EDGE;
		continue;
	    }

	    /*
	     * We get here only if the DeVAS_image_data is above low_threshold,
	     * distance 1 pseudo-cell check is passed, and distance 2
	     * pseudo-cell check is passed.
	     */

	    /*
	     * We get here only if the DeVAS_image_data is above high_threshold
	     * if simple thresholding, low_threshold if hysteresis thresholding,
	     * or 0.0 if no thresholding, if distance 1 pseudo-cell check
	     * is passed, and if distance 2 pseudo-cell check is also passed.
	     */

#ifdef PRINT_THRESHOLD_COUNTS
	    T3_passed_count++;
#endif	/* PRINT_THRESHOLD_COUNTS */
	}
    }

#ifdef PRINT_THRESHOLD_COUNTS
    printf ( "threshold passed counts: %d, %d, %d\n", T1_passed_count,
	    T2_passed_count, T3_passed_count );
#endif	/* PRINT_THRESHOLD_COUNTS */

    if ( threshold_type == HYSTERESIS ) {
	hysteresis ( edge_map );
    }

    clean_up_magnitude ( edge_map, magnitude );

    return ( edge_map );
}

static DeVAS_float_image *
find_orientation ( DeVAS_gray_image *edge_map, DeVAS_float_image *grad_Y,
	DeVAS_float_image *grad_X )
/*
 * Compute gradient orientation at boundary elements based on gradient in Y
 * and X directions.  Orientation is specified in degrees, with 0.0
 * corresponding to "left" and values increasing clockwise.  Orientation is
 * set to CANNY_DIR_NO_EDGE if no edge corresponds to that pixel.
 */
{
    int		    row, col;
    int		    n_rows, n_cols;
    double	    grad_Y_v, grad_X_v;
    DeVAS_float_image  *orientation;

    n_rows = DeVAS_image_n_rows ( edge_map );
    n_cols = DeVAS_image_n_cols ( edge_map );

    orientation = DeVAS_float_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    {
		if ( DeVAS_image_data ( edge_map, row, col ) == CANNY_NO_EDGE) {
		    DeVAS_image_data ( orientation, row, col) =
							CANNY_DIR_NO_EDGE;
		} else {
		    grad_Y_v = DeVAS_image_data ( grad_Y, row, col );
		    grad_X_v = DeVAS_image_data ( grad_X, row, col );
		    DeVAS_image_data ( orientation, row, col ) =
			std_angle ( radian2degree (
				    atan2 ( grad_Y_v, grad_X_v ) ) );
		}
	    }
	}
    }

    return ( orientation );
}

static void
clean_up_magnitude ( DeVAS_gray_image *edge_map,  DeVAS_float_image *magnitude )
/*
 * Clear out magnitude values not corresponding to an edge pixel.
 */
{
    int	    n_rows, n_cols;
    int	    row, col;

    n_rows = DeVAS_image_n_rows ( edge_map );
    n_cols = DeVAS_image_n_cols ( edge_map );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( DeVAS_image_data ( edge_map, row, col ) == CANNY_NO_EDGE)
		DeVAS_image_data ( magnitude, row, col ) = CANNY_MAG_NO_EDGE;
	}
    }
}

static void
hysteresis ( DeVAS_gray_image* edges )
/*
 * Implements the second part of a hysteresis thresholding algorithm.  Edges
 * in an image object containing pixels one of three values as specified in
 * remaining arguments to the function.  No check is made to see if other
 * values are present, and any such values will remain unchanged.  Edges must
 * have a margin of at least one on all sides.
 *
 * The algorithm marks as edges all 8-connected sequences of pixels for which
 * all values are either CANNY_CERTAIN_EDGE or CANNY_POSSIBLE_EDGE and at
 * least one pixel has the value of CANNY_CERTAIN_EDGE.  The pixels in each
 * such sequence are set to the value CANNY_MARKED_EDGE.  Any remaining pixels
 * with an original value of CANNY_POSSIBLE_EDGE are set to no_edge.  Any
 * pixels originally with values other than CANNY_CERTAIN_EDGE or
 * CANNY_POSSIBLE_EDGE are left unchanged.  The values of CANNY_MARKED_EDGE,
 * CANNY_CERTAIN_EDGE, and CANNY_POSSIBLE_EDGE MUST be distinct.
 *
 * Normally, a tri-level thresholding operation will preceed execution of this
 * routine.
 */
{
    int	    n_rows,n_cols;
    int	    row, col;

    n_rows = DeVAS_image_n_rows ( edges );
    n_cols = DeVAS_image_n_cols ( edges );

    for( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    /* Find a "high" edge. */
	    if ( DeVAS_image_data ( edges,row,col) != CANNY_CERTAIN_EDGE)
		continue;

	    DeVAS_image_data ( edges,row,col ) = CANNY_MARKED_EDGE;
	    follow ( edges, row, col );
	}
    }

    /* Clean up anything remaining "low" edges. */
    for( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( DeVAS_image_data ( edges, row, col ) == CANNY_POSSIBLE_EDGE) {
		DeVAS_image_data ( edges, row, col) = CANNY_NO_EDGE;
	    }
	}
    }
}

static void
follow ( DeVAS_gray_image *edges, int row, int col)
/*
 *	Check 8-connected neighbors to see if they are edges.
 */
{
    int	    new_row, new_samp;

    /*
     * By avoiding edge pixels, we avoid trying to follow outside of the
     * image.  This should never happen because earlier code avoids pixels
     * two in from the edge.
     */
    for ( new_row = row - 1; new_row <= row + 1; new_row++ ) {
	for ( new_samp = col - 1; new_samp <= col + 1; new_samp++ ) {
	    if ( ( DeVAS_image_data ( edges,new_row,new_samp) ==
			CANNY_CERTAIN_EDGE) ||
		    ( DeVAS_image_data ( edges, new_row, new_samp) ==
		      CANNY_POSSIBLE_EDGE ) ) {
#ifdef PRINT_THRESHOLD_COUNTS
		if ( DeVAS_image_data ( edges, new_row, new_samp) ==
			                      CANNY_POSSIBLE_EDGE ) {
		    promotion_count++;
		}
#endif	/* PRINT_THRESHOLD_COUNTS */
		DeVAS_image_data ( edges, new_row, new_samp) = CANNY_MARKED_EDGE;
		follow ( edges, new_row, new_samp );
	    }
	}
    }
}

#ifndef PERCENTILE_ALL
static void
hysteresis_label ( DeVAS_gray_image *edge_map, DeVAS_float_image *magnitude,
	double high_threshold, double low_threshold )
/*
 * Used when setting potential edge codes is done in a separate step from
 * thresholding.
 */
{
    int     row, col;

    for ( row = 2; row < DeVAS_image_n_rows ( edge_map ) - 2; row++ ) {
	for ( col = 2; col < DeVAS_image_n_cols ( edge_map ) - 2; col++ ) {
	    if ( DeVAS_image_data ( magnitude, row, col) <= low_threshold ) {
		/* likely not an * edge */
		DeVAS_image_data ( edge_map, row, col ) = CANNY_NO_EDGE;
	    } else if ( DeVAS_image_data ( magnitude, row, col ) >
		    high_threshold ) {
		/* pretty sure it's real */
		DeVAS_image_data ( edge_map, row, col) = CANNY_CERTAIN_EDGE;
	    } else { /* not really all that certain either way...  */
		DeVAS_image_data ( edge_map, row, col) = CANNY_POSSIBLE_EDGE;
	    }
	}
    }
}
#endif  /* PERCENTILE_ALL */

static double
std_angle ( double degrees )
/*
 * Maps angle in degrees to the range [0.0 - 360.0).
 */
{
    degrees = fmod ( degrees, 360.0 );

    if ( ( degrees <= -360.0 ) || ( degrees == -0.0 ) ) {
	degrees = 0.0;
    } else if (degrees < 0.0) {
	degrees += 360.0;
    }

    return(degrees);
}

static double
radian2degree ( double radians )
/*
 * Converts radian value to degrees.
 */
{
    return ( radians * ( 360.0 / ( 2.0 * M_PI ) ) );
}
