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
 *   In-memory xyY values filtered to remove image structure predicted
 *   to be below the detectability threshold.
 */

/* #define	DeVAS_CHECK_BOUNDS */
/* #define	OUTPUT_CONTRAST_BANDS */	/* request debugging output */
					/* needs to be linked with libpng */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <fftw3.h>
#include "devas-filter.h"
#include "devas-image.h"
#include "devas-utils.h"
#include "ChungLeggeCSF.h"
#include "dilate.h"
#ifdef OUTPUT_CONTRAST_BANDS
#include "devas-png.h"
#endif	/* OUTPUT_CONTRAST_BANDS */
#include "devas-filter-version.h"
#include "devas-license.h"       /* DeVAS open source license */

/*
 * Misc. defines private to these routines:
 */

#define	MAX_PLAUSIBLE_ACUITY	4.0
#define	MAX_PLAUSIBLE_CONTRAST	4.0

#define	MIN_AVERAGE_LUMINANCE	0.01	/* avoid divide by 0 in normalization */

#define	SMOOTH_INTERVAL_RATIO	0.35	/* ratio of peak band wavelength */
					/* to thresholded contrast smothing */
					/* radius */
#define	SMOOTH_FEATHER_RATIO	0.5	/* outer portion of smoothing */
					/* that will be feathered if */
					/* necessary */

#define	LOG2R_0		-10.0	/* Marker value for value of log2r(0). */
				/* Can't happen in practice, since r is pixel */
				/* distance and so never less than 1.0 except */
				/* at DC.  Needs to be < 1.0 for log2r_min to */
				/* work. */

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

int  DeVAS_verbose = FALSE;
int  DeVAS_veryverbose = FALSE;
int  DeVAS_band_number = -1;

/*
 * Local functions:
 */

static void		preallocate_images ( int n_rows, int n_cols,
    			    DeVAS_complexf_image **weighted_frequency_space,
    			    DeVAS_float_image **contrast_band,
    			    DeVAS_float_image **local_luminance,
    			    DeVAS_float_image **thresholded_contrast_band,
    			    DeVAS_gray_image **threshold_mask_initial_positive,
    			    DeVAS_gray_image **threshold_mask_initial_negative,
    			    DeVAS_float_image **threshold_distsq_positive,
    			    DeVAS_float_image **threshold_distsq_negative,
			    DeVAS_float_image **filtered_luminance );
static DeVAS_complexf_image *forward_transform ( DeVAS_float_image *source );
static DeVAS_float_image	*log2r_prep ( DeVAS_complexf_image
							*transformed_image );
static void		bandpass_filter ( int band,
			    DeVAS_complexf_image *frequency_space,
			    DeVAS_complexf_image *weighted_frequency_space,
			    DeVAS_float_image *log2r,
			    DeVAS_float_image *contrast_band,
			    fftwf_plan fft_inverse_plan );
static void		apply_threshold ( double sensitivity,
			    float peak_frequency_image,
			    DeVAS_float_image *contrast_band,
			    DeVAS_float_image *local_luminance,
			    DeVAS_float_image *thresholded_contrast_band,
			    DeVAS_gray_image *threshold_mask_initial_positive,
			    DeVAS_gray_image *threshold_mask_initial_negative,
			    DeVAS_float_image *threshold_distsq_positive,
			    DeVAS_float_image *threshold_distsq_negative,
			    int smoothing_flag );
static float		feather ( float contrast, float distsq,
			    float smoothing_radius, float smoothing_feather );
static DeVAS_float_image	*CSF_weight_prep ( DeVAS_complexf_image
						*frequency_space,
				double fov, double acuity,
				double contrast_sensitivity );
static DeVAS_float_image *filter_color ( DeVAS_float_image *chroma_channel,
				DeVAS_float_image *CSF_weights );
static DeVAS_complexf	rxc ( DeVAS_float real_value,
			    DeVAS_complexf complex_value );
static void		disassemble_input ( DeVAS_xyY_image *input_image,
			    DeVAS_float_image **luminance,
			    DeVAS_float_image **x, DeVAS_float_image **y );
static DeVAS_xyY_image	*assemble_output ( DeVAS_float_image
							*filtered_luminance,
			    DeVAS_float_image *filtered_x,
			    DeVAS_float_image *filtered_y,
			    double saturation );
static void		desaturate ( double saturation, DeVAS_float_image *x,
			    DeVAS_float_image *y );
static DeVAS_xyY		clip_to_xyY_gamut ( DeVAS_xyY xyY );
static XY_point		line_intersection ( XY_point line_1_p1,
			    XY_point line_1_p2, XY_point line_2_p1,
			    XY_point line_2_p2 );
static void		cleanup ( DeVAS_complexf_image *frequency_space,
			    DeVAS_float_image *log2r,
			    DeVAS_complexf_image *weighted_frequency_space,
			    DeVAS_float_image *contrast_band,
			    DeVAS_float_image *local_luminance,
			    DeVAS_float_image *thresholded_contrast_band,
			    DeVAS_float_image *luminance,
			    DeVAS_float_image *x,
			    DeVAS_float_image *y,
			    DeVAS_float_image *filtered_luminance,
			    DeVAS_gray_image *threshold_mask_initial_positive,
			    DeVAS_gray_image *threshold_mask_initial_negative,
			    DeVAS_float_image *threshold_distsq_positive,
			    DeVAS_float_image *threshold_distsq_negative,
			    double saturation,
			    DeVAS_float_image *filtered_x,
			    DeVAS_float_image *filtered_y,
			    fftwf_plan *fft_inverse_plan );

