/*
 * Implements a simulation of low vision acuity and contrast sensitivity
 * by using the quotient of the low vision CSF and normal vision CSF as
 * an MTF for filtering.
 *
 * - Uses CSF parameterized to account for degredations in acuity and contrast
 *   sensitivity.
 *
 * - Uses an improved threholding method that reduces the banding artifacts
 *   that otherwise occur, particularly when low vision is being simulated.
 *
 * Arguments:
 *
 *   input_image:	    In-memory, xyY values.  Also contains information
 *  			    about field of view.
 *
 *   acuity:		    ratio of desired peak contrast sensitivity frequency
 *  			    to nominal normal vision peak contrast sensitivity
 *  			    frequency.
 *
 *   contrast_sensitivity:  Ratio of desired peak contrast sensitivity to
 *  			    nominal normal vision peak contrast sensitivity.
 *
 *   saturation:	    Controls color saturation of output.
 *   			      1.0 ==> no attenuation of saturation
 *   			      0.0 ==> luminance only
 *   			      intermediate values in the range (0.0-1.0)
 *   			      	result in partial attenuation of saturation
 *
 * Returned value:
 *
 *   In-memory xyY values filtered to remove image structuer predicted
 *   to be below the detectability threshold.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <fftw3.h>
#include "KLT-filter.h"
#include "deva-image.h"
#include "deva-utils.h"
#include "ChungLeggeCSF.h"
#include "dilate.h"
#include "deva-filter-version.h"
#include "deva-license.h"       /* DEVA open source license */

#include "radianceIO.h"

/*
 * Misc. defines private to these routines:
 */

#define	MIN_AVERAGE_LUMINANCE	0.01	/* avoid divide by 0 in normalization */

#define	SMOOTH_INTERVAL_RATIO	0.25	/* ratio of peak band wavelength */
					/* to thresholded contrast smothing */
					/* radius */
#define	SMOOTH_MAXIMUM_RADIUS	512.0	/* with Felzenszwalb and Huttenlocher */
					/* (2012) distance transform, there */
					/* is no extra cost to large */
					/* smoothing radii */

#define	LOG2R_0		-10.0	/* Marker value for value of log2r(0). */
				/* Can't happen in practice, since r is pixel */
				/* distance and so never less than 1.0 except */
				/* at DC.  Needs to be < 1.0 for log2r_min to */
				/* work for band 0. */
/*
 * Used in clip_to_xyY_gamut ( ):
 */

typedef struct {
    double  x;
    double  y;
} XY_point;

/*
 * Global variables exposed to other routines:
 */

/***************************
int  DEVA_verbose = FALSE;
int  DEVA_veryverbose = FALSE;
 ***************************/
extern int  DEVA_verbose;
extern int  DEVA_veryverbose;

/*
 * Local functions:
 */

static DEVA_float_image	*CSF_weight_prep ( int n_rows_full, int n_cols_full,
				double fov, double acuity,
				double contrast_sensitivity );
static DEVA_float_image *filter_channel ( DEVA_float_image *channel,
				DEVA_float_image *CSF_weights );
static DEVA_complexf	rxc ( DEVA_float real_value,
			    DEVA_complexf complex_value );
static void		disassemble_input ( DEVA_xyY_image *input_image,
			    DEVA_float_image **luminance,
			    DEVA_float_image **x, DEVA_float_image **y );
static DEVA_xyY_image	*assemble_output ( DEVA_float_image *filtered_luminance,
			    DEVA_float_image *filtered_x,
			    DEVA_float_image *filtered_y,
			    double saturation );
static void		desaturate ( double saturation, DEVA_float_image *x,
			   DEVA_float_image *y );
static DEVA_xyY		clip_to_xyY_gamut ( DEVA_xyY xyY );
static XY_point		line_intersection ( XY_point line_1_p1,
			    XY_point line_1_p2, XY_point line_2_p1,
			    XY_point line_2_p2 );
