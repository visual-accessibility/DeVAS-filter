/*
 * Implements contrast thesholding filter described in Eli Peli, "Contrast in
 * complex images," Journal of the Optical Society of America A, 7(10),
 * 2032-2040, 1990, with the following additions:
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
 *   smoothing_flag:	    TRUE if thesholded contrast should be smoothed
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

// #define	DEVA_CHECK_BOUNDS

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <fftw3.h>
#include "deva-filter.h"
#include "deva-image.h"
#include "deva-utils.h"
#include "ChungLeggeCSF.h"
#include "dilate.h"
#include "deva-filter-version.h"
#include "deva-license.h"       /* DEVA open source license */

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

int  DEVA_verbose = FALSE;
int  DEVA_veryverbose = FALSE;

/*
 * Local functions:
 */

static void		preallocate_images ( int n_rows, int n_cols,
    			    DEVA_complexf_image **weighted_frequency_space,
    			    DEVA_float_image **contrast_band,
    			    DEVA_float_image **local_luminance,
    			    DEVA_float_image **thresholded_contrast_band,
    			    DEVA_gray_image **threshold_mask_initial_positive,
    			    DEVA_gray_image **threshold_mask_initial_negative,
    			    DEVA_gray_image **threshold_mask_positive,
    			    DEVA_gray_image **threshold_mask_negative,
			    DEVA_float_image **filtered_luminance );
static DEVA_complexf_image *forward_transform ( DEVA_float_image *source );
static DEVA_float_image	*log2r_prep ( DEVA_complexf_image *transformed_image );
static void		bandpass_filter ( int band,
			    DEVA_complexf_image *frequency_space,
			    DEVA_complexf_image *weighted_frequency_space,
			    DEVA_float_image *log2r,
			    DEVA_float_image *contrast_band,
			    fftwf_plan fft_inverse_plan );
static void		apply_threshold ( double sensitivity,
			    float peak_frequency_image,
			    DEVA_float_image *contrast_band,
			    DEVA_float_image *local_luminance,
			    DEVA_float_image *thresholded_contrast_band,
			    DEVA_gray_image *threshold_mask_initial_positive,
			    DEVA_gray_image *threshold_mask_initial_negative,
			    DEVA_gray_image *threshold_mask_positive,
			    DEVA_gray_image *threshold_mask_negative,
			    int smoothing_flag );
static DEVA_float_image	*CSF_weight_prep ( DEVA_complexf_image *frequency_space,
				double fov, double acuity,
				double contrast_sensitivity );