DeVAS_xyY_image *
devas_filter ( DeVAS_xyY_image *input_image, double acuity,
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
    DeVAS_float_image	*luminance;	/* Y channel of input */
    DeVAS_float_image	*x;		/* chromaticity channels of input */
    DeVAS_float_image	*y;
    DeVAS_complexf_image *frequency_space;/* transformed image */
    float		DC;		/* l_0 in Peli (1990) */
					/* DC of transformed image */
    DeVAS_complexf_image *weighted_frequency_space;  /* G_i in Peli (1990) */
    DeVAS_float_image	*log2r;		/* (re)used to creat bandpass weights */
    DeVAS_float_image	*CSF_weights;	/* (re)used for filtering color */
    DeVAS_float_image	*contrast_band;	/* a_i in Peli (1990) */
    DeVAS_float_image	*local_luminance; /* l_i in Peli (1990) */
    DeVAS_float_image	*thresholded_contrast_band; /* result of thesholding */
    fftwf_plan		fft_inverse_plan;	/* to create bandpass images */
    DeVAS_gray_image	*threshold_mask_initial_positive; /* threshold mask */
    DeVAS_gray_image	*threshold_mask_initial_negative; /* threshold mask */
    DeVAS_float_image	*threshold_distsq_positive;	 /* threshold mask */
    DeVAS_float_image	*threshold_distsq_negative;	 /* threshold mask */
    DeVAS_float_image	*filtered_luminance;	/* filtered luminance channel */
    DeVAS_float_image	*filtered_x = NULL;	/* filtered x chromaticity */
    						/* not always used */
    DeVAS_float_image	*filtered_y = NULL;	/* filtered x chromaticity */
    						/* not always used */
    DeVAS_xyY_image	*filtered_image;	/* full xyY output image */

    /*
     * Check argument validity.
     */

    if ( DeVAS_image_view ( input_image ) . type == 0 ) {
	fprintf ( stderr, "missing or invalid view record in input image!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( ( acuity <= 0.0 ) || ( acuity > MAX_PLAUSIBLE_ACUITY ) ) {
	fprintf ( stderr, "invalid or implausible acuity value (%f)!\n",
		acuity );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( ( contrast_sensitivity <= 0.0 ) ||
	    ( contrast_sensitivity > MAX_PLAUSIBLE_CONTRAST ) ) {
	fprintf ( stderr, "invalid or implausible contrast value (%f)!\n",
		contrast_sensitivity );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( ( saturation < 0.0 ) || ( saturation > 1.0 ) ) {
	fprintf ( stderr, "invalid or implausible saturation value (%f)!\n",
		saturation );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /*
     * One-time jobs:
     */

    disassemble_input ( input_image, &luminance, &x, &y );
    	/* break input into separate luminance and chromaticity channels */
    	/* allocates luminance, x, and y images */

    /*
     * Field-of-view needed in order to compute degrees/pixel, which is
     * necessary to accociate image with spatial frequencies.  FOV is (or
     * at least should be) in VIEW record in Radiance input .hdr image.
     * devas_commandline copies the VIEW record from the Radiance file to the
     * xyY input_image object.  disassemble_input copies the VIEW record from
     * the input_image object to the luminance, x, and y image objects.
     *
     * The fov used in the computation is the larger of the horizontal and
     * vertical fields-of-view.
     */
    fov = fmax ( DeVAS_image_view ( luminance ) . vert,
	    DeVAS_image_view ( luminance ) . horiz );
    if ( fov <= 0.0 ) {
	fprintf ( stderr, "devas_filter: invalid or missing fov (%f, %f)!\n",
		DeVAS_image_view ( luminance ) . vert,
		DeVAS_image_view ( luminance ) . horiz );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( DeVAS_verbose ) {
	fprintf ( stderr, "FOV = %.1f degrees\n", fov );
	ChungLeggeCSF_print_stats ( acuity, contrast_sensitivity );
    }

    /* preallocate image objects that will be reused for each processed band */
    preallocate_images ( DeVAS_image_n_rows ( luminance ),
	        DeVAS_image_n_cols ( luminance ),
		&weighted_frequency_space,
		&contrast_band,
		&local_luminance,
		&thresholded_contrast_band,
		&threshold_mask_initial_positive,
		&threshold_mask_initial_negative,
		&threshold_distsq_positive,
		&threshold_distsq_negative,
		&filtered_luminance );

    frequency_space = forward_transform ( luminance );	/* only done once */
    DC = DeVAS_image_data ( frequency_space, 0, 0 ) . real /
	((double) ( DeVAS_image_n_rows ( luminance ) *
	    DeVAS_image_n_cols ( luminance ) ) );
    		/*
		 * FFTW requires normalization by the product of the dimensions.
		 */

    /*
     * Local_luminance and filtered_luminance are iteratively computed
     * across bands.  This sets the starting values.
     */
    DeVAS_float_image_setvalue ( local_luminance, DC );
    	/* l_i in Peli (1990) */

    DeVAS_float_image_setvalue ( filtered_luminance, DC );
    	/* a_i in Peli (1990) */

    /* get a bit of speed by reusing for every band */
    log2r = log2r_prep ( frequency_space );

    /* get a bit of speed by reusing for every band */
    fft_inverse_plan =
	fftwf_plan_dft_c2r_2d ( DeVAS_image_n_rows ( luminance ),
	    	DeVAS_image_n_cols ( luminance ),
		(fftwf_complex *)
		    &DeVAS_image_data ( weighted_frequency_space, 0, 0 ),
		&DeVAS_image_data ( contrast_band, 0, 0 ),
		FFTW_ESTIMATE);

    /*
     * Iterate through bands to compute filtered_luminance:
     */

    n_bands = 0;
    n_lf_skipped = 0;
    n_bands_max = (int)
	ceil ( log2 ((double) imax ( DeVAS_image_n_rows ( luminance ),
			DeVAS_image_n_cols ( luminance ) ) ) );
    		/* may miss (very) high frequencies on diagonal */

    if ( DeVAS_veryverbose ) {
	fprintf ( stderr,
      "\nband  frequency     wavelength    peak\n"
        "     image angle   image angle sensitivity\n" );
    }

    for ( band = 0; band < n_bands_max; band++ ) {
	/* Iterate through bands from low to high frequency */

	/*
	 * Peak of cosine band in cycles/image.
	 * Frequency is relative to the longer axis of the image.
	 */
	peak_frequency_image = pow ( 2.0, band );

	/*
	 * Peak of cosine band in spatial frequency units, specified as
	 * visual angle.
	 */
	peak_frequency_angle = peak_frequency_image / fov;

	/* sensitivity at peak of cosine band */
	peak_sensitivity = ChungLeggeCSF ( peak_frequency_angle, acuity,
		contrast_sensitivity );

	if ( DeVAS_veryverbose ) {
	    fprintf ( stderr,
		"%2d: %6.2f %5.2f  %6.2f %5.2f  %6.2f\n",
		    band,
		    peak_frequency_image, peak_frequency_angle,
		    1.0 / peak_frequency_image, 1.0 / peak_frequency_angle,
		    peak_sensitivity );
	}

	/*
	 * End iterating over bands if/when sensitivity is < 1.0 for a
	 * frequency > peak sensitivity.
	 */
	if ( ( peak_frequency_angle > ChungLeggeCSF_peak_frequency ( acuity,
			contrast_sensitivity ) ) &&
		( peak_sensitivity < 1.0 ) ) {
	    if ( DeVAS_veryverbose ) {
		fprintf ( stderr,
    "ending iterations: below threshold bands on high frequency side of CSF\n"
			);
	    }
	    break;
	}

	n_bands++;	/* on to the next band */

	if ( peak_sensitivity < 1.0 ) {
	    /* skip below threshold band on low frequency side of CSF */
	    if ( DeVAS_veryverbose ) {
		fprintf ( stderr,
	"skipping below threshold band on low frequency side of CSF\n" );
	    }
	    n_lf_skipped++;
	    continue;
	} else {
	    /* compute the bandpass band */
	    bandpass_filter ( band,  frequency_space, weighted_frequency_space,
		    log2r, contrast_band, fft_inverse_plan );

	    /*
	     * treat bandpass band as local contrast and threshold based
	     * on CSF sensitivity
	     */
#ifdef OUTPUT_CONTRAST_BANDS
	    DeVAS_band_number = band;
#endif	/* OUTPUT_CONTRAST_BANDS */

	    apply_threshold ( peak_sensitivity, peak_frequency_image,
		    contrast_band, local_luminance, thresholded_contrast_band,
		    threshold_mask_initial_positive,
		    threshold_mask_initial_negative,
		    threshold_distsq_positive, threshold_distsq_negative,
		    smoothing_flag );

	    /* add another level to the image pyramid */
	    DeVAS_float_image_addto ( filtered_luminance,
		    thresholded_contrast_band );
	}

	DeVAS_float_image_addto ( local_luminance, contrast_band );
		/* for use in next iteration */
	    	/* do this even when skipping below threshold band on */
		/* low frequency side of CSF */
    }

    if ( DeVAS_veryverbose ) {
	fprintf ( stderr, "n_bands = %d, n_lf_skipped = %d\n", n_bands,
		n_lf_skipped );
    }

    if ( n_bands == n_lf_skipped ) {
	fprintf ( stderr, "devas-filter: no above threshold contrast!\n" );
    }

    if ( saturation > 0.0 ) {
	/*
	 * Some amount of chromaticity needs to be preserved.
	 * The spatial distribution of chroma values (x and y) is filtered
	 * using the lumiance CSF in order to avoid having sharp color
	 * boundaries confound the appearance of blurred luminance boundaries.
	 * This is a useful heuristic, but not based on any photometric
	 * model of low vision color perception.
	 */
	CSF_weights = CSF_weight_prep ( frequency_space, fov, acuity,
		contrast_sensitivity );

	/* spatial processing of color channels */
	filtered_x = filter_color ( x, CSF_weights );
	filtered_y = filter_color ( y, CSF_weights );

	/* partial desaturation of color channels */
	desaturate ( saturation, filtered_x, filtered_y );

	/* clean up */
	DeVAS_float_image_delete ( CSF_weights );
    }

    filtered_image = assemble_output ( filtered_luminance, filtered_x,
	    filtered_y, saturation );
	/* reassemble separate luminance and chromaticity channels into */
	/* single output image */

    /* keep exposure values as before */
    DeVAS_image_exposure_set ( filtered_image ) =
	DeVAS_image_exposure_set ( input_image );
    DeVAS_image_exposure ( filtered_image ) =
	DeVAS_image_exposure ( input_image );

    /* nothing's changed in the view */
    DeVAS_image_view ( filtered_image ) = DeVAS_image_view ( input_image );

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
		threshold_distsq_positive,
		threshold_distsq_negative,
		saturation,
		filtered_x,
		filtered_y,
		&fft_inverse_plan );

    return ( filtered_image );
}

void
devas_filter_print_version ( void )
{
   fprintf ( stderr, "devas_filter version %s\n", DeVAS_FILTER_VERSION_STRING );
}

static DeVAS_complexf_image *
forward_transform ( DeVAS_float_image *source )
{
    unsigned int	n_rows_input, n_cols_input;
    unsigned int	n_rows_transform, n_cols_transform;
    DeVAS_complexf_image	*transformed_image;
    fftwf_plan		fft_forward_plan;

    n_rows_input = DeVAS_image_n_rows ( source );
    n_cols_input = DeVAS_image_n_cols ( source );

    n_rows_transform = n_rows_input;
    n_cols_transform = ( n_cols_input / 2 ) + 1;

    transformed_image =
	DeVAS_complexf_image_new ( n_rows_transform, n_cols_transform );

    fft_forward_plan = fftwf_plan_dft_r2c_2d ( n_rows_input, n_cols_input,
	    &DeVAS_image_data ( source, 0, 0 ),
	    (fftwf_complex *) &DeVAS_image_data ( transformed_image, 0, 0 ),
	    FFTW_ESTIMATE );
    	/* FFTW3 documentation says to always create plan before initializing */
        /* input array, but then goes on to say that "technically" this is */
        /* not required for FFTW_ESTIMATE */

    fftwf_execute ( fft_forward_plan );

    fftwf_destroy_plan ( fft_forward_plan );

    return ( transformed_image );
}

static DeVAS_float_image *
log2r_prep ( DeVAS_complexf_image *transformed_image )
/*
 * Precompute log_2(r) in equation A2 of Peli (1990).
 *
 * These values can be reused for each band, thus saving repetitions
 * of square, square root, and log2 computations.
 *
 * transformed_image:	Used only to pass size of transformed image.
 */
{
    DeVAS_float_image	*log2r;
    int			row, col;
    unsigned int	n_rows, n_cols;
    double		row_dist, col_dist;

    n_rows = DeVAS_image_n_rows ( transformed_image );
    n_cols = DeVAS_image_n_cols ( transformed_image );

    log2r = DeVAS_float_image_new ( n_rows, n_cols );

    /*
     * DC at [0][0], full resolution in row dimension (requires
     * offset), half resolution in col dimension (use as-is).
     *
     * Need to special case first row to avoid log2(0.0).
     */
    DeVAS_image_data (log2r, 0, 0 ) = LOG2R_0;
				/* make sure log2(0) is never used */
    				/* set this value to be < -1.0 to make range */
				/* of band 0 work in bandpass_filter */
    for ( col = 1; col < n_cols; col++ ) {
	DeVAS_image_data ( log2r, 0, col ) = log2 ( (double) col );
    }

    /* first half of transform */
    for ( row = 1; row < ( n_rows + 1 ) / 2; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) row;
	    col_dist = (double) col;

	    DeVAS_image_data ( log2r, row, col ) =
		log2 ( sqrt ( ( row_dist * row_dist ) +
			    ( col_dist * col_dist ) ) );
	}
    }

    /* second half of transform */
    for ( row = ( n_rows + 1 ) / 2; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) ( n_rows - row );
	    col_dist = (double) col;

	    DeVAS_image_data ( log2r, row, col ) =
		log2 ( sqrt ( ( row_dist * row_dist ) +
			    ( col_dist * col_dist ) ) );
	}
    }

    return ( log2r );
}