static void		cleanup ( DEVA_float_image *luminance,
			    DEVA_float_image *x,
			    DEVA_float_image *y,
			    DEVA_float_image *filtered_luminance,
			    double saturation,
			    DEVA_float_image *filtered_x,
			    DEVA_float_image *filtered_y );

DEVA_xyY_image *
KLT_filter ( DEVA_xyY_image *input_image, double acuity,
	double contrast_sensitivity, double saturation )
/*
 * input_image:		 image to be filtered
 * acuity:		 decimal Snellen acuity to be simulated
 * contrast_sensitivity: contrast sensitivity adustment (1.0 => no adjustment)
 * smoothing_flag:	 TRUE => smooth thresholded contrast bands
 * saturation:		 control saturation of output
 */
{
    DEVA_float_image	*luminance;	/* Y channel of input */
    DEVA_float_image	*x;		/* chromaticity channels of input */
    DEVA_float_image	*y;
    double		fov;		/* along largest dimension (degrees) */
    DEVA_float_image	*CSF_weights;	/* (re)used for filtering channels */
    DEVA_float_image	*filtered_luminance;	/* filtered luminance channel */
    DEVA_float_image	*filtered_x = NULL;	/* filtered x chromaticity */
    DEVA_float_image	*filtered_y = NULL;	/* filtered x chromaticity */
    DEVA_xyY_image	*filtered_image;	/* full xyY output image */

    ChungLeggeCSF_set_peak_sensitivity ( KLTCSF_MAX_SENSITIVITY );
    ChungLeggeCSF_set_peak_frequency ( KLTCSF_PEAK_FREQUENCY );

    disassemble_input ( input_image, &luminance, &x, &y );
    /* break input into separate luminance and chromaticity channels */
    /* allocates luminance, x, and y images */

    /*
     * One-time jobs:
     */

    fov = fmax ( DEVA_image_view ( luminance ) . vert,
	   			 DEVA_image_view ( luminance ) . horiz );
    if ( fov <= 0.0 ) {
	fprintf ( stderr, "deva_filter: invalid or missing fov (%f, %f)!\n",
		DEVA_image_view ( luminance ) . vert,
			DEVA_image_view ( luminance ) . horiz );
	exit ( EXIT_FAILURE );
    }
    if ( DEVA_verbose ) {
	fprintf ( stderr, "FOV = %.1f degrees\n", fov );
	ChungLeggeCSF_print_stats ( acuity, contrast_sensitivity );
    }

    CSF_weights = CSF_weight_prep ( DEVA_image_n_rows ( luminance ),
	    DEVA_image_n_cols (luminance ), fov, acuity, contrast_sensitivity );

    filtered_luminance = filter_channel ( luminance, CSF_weights );

    if ( saturation > 0.0 ) {
	filtered_x = filter_channel ( x, CSF_weights );
	filtered_y = filter_channel ( y, CSF_weights );
	desaturate ( saturation, filtered_x, filtered_y );

    }
    DEVA_float_image_delete ( CSF_weights );

    filtered_image = assemble_output ( filtered_luminance, filtered_x,
	    filtered_y, saturation );
    /* reassemble separate luminance and chromaticity channels into */
    /* single output image */

    DEVA_image_view ( filtered_image ) = DEVA_image_view ( input_image );
					/* nothing's changed in the view */

    cleanup ( luminance, x, y, filtered_luminance, saturation, filtered_x,
	    filtered_y );

    return ( filtered_image );
}

void
KLT_filter_print_version ( void )
{
    fprintf ( stderr, "KLT_filter version %s\n", DEVAFILTER_VERSION_STRING );
}