static DEVA_float_image *filter_color ( DEVA_float_image *chroma_channel,
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
static void		cleanup ( DEVA_complexf_image *frequency_space,
			    DEVA_float_image *log2r,
			    DEVA_complexf_image *weighted_frequency_space,
			    DEVA_float_image *contrast_band,
			    DEVA_float_image *local_luminance,
			    DEVA_float_image *thresholded_contrast_band,
			    DEVA_float_image *luminance,
			    DEVA_float_image *x,
			    DEVA_float_image *y,
			    DEVA_float_image *filtered_luminance,
			    DEVA_gray_image *threshold_mask_initial_positive,
			    DEVA_gray_image *threshold_mask_initial_negative,
			    DEVA_gray_image *threshold_mask_positive,
			    DEVA_gray_image *threshold_mask_negative,
			    double saturation,
			    DEVA_float_image *filtered_x,
			    DEVA_float_image *filtered_y,
			    fftwf_plan *fft_inverse_plan );

DEVA_xyY_image *
deva_filter ( DEVA_xyY_image *input_image, double acuity,
	double contrast_sensitivity, int smoothing_flag, double saturation )
/*
 * input_image:		 image to be filtered
 * acuity:		 decimal Snellen acuity to be simulated
 * contrast_sensitivity: contrast sensitivity adustment (1.0 => no adjustment)
 * smoothing_flag:	 TRUE => smooth thresholded contrast bands
 * saturation:		 control saturation of output
 */
{
    int			band;		/* band index */
    int			n_bands;	/* number of bands actually used */
    int     		n_bands_max;	/* maximum possible number of bands */
    int			n_lf_skipped;	/* # low frequency below thres bands */
    double  		fov;		/* along largest dimension (degrees) */
    double  		peak_frequency_image;	/* cycles/image */
    double  		peak_frequency_angle;	/* cycles/degree */
    double		peak_sensitivity;	/* 1/Michelson */
    DEVA_float_image	*luminance;	/* Y channel of input */
    DEVA_float_image	*x;		/* chromaticity channels of input */
    DEVA_float_image	*y;
    DEVA_complexf_image	*frequency_space;/* transformed image */
    float		DC;		/* DC of transfored image */
    DEVA_complexf_image	*weighted_frequency_space;  /* work area */
    DEVA_float_image	*log2r;		/* (re)used to creat bandpass weights */
    DEVA_float_image	*CSF_weights;	/* (re)used for filtering color */
    DEVA_float_image	*contrast_band;	/* result of bandpass filter */
    DEVA_float_image	*local_luminance;/* used to normalize local contrast */
    DEVA_float_image	*thresholded_contrast_band; /* result of thesholding */
    fftwf_plan		fft_inverse_plan;	/* to create bandpass images */
    DEVA_gray_image	*threshold_mask_initial_positive; /* threshold mask */
    DEVA_gray_image	*threshold_mask_initial_negative; /* threshold mask */
    DEVA_gray_image	*threshold_mask_positive;	 /* threshold mask */
    DEVA_gray_image	*threshold_mask_negative;	 /* threshold mask */
    DEVA_float_image	*filtered_luminance;	/* filtered luminance channel */
    DEVA_float_image	*filtered_x = NULL;	/* filtered x chromaticity */
    						/* not always used */
    DEVA_float_image	*filtered_y = NULL;	/* filtered x chromaticity */
    						/* not always used */
    DEVA_xyY_image	*filtered_image;	/* full xyY output image */

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

    /* preallocate image objects that will be reused for each processed band */
    preallocate_images ( DEVA_image_n_rows ( luminance ),
	        DEVA_image_n_cols ( luminance ),
		&weighted_frequency_space,
		&contrast_band,
		&local_luminance,
		&thresholded_contrast_band,
		&threshold_mask_initial_positive,
		&threshold_mask_initial_negative,
		&threshold_mask_positive,
		&threshold_mask_negative,
		&filtered_luminance );

    frequency_space = forward_transform ( luminance );	/* only done once */
    DC = DEVA_image_data ( frequency_space, 0, 0 ) . real /
	((double) ( DEVA_image_n_rows ( luminance ) *
	    DEVA_image_n_cols ( luminance ) ) );

    /*
     * Local_luminance and filtered_luminance are iteratively computed
     * across bands.  This sets the starting values.
     */
    DEVA_float_image_set_value ( local_luminance, DC );
    DEVA_float_image_set_value ( filtered_luminance, DC );

    /* get a bit of speed by reusing for every band */
    log2r = log2r_prep ( frequency_space );

    /* get a bit of speed by reusing for every band */
    fft_inverse_plan =
	fftwf_plan_dft_c2r_2d ( DEVA_image_n_rows ( luminance ),
	    	DEVA_image_n_cols ( luminance ),
		(fftwf_complex *)
		    &DEVA_image_data ( weighted_frequency_space, 0, 0 ),
		&DEVA_image_data ( contrast_band, 0, 0 ),
		FFTW_ESTIMATE);

    /*
     * Iterate through bands to compute filtered_luminance:
     */

    n_bands = 0;
    n_lf_skipped = 0;
    n_bands_max = (int)
	ceil ( log2 ((double) imax ( DEVA_image_n_rows ( luminance ),
			DEVA_image_n_cols ( luminance ) ) ) );
    		/* may miss (very) high frequencies on diagonal */

    if ( DEVA_veryverbose ) {
	fprintf ( stderr,
      "\nband peak_frequency_image peak_frequency_angle peak_sensitivity\n" );
    }

    for ( band = 0; band < n_bands_max; band++ ) {

	/* peak of cosine band in image coordinates */
	peak_frequency_image = pow ( 2.0, band );

	/* peak of cosine band in spatial frequency units */
	peak_frequency_angle = peak_frequency_image / fov;

	/* sensitivity at peak of cosine band */
	peak_sensitivity = ChungLeggeCSF ( peak_frequency_angle, acuity,
		contrast_sensitivity );

	if ( DEVA_veryverbose ) {
	    fprintf ( stderr,
		"%2d:       %9.2f,         %9.2f,         %9.2f\n",
		    band, peak_frequency_image, peak_frequency_angle,
		    peak_sensitivity );
	}

	/*
	 * End iterating over bands if/when sensitivity is < 1.0 for a
	 * frequency > peak sensitivity.
	 */
	if ( ( peak_frequency_angle > ChungLeggeCSF_peak_frequency ( acuity,
			contrast_sensitivity ) ) &&
		( peak_sensitivity < 1.0 ) ) {
	    if ( DEVA_veryverbose ) {
		fprintf ( stderr,
    "ending iterations: below threshold bands on high frequency side of CSF\n"
			);
	    }
	    break;
	}

	n_bands++;	/* on to the next band */

	if ( peak_sensitivity < 1.0 ) {
	    /* skip below threshold band on low frequency side of CSF */
	    if ( DEVA_veryverbose ) {
		fprintf ( stderr,
	"skipping below threshold band on low frequency side of CSF\n" );
	    }
	    n_lf_skipped++;
	    continue;
	} else {
	    /* computer the bandpass band */
	    bandpass_filter ( band,  frequency_space, weighted_frequency_space,
		    log2r, contrast_band, fft_inverse_plan );

	    /*
	     * treat bandpass band as local contrast and threshold based
	     * on CSF sensitivity
	     */
	    apply_threshold ( peak_sensitivity, peak_frequency_image,
		    contrast_band, local_luminance, thresholded_contrast_band,
		    threshold_mask_initial_positive,
		    threshold_mask_initial_negative,
		    threshold_mask_positive, threshold_mask_negative,
		    smoothing_flag );

	    /* add another level to the image pyramid */
	    DEVA_float_image_addto ( filtered_luminance,
		    thresholded_contrast_band );
	}

	DEVA_float_image_addto ( local_luminance, contrast_band );
		/* for use in next iteration */
	    	/* do this even when skipping below threshold band on */
		/* low frequency side of CSF */
    }

    if ( DEVA_veryverbose ) {
	fprintf ( stderr, "n_bands = %d, n_lf_skipped = %d\n", n_bands,
		n_lf_skipped );
    }

    if ( n_bands == n_lf_skipped ) {
	fprintf ( stderr, "deva-filter: no above threshold contrast!\n" );
    }

    if ( saturation > 0.0 ) {
	CSF_weights = CSF_weight_prep ( frequency_space, fov, acuity,
		contrast_sensitivity );

	filtered_x = filter_color ( x, CSF_weights );
	filtered_y = filter_color ( y, CSF_weights );
	desaturate ( saturation, filtered_x, filtered_y );

	DEVA_float_image_delete ( CSF_weights );
    }

    filtered_image = assemble_output ( filtered_luminance, filtered_x,
	    filtered_y, saturation );
	/* reassemble separate luminance and chromaticity channels into */
	/* single output image */

    DEVA_image_view ( filtered_image ) = DEVA_image_view ( input_image );
    					/* nothing's changed in the view */

    cleanup ( frequency_space,
		log2r,
		weighted_frequency_space,
		contrast_band,
		local_luminance,
		thresholded_contrast_band,
		luminance,
		x,
		y,
		filtered_luminance,
		threshold_mask_initial_positive,
		threshold_mask_initial_negative,
		threshold_mask_positive,
		threshold_mask_negative,
		saturation,
		filtered_x,
		filtered_y,
		&fft_inverse_plan );

    return ( filtered_image );
}

void
deva_filter_print_version ( void )
{
    fprintf ( stderr, "deva_filter version %s\n", DEVAFILTER_VERSION_STRING );
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
log2r_prep ( DEVA_complexf_image *transformed_image )
/*
 * Precompute log_2(r) in equation A2 of Peli (1990).
 *
 * These values can be reused for each band, thus saving repetitions
 * of square, square root, and log2 computations.
 *
 * transformed_image:	Used only to pass size of transformed image.
 */
{
    DEVA_float_image	*log2r;
    int			row, col;
    unsigned int	n_rows, n_cols;
    double		row_dist, col_dist;

    n_rows = DEVA_image_n_rows ( transformed_image );
    n_cols = DEVA_image_n_cols ( transformed_image );

    log2r = DEVA_float_image_new ( n_rows, n_cols );

    /*
     * DC at [0][0], full resolution in row dimension (requires
     * offset), half resolution in col dimension (use as-is).
     *
     * Need to special case first row to avoid log2(0.0).
     */
    DEVA_image_data (log2r, 0, 0 ) = LOG2R_0;
				/* make sure log2(0) is never used */
    				/* set this value to be < -1.0 to make range */
				/* of band 0 work in bandpass_filter */
    for ( col = 1; col < n_cols; col++ ) {
	DEVA_image_data ( log2r, 0, col ) = log2 ( col );
    }

    for ( row = 1; row < ( n_rows + 1 ) / 2; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) row;
	    col_dist = (double) col;

	    DEVA_image_data ( log2r, row, col ) =
		log2 ( sqrt ( ( row_dist * row_dist ) +
			    ( col_dist * col_dist ) ) );
	}
    }

    for ( row = ( n_rows + 1 ) / 2; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) ( n_rows - row );
	    col_dist = (double) col;

	    DEVA_image_data ( log2r, row, col ) =
		log2 ( sqrt ( ( row_dist * row_dist ) +
			    ( col_dist * col_dist ) ) );
	}
    }

    return ( log2r );
}