static void
bandpass_filter ( int band,  DeVAS_complexf_image *frequency_space,
	DeVAS_complexf_image *weighted_frequency_space,
	DeVAS_float_image *log2r, DeVAS_float_image *contrast_band,
	fftwf_plan fft_inverse_plan )
/*
 * Weight frequency space values using equation A2 in Peli (1990). Weights are
 * a shifted cosine over (-pi - pi), centered at the band frequency, with a
 * log2 scaling of the cosine angle.
 */
{
    int     row, col;
    double  log2r_min, log2r_max;
    double  log2r_value;
    double  filter_weight;
    double  norm;

    log2r_min = (double) ( band - 1 );
    log2r_max = (double) ( band + 1 );

    for ( row = 0; row < DeVAS_image_n_rows ( frequency_space ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( frequency_space ); col++ ) {
	    log2r_value = DeVAS_image_data ( log2r, row, col );
	    if ( ( log2r_value > log2r_min ) &&
		    ( log2r_value < log2r_max ) ) {
		filter_weight = 0.5 * ( 1.0 +
			cos ( ( log2r_value - (double) band ) * M_PI ) );
	    } else {
		filter_weight = 0.0;
	    }

	    DeVAS_image_data ( weighted_frequency_space, row, col ) =
		rxc ( filter_weight,
			DeVAS_image_data ( frequency_space, row, col ) );
	}
    }

    fftwf_execute ( fft_inverse_plan );	/* specified in calling program: */
    					/* src: weighted_frequency_space */
    					/* dst: contrast_band */

    norm = 1.0 / (double) ( DeVAS_image_n_rows ( contrast_band ) *
	    DeVAS_image_n_cols ( contrast_band ) );

    for ( row = 0; row < DeVAS_image_n_rows ( contrast_band ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( contrast_band ); col++ ) {
	    DeVAS_image_data ( contrast_band, row, col ) *= norm;
	}
    }
}