static DEVA_complexf_image *
forward_transform ( DEVA_float_image *source )
{
    unsigned int	n_rows_input, n_cols_input;
    unsigned int	n_rows_transform, n_cols_transform;
    DEVA_complexf_image	*transformed_image;
    fftwf_plan		fft_forward_plan;

    n_rows_input = DEVA_image_n_rows ( source );
    n_cols_input = DEVA_image_n_cols ( source );

    n_rows_transform = n_rows_input;
    n_cols_transform = ( n_cols_input / 2 ) + 1;

    transformed_image =
	DEVA_complexf_image_new ( n_rows_transform, n_cols_transform );

    fft_forward_plan = fftwf_plan_dft_r2c_2d ( n_rows_input, n_cols_input,
	    &DEVA_image_data ( source, 0, 0 ),
	    (fftwf_complex *) &DEVA_image_data ( transformed_image, 0, 0 ),
	    FFTW_ESTIMATE );
    	/* FFTW3 documentation says to always create plan before initializing */
        /* input array, but then goes on to say that "technically" this is */
        /* not required for FFTW_ESTIMATE */

    fftwf_execute ( fft_forward_plan );

    fftwf_destroy_plan ( fft_forward_plan );

    return ( transformed_image );
}

static DEVA_float_image *
CSF_weight_prep ( int n_rows_full, int n_cols_full,
	double fov,
	double acuity, double contrast_sensitivity )
/*
 * Precompute CSF-based filter weights.
 * Suppress low frequency rolloff in CSF.
 * Normalize CSF to have a peak value of 1.0.
 *
 * These values can be reused for each color, thus saving repetitions
 * of square, square root, and CSF computations.
 *
 * frequency_space:	Used only to pass size of transformed image.
 */
{
    DEVA_float_image	*CSF_weights;
    int			row, col;
    unsigned int	n_rows_transform, n_cols_transform;
    double		row_dist, col_dist;
    double		CSF_normal;
    double		CSF_low;
    double		frequency_angle;

    n_rows_transform = n_rows_full;
    n_cols_transform = ( n_cols_full / 2 ) + 1;

    CSF_weights = DEVA_float_image_new ( n_rows_transform, n_cols_transform );

    /*
     * DC at [0][0], full resolution in row dimension (requires
     * offset), half resolution in col dimension (use as-is).
     */

    DEVA_image_data ( CSF_weights, 0, 0 ) = 1.0;

    for ( col = 1; col < n_cols_transform; col++ ) {
	frequency_angle = ((double) col ) / fov;

	CSF_low = ChungLeggeCSF ( frequency_angle, acuity,
		contrast_sensitivity );
	CSF_normal = ChungLeggeCSF ( frequency_angle, 1.0, 1.0 );
	if ( CSF_low < CSF_normal ) {
	    DEVA_image_data ( CSF_weights, 0, col ) = CSF_low / CSF_normal;
	} else {
	    DEVA_image_data ( CSF_weights, 0, col ) = 1.0;
	}
    }

    for ( row = 1; row < ( n_rows_transform + 1 ) / 2; row++ ) {
	for ( col = 0; col < n_cols_transform; col++ ) {
	    row_dist = (double) row;
	    col_dist = (double) col;
	    frequency_angle = sqrt ( ( row_dist * row_dist ) +
		    ( col_dist * col_dist ) ) / fov;

	    CSF_low = ChungLeggeCSF ( frequency_angle, acuity,
		    contrast_sensitivity );
	    CSF_normal = ChungLeggeCSF ( frequency_angle, 1.0, 1.0 );
	    if ( CSF_low < CSF_normal ) {
		DEVA_image_data ( CSF_weights, row, col ) =
		    CSF_low / CSF_normal;
	    } else {
		DEVA_image_data ( CSF_weights, row, col ) = 1.0;
	    }
	}
    }

    for ( row = ( n_rows_transform + 1 ) / 2; row < n_rows_transform; row++ ) {
	for ( col = 0; col < n_cols_transform; col++ ) {
	    row_dist = (double) ( n_rows_transform - row );
	    col_dist = (double) col;
	    frequency_angle = sqrt ( ( row_dist * row_dist ) +
		    ( col_dist * col_dist ) ) / fov;

	    CSF_low = ChungLeggeCSF ( frequency_angle, acuity,
		    contrast_sensitivity );
	    CSF_normal = ChungLeggeCSF ( frequency_angle, 1.0, 1.0 );
	    if ( CSF_low < CSF_normal ) {
		DEVA_image_data ( CSF_weights, row, col ) =
		    CSF_low / CSF_normal;
	    } else {
		DEVA_image_data ( CSF_weights, row, col ) = 1.0;
	    }
	}
    }

    return ( CSF_weights );
}