static void
bandpass_filter ( int band,  DEVA_complexf_image *frequency_space,
	DEVA_complexf_image *weighted_frequency_space,
	DEVA_float_image *log2r, DEVA_float_image *contrast_band,
	fftwf_plan fft_inverse_plan )
/*
 * Weight frequency space values using equation A2 in Peli (1990). Weights are
 * a shifted cosine over (-pi - pi), centered at the band frequency, with a
 * log2 scaling of the cosine angle.
 */
{
    int     row, col;
    float   log2r_min, log2r_max;
    float   log2r_value;
    float   filter_weight;
    float   norm;

    log2r_min = (float) ( band - 1 );
    log2r_max = (float) ( band + 1 );

    for ( row = 0; row < DEVA_image_n_rows ( frequency_space ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( frequency_space ); col++ ) {
	    log2r_value = DEVA_image_data ( log2r, row, col );
	    if ( ( log2r_value > log2r_min ) &&
		    ( log2r_value < log2r_max ) ) {
		filter_weight = 0.5 * ( 1.0 + cos ( M_PI * log2r_value -
				( band * M_PI ) ) );
	    } else {
		filter_weight = 0.0;
	    }

	    DEVA_image_data ( weighted_frequency_space, row, col ) =
		rxc ( filter_weight,
			DEVA_image_data ( frequency_space, row, col ) );
	}
    }

    fftwf_execute ( fft_inverse_plan );	/* specified in calling program: */
    					/* src: weighted_frequency_space */
    					/* dst: contrast_band */

    norm = 1.0 / (double) ( DEVA_image_n_rows ( contrast_band ) *
	    DEVA_image_n_cols ( contrast_band ) );

    for ( row = 0; row < DEVA_image_n_rows ( contrast_band ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( contrast_band ); col++ ) {
	    DEVA_image_data ( contrast_band, row, col ) *= norm;
	}
    }
}