static void
apply_threshold ( double sensitivity, float peak_frequency_image,
	DeVAS_float_image *contrast_band, DeVAS_float_image *local_luminance,
	DeVAS_float_image *thresholded_contrast_band,
	DeVAS_gray_image *threshold_mask_initial_positive,
	DeVAS_gray_image *threshold_mask_initial_negative,
	DeVAS_float_image *threshold_distsq_positive,
	DeVAS_float_image *threshold_distsq_negative,
	int smoothing_flag )
/*
 * Return (in thresholded_contrast_band) the thresholded contrast band, where
 * the threshold is applied to the normalized contrast value. If done as
 * described in Peli (1990), this leaves a sharp edge at the transition between
 * above and below threshold values (see Thompson, 2017).  This artifact can
 * be reduced by keeping below thrshold values that are sufficiently close to
 * an above threshold value of the same sign, where "sufficiently close" is
 * defined in terms of the peak frequency of the band being processed.
 * Elongated region of below threshold values that are adjacent to a same-sign
 * above threshold values can still generate artificats as the "sufficiently
 * close" boundary.  To address this issue, the the below contrast values
 * retained by this process are feathered towards 0 at distances approaching
 * the "sufficiently close" boundary.
 *
 * sensitivity:			    sensitivity threshold
 * peak_frequency_image:	    peak frequency of band (used for smoothing)
 * contrast_band:		    output of bandpass filter
 * local_luminance:		    used to normalize contrast_band
 * thresholded_contrast_band:       contrast_band with below threshold value
 * 					set to zero
 * threshold_mask_initial_positive: used to help in smoothing
 * threshold_mask_initial_negative: used to help in smoothing
 * threshold_distsq_positive:	    used to help in smoothing
 * threshold_distsq_negative:	    used to help in smoothing
 * smoothing_flag:		    TRUE if smoothing should be done
 */
{
    int	    row, col;
    double  threshold;
    double  normalized_contrast;
    double  smoothing_radius;
    double  smoothing_feather;
#ifdef OUTPUT_CONTRAST_BANDS
    DeVAS_gray_image	*debug_display_image; /* for thresholded contrast */
    double		max_thresholded_contrast, min_thresholded_contrast;
    double		max_contrast, min_contrast;
    double		norm_thresholded_contrast;
    double		norm_contrast;
    char		debug_filename[1000];	/* warning: fixed size!!! */

    debug_display_image =
	DeVAS_gray_image_new ( DeVAS_image_n_rows ( contrast_band ),
		DeVAS_image_n_cols ( contrast_band ) );
#endif	/* OUTPUT_CONTRAST_BANDS */

    if ( sensitivity < 1.0 ) {	
	/* nothing will be visible! (should not happen) */
	DeVAS_float_image_setvalue ( thresholded_contrast_band, 0.0 );
	return;
    } else {
	threshold = 1.0 / sensitivity;
    }

    /*
     * smoothing_radius is SMOOTH_INTERVAL_RATIO * wavelength of band peak
     */
    smoothing_radius = SMOOTH_INTERVAL_RATIO *
	((double) imax ( DeVAS_image_n_rows ( contrast_band ),
	    DeVAS_image_n_cols ( contrast_band ) ) ) *
	( 1.0 / peak_frequency_image );
    smoothing_feather = ( 1.0 - SMOOTH_FEATHER_RATIO ) * smoothing_radius;

    if ( smoothing_flag && ( smoothing_radius >= 1.0 ) ) {
	/*
	 * Smoothing preserves below contrast values that are
	 * adjacent to above contrast values of the same sign.
	 */

	/*
	 * Get maps of above threshold contrasts.  Do this separately
	 * for positive and negative contrasts.
	 */
	if ( DeVAS_veryverbose ) {
	    fprintf ( stderr, "smoothing_radius = %f, smoothing_feather = %f\n",
		    smoothing_radius, smoothing_feather );
	}
	for ( row = 0; row < DeVAS_image_n_rows ( contrast_band ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( contrast_band ); col++ ) {

		normalized_contrast =
		    DeVAS_image_data ( contrast_band, row, col ) /
		    fmax (DeVAS_image_data ( local_luminance, row, col ),
			    MIN_AVERAGE_LUMINANCE );

		DeVAS_image_data ( threshold_mask_initial_positive, row, col ) =
		    ( normalized_contrast >= threshold );

		DeVAS_image_data ( threshold_mask_initial_negative, row, col ) =
		    ( normalized_contrast <= -threshold );
	    }
	}

	/*
	 * Get distance from above threshold positive and negative contrast
	 * pixels.
	 */
	dt_euclid_sq_2 ( threshold_mask_initial_positive,
		threshold_distsq_positive );
	dt_euclid_sq_2 ( threshold_mask_initial_negative,
		threshold_distsq_negative );

	/*
	 * Keep any contrast that is flagged by the map of the
	 * appropriate sign.
	 */
	for ( row = 0; row < DeVAS_image_n_rows ( contrast_band ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( contrast_band ); col++ ) {

		if ( DeVAS_image_data ( contrast_band, row, col ) > 0.0 ) {
		    DeVAS_image_data ( thresholded_contrast_band, row, col ) =
			feather ( DeVAS_image_data ( contrast_band, row, col ),
			    DeVAS_image_data ( threshold_distsq_positive, row,
				col ), smoothing_radius, smoothing_feather );
		} else {
		    DeVAS_image_data ( thresholded_contrast_band, row, col ) =
			feather ( DeVAS_image_data ( contrast_band, row, col ),
			    DeVAS_image_data ( threshold_distsq_negative, row,
				col ), smoothing_radius, smoothing_feather );
		}
	    }
	}

    } else {
	/*
	 * No smoothing, so just do straightforward thresholding.
	 */
	for ( row = 0; row < DeVAS_image_n_rows ( contrast_band ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( contrast_band ); col++ ) {
		normalized_contrast =
		    DeVAS_image_data ( contrast_band, row, col ) /
		    fmax (DeVAS_image_data ( local_luminance, row, col ),
			    MIN_AVERAGE_LUMINANCE );
		if ( fabs ( normalized_contrast ) >= threshold ) {
		    DeVAS_image_data ( thresholded_contrast_band, row, col ) =
			DeVAS_image_data ( contrast_band, row, col );
		}
		else {
		    DeVAS_image_data ( thresholded_contrast_band, row, col ) =
			0.0;
		}
	    }
	}
    }

#ifdef OUTPUT_CONTRAST_BANDS

    max_contrast = min_contrast = DeVAS_image_data ( contrast_band, 0, 0 );
    for ( row = 0; row < DeVAS_image_n_rows ( contrast_band ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( contrast_band ); col++ ) {
	    max_contrast = fmax ( max_contrast,
		    DeVAS_image_data ( contrast_band, row, col ) );
	    min_contrast = fmin ( min_contrast,
		    DeVAS_image_data ( contrast_band, row, col ) );
	}
    }
    norm_contrast = 0.5 / fmax ( max_contrast, fabs ( min_contrast ) );

    for ( row = 0; row < DeVAS_image_n_rows ( contrast_band ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( contrast_band ); col++ ) {
	    DeVAS_image_data ( debug_display_image, row, col ) =
		(int) ( 255.99 * ( ( norm_contrast *
			DeVAS_image_data ( contrast_band, row, col ) ) + 0.5 ) );
	    if ( DeVAS_image_data ( debug_display_image, row, col ) < 0 ) {
		DeVAS_image_data ( debug_display_image, row, col ) = 0;
	    }
	    if ( DeVAS_image_data ( debug_display_image, row, col ) > 255 ) {
		DeVAS_image_data ( debug_display_image, row, col ) = 255;
	    }
	}
    }

    sprintf ( debug_filename, "unthresholded_contrast_band_%02d.png",
	    DeVAS_band_number );
    DeVAS_gray_image_to_filename_png ( debug_filename,
	    debug_display_image );

    max_thresholded_contrast = min_thresholded_contrast =
	DeVAS_image_data ( thresholded_contrast_band, 0, 0 );
    for ( row = 0;
	    row < DeVAS_image_n_rows ( thresholded_contrast_band );
	    row++ ) {
	for ( col = 0;
		col < DeVAS_image_n_cols ( thresholded_contrast_band );
		col++ ) {
	    max_thresholded_contrast = fmax ( max_thresholded_contrast,
		    DeVAS_image_data ( thresholded_contrast_band, row,
			col ) );
	    min_thresholded_contrast = fmin ( min_thresholded_contrast,
		    DeVAS_image_data ( thresholded_contrast_band,
			row, col ) );
	}
    }
    norm_thresholded_contrast = 0.5 / fmax ( max_thresholded_contrast,
	    fabs ( min_thresholded_contrast ) );

    for ( row = 0;
	    row < DeVAS_image_n_rows ( thresholded_contrast_band );
	    row++ ) {
	for ( col = 0;
		col < DeVAS_image_n_cols ( thresholded_contrast_band );
		col++ ) {
	    DeVAS_image_data ( debug_display_image, row, col ) =
		(int) ( 255.99 *
			( ( norm_thresholded_contrast *
			    DeVAS_image_data ( thresholded_contrast_band,
				row, col ) ) + 0.5 ) );
	    if ( DeVAS_image_data ( debug_display_image, row, col )
		    < 0 ) {
		DeVAS_image_data ( debug_display_image, row, col ) = 0;
	    }
	    if ( DeVAS_image_data ( debug_display_image, row, col )
		    > 255 ) {
		DeVAS_image_data ( debug_display_image, row, col ) = 255;
	    }
	}
    }

    sprintf ( debug_filename, "thresholded_contrast_band_%02d.png",
	    DeVAS_band_number );
    DeVAS_gray_image_to_filename_png ( debug_filename,
	    debug_display_image );

#endif	/* OUTPUT_CONTRAST_BANDS */

}