static DEVA_float_image *
filter_channel ( DEVA_float_image *channel, DEVA_float_image *CSF_weights )
/*
 * Filter channel using CSF as if it were an MTF.
 */
{
    DEVA_float_image	*filtered_channel;
    fftwf_plan		fft_inverse_plan;
    DEVA_complexf_image	*frequency_space;
    double		norm;
    int			row, col;

    frequency_space = forward_transform ( channel );

    for ( row = 0; row < DEVA_image_n_rows ( CSF_weights ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( CSF_weights ); col++ ) {
	    DEVA_image_data ( frequency_space, row, col ) =
		rxc ( DEVA_image_data ( CSF_weights, row, col ),
			DEVA_image_data ( frequency_space, row, col ) );
	}
    }

    filtered_channel = DEVA_float_image_new ( DEVA_image_n_rows ( channel ),
	    DEVA_image_n_cols ( channel ) );

    fft_inverse_plan =
	fftwf_plan_dft_c2r_2d ( DEVA_image_n_rows ( channel ),
		DEVA_image_n_cols ( channel ),
		(fftwf_complex *) &DEVA_image_data ( frequency_space, 0, 0 ),
		&DEVA_image_data ( filtered_channel, 0, 0 ), FFTW_ESTIMATE);
    fftwf_execute ( fft_inverse_plan );

    norm = 1.0 / (double) ( DEVA_image_n_rows ( filtered_channel ) *
	    DEVA_image_n_cols ( filtered_channel ) );

    for ( row = 0; row < DEVA_image_n_rows ( filtered_channel ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( filtered_channel ); col++ ) {
	    DEVA_image_data ( filtered_channel, row, col ) *= norm;
	}
    }

    fftwf_destroy_plan ( fft_inverse_plan );
    DEVA_complexf_image_delete ( frequency_space );

    return ( filtered_channel );
}

static DEVA_complexf
rxc ( DEVA_float real_value, DEVA_complexf complex_value )
    /*
     * Multiply a complex number by a real value.
     */
{
    DEVA_complexf value;

    value.real = real_value * complex_value.real;
    value.imaginary = real_value * complex_value.imaginary;

    return ( value );
}

static void
disassemble_input ( DEVA_xyY_image *input_image, DEVA_float_image **luminance,
	DEVA_float_image **x, DEVA_float_image **y )
/* break input into separate luminance and chromaticity channels */
{
    int	    row, col;

    *luminance = DEVA_float_image_new ( DEVA_image_n_rows ( input_image ),
	    DEVA_image_n_cols ( input_image ) );
    *x = DEVA_float_image_new ( DEVA_image_n_rows ( input_image ),
	    DEVA_image_n_cols ( input_image ) );
    *y = DEVA_float_image_new ( DEVA_image_n_rows ( input_image ),
	    DEVA_image_n_cols ( input_image ) );

    for ( row = 0; row < DEVA_image_n_rows ( input_image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( input_image ); col++ ) {
	    DEVA_image_data ( *luminance, row, col ) =
	    		DEVA_image_data ( input_image, row, col ).Y;
	    DEVA_image_data ( *x, row, col ) =
	    		DEVA_image_data ( input_image, row, col ) . x;
	    DEVA_image_data ( *y, row, col ) =
	    		DEVA_image_data ( input_image, row, col ) . y;
	}
    }

    DEVA_image_view ( *luminance ) . vert =
		DEVA_image_view ( input_image ) .vert;
    DEVA_image_view ( *luminance ) . horiz =
		DEVA_image_view ( input_image ) .horiz;

    if ( DEVA_image_description ( input_image ) != NULL ) {
	DEVA_image_description ( *luminance ) =
	    strdup ( DEVA_image_description ( input_image ) );
    } else {
	DEVA_image_description ( *luminance ) = NULL;
    }
}