static void
apply_threshold ( double sensitivity, float peak_frequency_image,
	DEVA_float_image *contrast_band, DEVA_float_image *local_luminance,
	DEVA_float_image *thresholded_contrast_band,
	DEVA_gray_image *threshold_mask_initial_positive,
	DEVA_gray_image *threshold_mask_initial_negative,
	DEVA_gray_image *threshold_mask_positive,
	DEVA_gray_image *threshold_mask_negative,
	int smoothing_flag )
/*
 * sensitivity:			    sensitivity threshold
 * peak_frequency_image:	    peak frequency of band (used for smoothing)
 * contrast_band:		    output of bandpass filter
 * local_luminance:		    used to normalize contrast_band
 * thresholded_contrast_band:       contrast_band with below threshold value
 * 					set to zero
 * threshold_mask_initial_positive: used to help in smoothing
 * threshold_mask_initial_negative: used to help in smoothing
 * threshold_mask_positive:	    used to help in smoothing
 * threshold_mask_negative:	    used to help in smoothing
 * smoothing_flag:		    TRUE is smoothing should be done
 */
{
    int	    row, col;
    double  threshold;
    double  normalized_contrast;
    double  smoothing_radius;

    if ( sensitivity < 1.0 ) {	
	/* nothing will be visible! (should not happen) */
	DEVA_float_image_set_value ( thresholded_contrast_band, 0.0 );
	return;
    } else {
	threshold = 1.0 / sensitivity;
    }

    /*
     * smoothing_radius is SMOOTH_INTERVAL_RATIO * wavelength of band peak
     */
    smoothing_radius = SMOOTH_INTERVAL_RATIO *
	((double) imax ( DEVA_image_n_rows ( contrast_band ),
	    DEVA_image_n_cols ( contrast_band ) ) ) *
		( 1.0 / peak_frequency_image );
    if ( smoothing_radius > SMOOTH_MAXIMUM_RADIUS ) {
	smoothing_radius = SMOOTH_MAXIMUM_RADIUS;
    }

    if ( smoothing_flag && ( smoothing_radius >= 1.0 ) ) {
	/*
	 * Smoothing preserves below contrast values that are
	 * adjecent to above contrast values of the same sign.
	 */

	/*
	 * Get maps of above threshold contrasts.  Do this separately
	 * for positive and negative contrasts.
	 */
	for ( row = 0; row < DEVA_image_n_rows ( contrast_band ); row++ ) {
	    for ( col = 0; col < DEVA_image_n_cols ( contrast_band ); col++ ) {

		normalized_contrast =
		    DEVA_image_data ( contrast_band, row, col ) /
		        fmax (DEVA_image_data ( local_luminance, row, col ),
			    MIN_AVERAGE_LUMINANCE );

		DEVA_image_data ( threshold_mask_initial_positive, row, col ) =
		    ( normalized_contrast >= threshold );

		DEVA_image_data ( threshold_mask_initial_negative, row, col ) =
		    ( normalized_contrast <= -threshold );
	    }
	}

	/*
	 * Expand the maps by an amount proportional to the
	 * peak wavelength of the band.
	 */
	DEVA_gray_dilate2 ( threshold_mask_initial_positive,
		threshold_mask_positive, smoothing_radius );

	DEVA_gray_dilate2 ( threshold_mask_initial_negative,
		threshold_mask_negative, smoothing_radius );

	/*
	 * Keep any contrast that is flagged by the map of the
	 * appropriate sign.
	 */
	for ( row = 0; row < DEVA_image_n_rows ( contrast_band ); row++ ) {
	    for ( col = 0; col < DEVA_image_n_cols ( contrast_band ); col++ ) {
		if ( ( ( DEVA_image_data ( contrast_band, row, col ) > 0.0 )
			    && DEVA_image_data ( threshold_mask_positive,
				row, col ) ) ||
			( ( DEVA_image_data ( contrast_band, row, col ) < 0.0 )
			  && DEVA_image_data ( threshold_mask_negative,
			      				row, col ) ) ) {
		    DEVA_image_data ( thresholded_contrast_band, row, col ) =
			DEVA_image_data ( contrast_band, row, col );
		} else {
		    DEVA_image_data ( thresholded_contrast_band, row, col ) =
			0.0;
		}
	    }
	}
    } else {
	/*
	 * No smoothing, so just do straightforward thresholding.
	 */
	for ( row = 0; row < DEVA_image_n_rows ( contrast_band ); row++ ) {
	    for ( col = 0; col < DEVA_image_n_cols ( contrast_band ); col++ ) {
		normalized_contrast =
		    DEVA_image_data ( contrast_band, row, col ) /
		        fmax (DEVA_image_data ( local_luminance, row, col ),
			    MIN_AVERAGE_LUMINANCE );
		if ( fabs ( normalized_contrast ) >= threshold ) {
		    DEVA_image_data ( thresholded_contrast_band, row, col ) =
			DEVA_image_data ( contrast_band, row, col );
		}
		else {
		    DEVA_image_data ( thresholded_contrast_band, row, col ) =
									0.0;
		}
	    }
	}
    }
}