static float
feather ( float contrast, float distsq, float smoothing_radius,
	float smoothing_feather )
/*
 * Restore the full amount of the below contrast value if closer than
 * smoothing_feather contrast value of the same sign.  When the distance is
 * in the range [smoothing_feather -- smoothing_radius], linearly reduce this
 * restored value as the location approaches smoothing_radius.
 */
{
    if ( distsq < ( smoothing_feather * smoothing_feather ) ) {
	return ( contrast );
    } else if ( distsq > ( smoothing_radius * smoothing_radius ) ) {
	return ( 0.0 );
    } else {
	return ( ( ( smoothing_radius - sqrt ( distsq ) ) /
		    ( smoothing_radius - smoothing_feather ) ) * contrast );
    }
}

static DeVAS_float_image *
CSF_weight_prep ( DeVAS_complexf_image *frequency_space, double fov,
	double acuity, double contrast_sensitivity )
/*
 * Precompute CSF-based filter weights for filtering color channels.
 * Suppress low frequency rolloff in CSF to avoid visual artifacts.
 * Normalize CSF to have a peak value of 1.0.
 *
 * These values can be reused for each color, thus saving repetitions
 * of square, square root, and CSF computations.
 *
 * frequency_space:	Used only to pass size of transformed image.
 */
{
    DeVAS_float_image	*CSF_weights;
    int			row, col;
    unsigned int	n_rows, n_cols;
    double		row_dist, col_dist;
    double		CSF_peak_frequency;
    double		CSF_peak_sensitivity;
    double		frequency_angle;

    n_rows = DeVAS_image_n_rows ( frequency_space );
    n_cols = DeVAS_image_n_cols ( frequency_space );

    CSF_weights = DeVAS_float_image_new ( n_rows, n_cols );

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
	    DeVAS_image_data ( CSF_weights, 0, col ) = 1.0;
	} else {
	    DeVAS_image_data ( CSF_weights, 0, col ) =
		ChungLeggeCSF ( frequency_angle, acuity,
		    contrast_sensitivity ) /  CSF_peak_sensitivity;
	}
    }

    /* first half of transform */
    for ( row = 1; row < ( n_rows + 1 ) / 2; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) row;
	    col_dist = (double) col;
	    frequency_angle = sqrt ( ( row_dist * row_dist ) +
		    ( col_dist * col_dist ) ) / fov;

	    if ( frequency_angle <= CSF_peak_frequency ) {
		DeVAS_image_data ( CSF_weights, row, col ) = 1.0;
	    } else {
		DeVAS_image_data ( CSF_weights, row, col ) =
		    ChungLeggeCSF ( frequency_angle, acuity,
			    contrast_sensitivity ) /  CSF_peak_sensitivity;
	    }
	}
    }

    /* second half of transform */
    for ( row = ( n_rows + 1 ) / 2; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    row_dist = (double) ( n_rows - row );
	    col_dist = (double) col;
	    frequency_angle = sqrt ( ( row_dist * row_dist ) +
		    ( col_dist * col_dist ) ) / fov;

	    if ( frequency_angle <= CSF_peak_frequency ) {
		DeVAS_image_data ( CSF_weights, row, col ) = 1.0;
	    } else {
		DeVAS_image_data ( CSF_weights, row, col ) =
		    ChungLeggeCSF ( frequency_angle, acuity,
			    contrast_sensitivity ) /  CSF_peak_sensitivity;
	    }
	}
    }

    return ( CSF_weights );
}