static DEVA_xyY_image *
assemble_output ( DEVA_float_image *filtered_luminance,
	DEVA_float_image *filtered_x, DEVA_float_image *filtered_y,
	double saturation )
/* reassemble separate luminance and chromaticity channels into single */
/* output image */
{
    DEVA_xyY	    xyY;
    DEVA_xyY_image  *output_image;
    int		    row, col;

    output_image =
	DEVA_xyY_image_new ( DEVA_image_n_rows ( filtered_luminance ),
	    DEVA_image_n_cols ( filtered_luminance ) );

    if ( saturation > 0.0 )  {
	for ( row = 0; row < DEVA_image_n_rows ( output_image ); row++ ) {
	    for ( col = 0; col < DEVA_image_n_cols ( output_image ); col++ ) {
		xyY.x = DEVA_image_data ( filtered_x, row, col );
		xyY.y = DEVA_image_data ( filtered_y, row, col );
		xyY.Y = DEVA_image_data ( filtered_luminance, row, col );

		DEVA_image_data ( output_image, row, col ) =
		    clip_to_xyY_gamut ( xyY );
	    }
	} 
    } else {
	for ( row = 0; row < DEVA_image_n_rows ( output_image ); row++ ) {
	    for ( col = 0; col < DEVA_image_n_cols ( output_image ); col++ ) {
		xyY.x = DEVA_x_WHITEPOINT;
		xyY.y = DEVA_y_WHITEPOINT;
		xyY.Y = DEVA_image_data ( filtered_luminance, row, col );

		DEVA_image_data ( output_image, row, col ) =
		    clip_to_xyY_gamut ( xyY );
	    }
	}
    }

    return ( output_image );
}