static DEVA_float_image *
CSF_weight_prep ( DEVA_complexf_image *frequency_space, double fov,
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
    unsigned int	n_rows, n_cols;
    double		row_dist, col_dist;
    double		CSF_peak_frequency;
    double		CSF_peak_sensitivity;
    double		frequency_angle;

    n_rows = DEVA_image_n_rows ( frequency_space );
    n_cols = DEVA_image_n_cols ( frequency_space );

    CSF_weights = DEVA_float_image_new ( n_rows, n_cols );

    CSF_peak_frequency = ChungLeggeCSF_peak_frequency ( acuity,
	    contrast_sensitivity );
    CSF_peak_sensitivity = ChungLeggeCSF_peak_sensitivity ( acuity,
	    contrast_sensitivity );

    /*
     * DC at [0][0], full resolution in row dimension (requires
     * offset), half resolution in col dimension (use as-is).
     */
    for ( col = 0; col < n_cols; col++ ) {
	frequency_angle = ((double) col ) / fov;

	if ( frequency_angle <= CSF_peak_frequency ) {
	    DEVA_image_data ( CSF_weights, 0, col ) = 1.0;
	} else {
	    DEVA_image_data ( CSF_weights, 0, col ) =
		ChungLeggeCSF ( frequency_angle, acuity,
		    contrast_sensitivity ) /  CSF_peak_sensitivity;
	}
    }

    for ( row = 1; row < ( n_rows + 1 ) / 2; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) row;
	    col_dist = (double) col;
	    frequency_angle = sqrt ( ( row_dist * row_dist ) +
		    ( col_dist * col_dist ) ) / fov;

	    if ( frequency_angle <= CSF_peak_frequency ) {
		DEVA_image_data ( CSF_weights, row, col ) = 1.0;
	    } else {
		DEVA_image_data ( CSF_weights, row, col ) = ChungLeggeCSF ( frequency_angle,
			acuity, contrast_sensitivity ) /  CSF_peak_sensitivity;
	    }
	}
    }

    for ( row = ( n_rows + 1 ) / 2; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) ( n_rows - row );
	    col_dist = (double) col;
	    frequency_angle = sqrt ( ( row_dist * row_dist ) +
		    ( col_dist * col_dist ) ) / fov;

	    if ( frequency_angle <= CSF_peak_frequency ) {
		DEVA_image_data ( CSF_weights, row, col ) = 1.0;
	    } else {
		DEVA_image_data ( CSF_weights, row, col ) = ChungLeggeCSF ( frequency_angle,
			acuity, contrast_sensitivity ) /  CSF_peak_sensitivity;
	    }
	}
    }

    return ( CSF_weights );
}