static DeVAS_float_image *
filter_color ( DeVAS_float_image *chroma_channel,
	DeVAS_float_image *CSF_weights )
/*
 * Filter chroma_channel using CSF as if it were an MTF.
 */
{
    DeVAS_float_image	*filtered_chroma_channel;
    fftwf_plan		fft_inverse_plan;
    DeVAS_complexf_image	*frequency_space;
    double		norm;
    int			row, col;

    /* forward FFT */
    frequency_space = forward_transform ( chroma_channel );

    /* multiply by frequency space CSF values */
    for ( row = 0; row < DeVAS_image_n_rows ( CSF_weights ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( CSF_weights ); col++ ) {
	    DeVAS_image_data ( frequency_space, row, col ) =
		rxc ( DeVAS_image_data ( CSF_weights, row, col ),
			DeVAS_image_data ( frequency_space, row, col ) );
	}
    }

    /* inverse FFT */
    filtered_chroma_channel =
	DeVAS_float_image_new ( DeVAS_image_n_rows ( chroma_channel ),
	    DeVAS_image_n_cols ( chroma_channel ) );
    fft_inverse_plan =
	fftwf_plan_dft_c2r_2d ( DeVAS_image_n_rows ( chroma_channel ),
		DeVAS_image_n_cols ( chroma_channel ),
		(fftwf_complex *) &DeVAS_image_data ( frequency_space, 0, 0 ),
		&DeVAS_image_data ( filtered_chroma_channel, 0, 0 ),
		FFTW_ESTIMATE);
    fftwf_execute ( fft_inverse_plan );

    /* normalize */
    norm = 1.0 / (double) ( DeVAS_image_n_rows ( filtered_chroma_channel ) *
	    DeVAS_image_n_cols ( filtered_chroma_channel ) );
    for ( row = 0; row < DeVAS_image_n_rows ( filtered_chroma_channel );
	    					row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( filtered_chroma_channel );
						    col++ ) {
	    DeVAS_image_data ( filtered_chroma_channel, row, col ) *= norm;
	}
    }

    /* clean up */
    fftwf_destroy_plan ( fft_inverse_plan );
    DeVAS_complexf_image_delete ( frequency_space );

    return ( filtered_chroma_channel );
}