static void
desaturate ( double saturation, DEVA_float_image *x, DEVA_float_image *y )
{
    int	    row, col;

    if ( !DEVA_float_image_samesize ( x, y ) ) {
	fprintf ( stderr, "desaturate_ingamut: incompatible inputs!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( saturation <= 0.0 ) {
	/* handled in assemble_output ( ) */
	return;
    }

    if ( saturation < 1.0 ) {
	for ( row = 0; row < DEVA_image_n_rows ( x ); row++ ) {
	    for ( col = 0; col < DEVA_image_n_cols ( x ); col++ ) {
		DEVA_image_data ( x, row, col ) =
		    ( saturation * DEVA_image_data ( x, row, col ) ) +
		    	( ( 1.0 - saturation ) * DEVA_x_WHITEPOINT );
		DEVA_image_data ( y, row, col ) =
		    ( saturation * DEVA_image_data ( y, row, col ) ) +
		    	( ( 1.0 - saturation ) * DEVA_y_WHITEPOINT );
	    }
	}
    }
}

static DEVA_xyY
clip_to_xyY_gamut ( DEVA_xyY xyY )
/*
 * If value is outside of gamut triangle, return the point on the gamut
 * triangle intersected by a line from the whitepoint to the value.
 */
{
    DEVA_xyY	ingamut_xyY;
    XY_point	xy_point, origin, x_max, y_max, white_pt;
    XY_point	intersection;

    ingamut_xyY.Y = fmax ( xyY.Y, 0.0 );	/* Y can't be negative */

    xy_point.x = xyY.x; xy_point.y = xyY.y;
    origin.x = 0.0; origin.y = 0.0;
    x_max.x = 1.0; x_max.y = 0.0;
    y_max.x = 0.0; y_max.y = 1.0;
    white_pt.x = DEVA_x_WHITEPOINT;
    white_pt.y = DEVA_y_WHITEPOINT;

    if ( xyY.y < 0.0 ) {
	/* wrong side of x-axis */
	intersection = line_intersection ( origin, x_max, white_pt, xy_point );
	if ( ( intersection.y >= 0.0 ) && intersection.y <= 1.0 ) {
	    ingamut_xyY.x = intersection.x;
	    ingamut_xyY.y = intersection.y;

	    return ( ingamut_xyY );
	}
    } else if ( xyY.x < 0.0 ) {
	/* wrong side of y-axis */
	intersection = line_intersection ( origin, y_max, white_pt, xy_point );
	if ( ( intersection.x >= 0.0 ) && intersection.x <= 1.0 ) {
	    ingamut_xyY.x = intersection.x;
	    ingamut_xyY.y = intersection.y;

	    return ( ingamut_xyY );
	}
    } else if ( ( xyY.x + xyY.y ) > 1.0 ) {
	/* wrong side of gamut hypotenuse */
	intersection = line_intersection ( x_max, y_max, white_pt, xy_point );
	if ( ( intersection.x <= 1.0 ) && intersection.y <= 1.0 ) {
	    ingamut_xyY.x = intersection.x;
	    ingamut_xyY.y = intersection.y;

	    return ( ingamut_xyY );
	}
    } else if ( ( xyY.x >= 0.0 ) && ( xyY.y >= 0.0 ) &&
	    ( ( xyY.x + xyY.y ) <= 1.0 ) ) {
	ingamut_xyY.x = xyY.x;
	ingamut_xyY.y = xyY.y;

	return ( ingamut_xyY );
    }

    fprintf ( stderr, "clip_to_xyY_gamut: internal error!\n" );
    exit ( EXIT_FAILURE );
}

#define	LINE_INTERSECTION_EPSILON	0.0001

static XY_point
line_intersection ( XY_point line_1_p1, XY_point line_1_p2, XY_point line_2_p1,
	XY_point line_2_p2 )
/*
 * Returns the intersection of two infinite lines, each specified in terms
 * of two points on the line.  It is a fatal error for the two points
 * specifying a particular line to be coincident or for the two lines to
 * be parallel or coincident.
 */
{
    double	denominator;
    XY_point	intersection;

    denominator = ( ( line_1_p1.x - line_1_p2.x ) *
	    				( line_2_p1.y - line_2_p2.y ) ) -
	( ( line_1_p1.y - line_1_p2.y ) * ( line_2_p1.x - line_2_p2.x ) );

    if ( fabs ( denominator ) < LINE_INTERSECTION_EPSILON ) {
	fprintf ( stderr,
	    "line_intersection: coincident or parallel lines or points!\n" );
	exit ( EXIT_FAILURE );
    }

    intersection.x =
	( ( ( ( line_1_p1.x * line_1_p2.y ) - ( line_1_p1.y * line_1_p2.x ) ) *
	    			( line_2_p1.x - line_2_p2.x ) ) -
	  ( ( line_1_p1.x - line_1_p2.x ) *
	    ( ( line_2_p1.x * line_2_p2.y ) -
	      			( line_2_p1.y * line_2_p2.x ) ) ) ) /
			denominator;

    intersection.y =
	( ( ( ( line_1_p1.x * line_1_p2.y ) - ( line_1_p1.y * line_1_p2.x ) ) *
	    			( line_2_p1.y - line_2_p2.y ) ) -
	  ( ( line_1_p1.y - line_1_p2.y ) *
	    ( ( line_2_p1.x * line_2_p2.y ) -
	      			( line_2_p1.y * line_2_p2.x ) ) ) ) /
			denominator;

    return ( intersection );
}

static void
cleanup (
    DEVA_float_image *luminance,
    DEVA_float_image *x,
    DEVA_float_image *y,
    DEVA_float_image *filtered_luminance,
    double saturation,	/* needed for filtered_x and filtered_y */
    DEVA_float_image *filtered_x,
    DEVA_float_image *filtered_y )
/*
 * de-leak memory
 */
{
    DEVA_float_image_delete ( luminance );
    DEVA_float_image_delete ( x );
    DEVA_float_image_delete ( y );
    DEVA_float_image_delete ( filtered_luminance );
    if ( saturation > 0.0 ) {
	DEVA_float_image_delete ( filtered_x );
	DEVA_float_image_delete ( filtered_y );
    }
    fftwf_cleanup ( );
}