static DEVA_float_image *
filter_color ( DEVA_float_image *chroma_channel, DEVA_float_image *CSF_weights )
/*
 * Filter chroma_channel using CSF as if it were an MTF.
 */
{
    DEVA_float_image	*filtered_chroma_channel;
    fftwf_plan		fft_inverse_plan;
    DEVA_complexf_image	*frequency_space;
    double		norm;
    int			row, col;

    frequency_space = forward_transform ( chroma_channel );

    for ( row = 0; row < DEVA_image_n_rows ( CSF_weights ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( CSF_weights ); col++ ) {
	    DEVA_image_data ( frequency_space, row, col ) =
		rxc ( DEVA_image_data ( CSF_weights, row, col ),
			DEVA_image_data ( frequency_space, row, col ) );
	}
    }

    filtered_chroma_channel =
	DEVA_float_image_new ( DEVA_image_n_rows ( chroma_channel ),
	    DEVA_image_n_cols ( chroma_channel ) );

    fft_inverse_plan =
	fftwf_plan_dft_c2r_2d ( DEVA_image_n_rows ( chroma_channel ),
		DEVA_image_n_cols ( chroma_channel ),
		(fftwf_complex *) &DEVA_image_data ( frequency_space, 0, 0 ),
		&DEVA_image_data ( filtered_chroma_channel, 0, 0 ),
		FFTW_ESTIMATE);
    fftwf_execute ( fft_inverse_plan );

    norm = 1.0 / (double) ( DEVA_image_n_rows ( filtered_chroma_channel ) *
	    DEVA_image_n_cols ( filtered_chroma_channel ) );

    for ( row = 0; row < DEVA_image_n_rows ( filtered_chroma_channel );
	    					row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( filtered_chroma_channel );
						    col++ ) {
	    DEVA_image_data ( filtered_chroma_channel, row, col ) *= norm;
	}
    }

    fftwf_destroy_plan ( fft_inverse_plan );
    DEVA_complexf_image_delete ( frequency_space );

    return ( filtered_chroma_channel );
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
		DEVA_image_data ( input_image, row, col ) . Y;
	    DEVA_image_data (*x, row, col ) =
		DEVA_image_data ( input_image, row, col ) . x;
	    DEVA_image_data (*y, row, col ) =
		DEVA_image_data ( input_image, row, col ) . y;
	}
    }

    DEVA_image_view ( *x ) = DEVA_image_view ( input_image );
    DEVA_image_view ( *y ) = DEVA_image_view ( input_image );
    DEVA_image_view ( *luminance ) = DEVA_image_view ( input_image );

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
	fprintf ( stderr, "desaturate: incompatible inputs!\n" );
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
preallocate_images ( int n_rows, int n_cols,
    DEVA_complexf_image	**weighted_frequency_space,
    DEVA_float_image	**contrast_band,
    DEVA_float_image	**local_luminance,
    DEVA_float_image	**thresholded_contrast_band,
    DEVA_gray_image	**threshold_mask_initial_positive,
    DEVA_gray_image	**threshold_mask_initial_negative,
    DEVA_gray_image	**threshold_mask_positive,
    DEVA_gray_image	**threshold_mask_negative,
    DEVA_float_image	**filtered_luminance )