static DeVAS_complexf
rxc ( DeVAS_float real_value, DeVAS_complexf complex_value )
/*
 * Multiply a complex number by a real value.
 */
{
    DeVAS_complexf value;

    value.real = real_value * complex_value.real;
    value.imaginary = real_value * complex_value.imaginary;

    return ( value );
}

static void
disassemble_input ( DeVAS_xyY_image *input_image, DeVAS_float_image **luminance,
	DeVAS_float_image **x, DeVAS_float_image **y )
/*
 * Break input into separate luminance and chromaticity channels
 */
{
    int	    row, col;

    *luminance = DeVAS_float_image_new ( DeVAS_image_n_rows ( input_image ),
	    DeVAS_image_n_cols ( input_image ) );
    *x = DeVAS_float_image_new ( DeVAS_image_n_rows ( input_image ),
	    DeVAS_image_n_cols ( input_image ) );
    *y = DeVAS_float_image_new ( DeVAS_image_n_rows ( input_image ),
	    DeVAS_image_n_cols ( input_image ) );

    for ( row = 0; row < DeVAS_image_n_rows ( input_image ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( input_image ); col++ ) {
	    DeVAS_image_data ( *luminance, row, col ) =
		DeVAS_image_data ( input_image, row, col ) . Y;
	    DeVAS_image_data (*x, row, col ) =
		DeVAS_image_data ( input_image, row, col ) . x;
	    DeVAS_image_data (*y, row, col ) =
		DeVAS_image_data ( input_image, row, col ) . y;
	}
    }

    /* Copy over view record (for fov). */
    DeVAS_image_view ( *x ) = DeVAS_image_view ( input_image );
    DeVAS_image_view ( *y ) = DeVAS_image_view ( input_image );
    DeVAS_image_view ( *luminance ) = DeVAS_image_view ( input_image );

    if ( DeVAS_image_description ( input_image ) != NULL ) {
	DeVAS_image_description ( *luminance ) =
	    strdup ( DeVAS_image_description ( input_image ) );
    } else {
	DeVAS_image_description ( *luminance ) = NULL;
    }
}

static DeVAS_xyY_image *
assemble_output ( DeVAS_float_image *filtered_luminance,
	DeVAS_float_image *filtered_x, DeVAS_float_image *filtered_y,
	double saturation )
/*
 * Reassemble separate luminance and chromaticity channels into single
 * output image.
 */
{
    DeVAS_xyY	    xyY;
    DeVAS_xyY_image  *output_image;
    int		    row, col;

    output_image =
	DeVAS_xyY_image_new ( DeVAS_image_n_rows ( filtered_luminance ),
		DeVAS_image_n_cols ( filtered_luminance ) );

    if ( saturation > 0.0 )  {
	/* partially desaturated output requested */
	for ( row = 0; row < DeVAS_image_n_rows ( output_image ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( output_image ); col++ ) {
		xyY.x = DeVAS_image_data ( filtered_x, row, col );
		xyY.y = DeVAS_image_data ( filtered_y, row, col );
		xyY.Y = DeVAS_image_data ( filtered_luminance, row, col );

		DeVAS_image_data ( output_image, row, col ) =
		    clip_to_xyY_gamut ( xyY );
			/* filtered (x,y) values may be out of gamut */
	    }
	} 
    } else {
	/* totally desaturated output requested */
	for ( row = 0; row < DeVAS_image_n_rows ( output_image ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( output_image ); col++ ) {
		xyY.x = DeVAS_x_WHITEPOINT;
		xyY.y = DeVAS_y_WHITEPOINT;
		xyY.Y = DeVAS_image_data ( filtered_luminance, row, col );

		DeVAS_image_data ( output_image, row, col ) =
		    clip_to_xyY_gamut ( xyY );
			/* filtered (x,y) values may be out of gamut */
	    }
	}
    }

    return ( output_image );
}

static void
desaturate ( double saturation, DeVAS_float_image *x, DeVAS_float_image *y )
/*
 * In-place desaturation of (x,y) chromaticity channels.
 */
{
    int	    row, col;

    if ( !DeVAS_float_image_samesize ( x, y ) ) {
	fprintf ( stderr, "desaturate: incompatible inputs!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( saturation <= 0.0 ) {
	/* handled in assemble_output ( ) */
	return;
    }

    if ( saturation < 1.0 ) {
	for ( row = 0; row < DeVAS_image_n_rows ( x ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( x ); col++ ) {
		DeVAS_image_data ( x, row, col ) =
		    ( saturation * DeVAS_image_data ( x, row, col ) ) +
		    	( ( 1.0 - saturation ) * DeVAS_x_WHITEPOINT );
		DeVAS_image_data ( y, row, col ) =
		    ( saturation * DeVAS_image_data ( y, row, col ) ) +
		    	( ( 1.0 - saturation ) * DeVAS_y_WHITEPOINT );
	    }
	}
    }
}

static DeVAS_xyY
clip_to_xyY_gamut ( DeVAS_xyY xyY )
/*
 * If value is outside of gamut triangle, return the point on the gamut
 * triangle intersected by a line from the whitepoint to the value.
 */
{
    DeVAS_xyY	ingamut_xyY;
    XY_point	xy_point, origin, x_max, y_max, white_pt;
    XY_point	intersection;

    ingamut_xyY.Y = fmax ( xyY.Y, 0.0 );	/* Y can't be negative */

    xy_point.x = xyY.x; xy_point.y = xyY.y;
    origin.x = 0.0; origin.y = 0.0;
    x_max.x = 1.0; x_max.y = 0.0;
    y_max.x = 0.0; y_max.y = 1.0;
    white_pt.x = DeVAS_x_WHITEPOINT;
    white_pt.y = DeVAS_y_WHITEPOINT;

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
    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
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
 *
 * Algorithm taken from <https://en.wikipedia.org/wiki/Line–line_intersection>.
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
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
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
    DeVAS_complexf_image	**weighted_frequency_space,
    DeVAS_float_image	**contrast_band,
    DeVAS_float_image	**local_luminance,
    DeVAS_float_image	**thresholded_contrast_band,
    DeVAS_gray_image	**threshold_mask_initial_positive,
    DeVAS_gray_image	**threshold_mask_initial_negative,
    DeVAS_float_image	**threshold_distsq_positive,
    DeVAS_float_image	**threshold_distsq_negative,
    DeVAS_float_image	**filtered_luminance )
/*
 * Preallocate image objects that will be reused for each processed band.
 */
{
    int	    n_rows_transform, n_cols_transform;

    n_rows_transform = n_rows;
    n_cols_transform = ( n_cols / 2 ) + 1;

    *weighted_frequency_space =
	DeVAS_complexf_image_new ( n_rows_transform, n_cols_transform );
    *contrast_band = DeVAS_float_image_new ( n_rows, n_cols );
    *local_luminance = DeVAS_float_image_new ( n_rows, n_cols );
    *thresholded_contrast_band = DeVAS_float_image_new ( n_rows, n_cols );
    *threshold_mask_initial_positive = DeVAS_gray_image_new ( n_rows, n_cols );
    *threshold_mask_initial_negative = DeVAS_gray_image_new ( n_rows, n_cols );
    *threshold_distsq_positive = DeVAS_float_image_new ( n_rows, n_cols );
    *threshold_distsq_negative = DeVAS_float_image_new ( n_rows, n_cols );
    *filtered_luminance = DeVAS_float_image_new ( n_rows, n_cols );
}

static void
cleanup (
    DeVAS_complexf_image *frequency_space,
    DeVAS_float_image *log2r,
    DeVAS_complexf_image *weighted_frequency_space,
    DeVAS_float_image *contrast_band,
    DeVAS_float_image *local_luminance,
    DeVAS_float_image *thresholded_contrast_band,
    DeVAS_float_image *luminance,
    DeVAS_float_image *x,
    DeVAS_float_image *y,
    DeVAS_float_image *filtered_luminance,
    DeVAS_gray_image *threshold_mask_initial_positive,
    DeVAS_gray_image *threshold_mask_initial_negative,
    DeVAS_float_image *threshold_distsq_positive,
    DeVAS_float_image *threshold_distsq_negative,
    double saturation,	/* needed for filtered_x and filtered_y */
    DeVAS_float_image *filtered_x,
    DeVAS_float_image *filtered_y,
    fftwf_plan *fft_inverse_plan )
/*
 * de-leak memory
 */
{
    DeVAS_complexf_image_delete ( frequency_space );
    DeVAS_float_image_delete ( log2r );
    DeVAS_complexf_image_delete ( weighted_frequency_space );
    DeVAS_float_image_delete ( contrast_band );
    DeVAS_float_image_delete ( local_luminance );
    DeVAS_float_image_delete ( thresholded_contrast_band );
    DeVAS_float_image_delete ( luminance );
    DeVAS_float_image_delete ( x );
    DeVAS_float_image_delete ( y );
    DeVAS_float_image_delete ( filtered_luminance );
    DeVAS_gray_image_delete ( threshold_mask_initial_positive );
    DeVAS_gray_image_delete ( threshold_mask_initial_negative );
    DeVAS_float_image_delete ( threshold_distsq_positive );
    DeVAS_float_image_delete ( threshold_distsq_negative );
    if ( saturation > 0.0 ) {
	DeVAS_float_image_delete ( filtered_x );
	DeVAS_float_image_delete ( filtered_y );
    }
    fftwf_destroy_plan ( *fft_inverse_plan );
    fftwf_cleanup ( );
}