/*
 * Preallocate image objects that will be reused for each processed band.
 */
{
    int	    n_rows_transform, n_cols_transform;

    n_rows_transform = n_rows;
    n_cols_transform = ( n_cols / 2 ) + 1;

    *weighted_frequency_space =
	DEVA_complexf_image_new ( n_rows_transform, n_cols_transform );
    *contrast_band = DEVA_float_image_new ( n_rows, n_cols );
    *local_luminance = DEVA_float_image_new ( n_rows, n_cols );
    *thresholded_contrast_band = DEVA_float_image_new ( n_rows, n_cols );
    *threshold_mask_initial_positive = DEVA_gray_image_new ( n_rows, n_cols );
    *threshold_mask_initial_negative = DEVA_gray_image_new ( n_rows, n_cols );
    *threshold_mask_positive = DEVA_gray_image_new ( n_rows, n_cols );
    *threshold_mask_negative = DEVA_gray_image_new ( n_rows, n_cols );
    *filtered_luminance = DEVA_float_image_new ( n_rows, n_cols );
}

static void
cleanup (
    DEVA_complexf_image *frequency_space,
    DEVA_float_image *log2r,
    DEVA_complexf_image *weighted_frequency_space,
    DEVA_float_image *contrast_band,
    DEVA_float_image *local_luminance,
    DEVA_float_image *thresholded_contrast_band,
    DEVA_float_image *luminance,
    DEVA_float_image *x,
    DEVA_float_image *y,
    DEVA_float_image *filtered_luminance,
    DEVA_gray_image *threshold_mask_initial_positive,
    DEVA_gray_image *threshold_mask_initial_negative,
    DEVA_gray_image *threshold_mask_positive,
    DEVA_gray_image *threshold_mask_negative,
    double saturation,	/* needed for filtered_x and filtered_y */
    DEVA_float_image *filtered_x,
    DEVA_float_image *filtered_y,
    fftwf_plan *fft_inverse_plan )
/*
 * de-leak memory
 */
{
    DEVA_complexf_image_delete ( frequency_space );
    DEVA_float_image_delete ( log2r );
    DEVA_complexf_image_delete ( weighted_frequency_space );
    DEVA_float_image_delete ( contrast_band );
    DEVA_float_image_delete ( local_luminance );
    DEVA_float_image_delete ( thresholded_contrast_band );
    DEVA_float_image_delete ( luminance );
    DEVA_float_image_delete ( x );
    DEVA_float_image_delete ( y );
    DEVA_float_image_delete ( filtered_luminance );
    DEVA_gray_image_delete ( threshold_mask_initial_positive );
    DEVA_gray_image_delete ( threshold_mask_initial_negative );
    DEVA_gray_image_delete ( threshold_mask_positive );
    DEVA_gray_image_delete ( threshold_mask_negative );
    if ( saturation > 0.0 ) {
	DEVA_float_image_delete ( filtered_x );
	DEVA_float_image_delete ( filtered_y );
    }
    fftwf_destroy_plan ( *fft_inverse_plan );
    fftwf_cleanup ( );
}