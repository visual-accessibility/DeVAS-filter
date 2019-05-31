#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "visualize-hazards.h"
#include "devas-visibility.h"
#include "devas-image.h"
#include "devas-utils.h"
#include "devas-license.h"

#define	SQ(x)	((x) * (x))

static DeVAS_RGB		color_hazard_level ( double hazard_level,
			     Visualization_type visualization_type );
static DeVAS_gray_image	*expand_3x_i ( DeVAS_gray_image *image );
static DeVAS_float_image	*expand_3x_f ( DeVAS_float_image *image );

DeVAS_RGB_image *
visualize_hazards ( DeVAS_float_image *hazards,
	Measurement_type measurement_type, double scale_parameter,
	Visualization_type visualization_type,
	DeVAS_gray_image *mask, DeVAS_gray_image *ROI,
	DeVAS_gray_image **geometry_boundaries,
	double *hazard_average )
/*
 * Make displayable file showing predicted geometry discontinuities that are
 * not visible at specified level of low vision. 
 *
 *
 * hazards:		Visual angle from geometry boundaries to nearest
 *			luminance boundary.
 * Measurement_type:	Gaussian_measure
 * 			    1 - exp ( -0.5 * ( x / Gaussian_sigma ) ^ 2 )
 * 			reciprocal_measure
 * 			    scale / ( visual_angle + scale )
 * 			linear_measure
 * 			    1 - ( min(visual_angle,max_hazard ) / max_hazard )
 *
 * scale_parameter:	Gaussian_sigma, reciprocal_scale, or max_hazard as
 * 			appropriate for measurement type
 *
 * visualization_type:	red_gray_type
 * 			    redish => visibility hazard
 * 			    grayish => probably OK
 * 			red_green_type
 * 			    redish => visibility hazard
 *			    greenish => probably OK
 *
 * mask:		TRUE => pixel should be marked in special way (ignored
 * 			if NULL).
 *
 * geometry_boundaries:	Added to displayed mask area if not NULL.
 *
 * hazard_average:	Average hazard visibility over geometry boundary
 * 			elements (returned value if not NULL).  1.0 indicates
 * 			high likelihood that boundary is visible.
 */
{
    double		reciprocal_scale = -1.0;
    			/* 1 - scale / ( visual_angle + scale ) */
    double		max_hazard = -1.0;
    			/* min(visual_angle,max_hazard ) / max_hazard */
    double		Gaussian_sigma = -1.0;
    			/* 1 - exp ( -0.5 * ( x / Gaussian_sigma ) ^ 2 ) */
    int			row, col;
    double		hazard_level;
    DeVAS_float_image	*hazards_thickened;
    DeVAS_gray_image	*geometry_thickened = NULL;
    DeVAS_RGB_image	*visualization;
    DeVAS_RGB		black;
    DeVAS_RGBf		mask_color_f;
    DeVAS_RGB		mask_color;
    DeVAS_RGBf		geometry_color_f;
    DeVAS_RGB		geometry_color;
    double		sum;
    unsigned int	count;

    if ( mask != NULL ) {
	if ( !DeVAS_image_samesize ( hazards, mask ) ) {
	    fprintf ( stderr,
		    "visualize_hazards: hazards and mask size mismatch!" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}
    }

    if ( geometry_boundaries != NULL ) {
	if ( !DeVAS_image_samesize ( hazards, *geometry_boundaries ) ) {
	    fprintf ( stderr,
	"visualize_hazards: hazards and geometry_boundaries size mismatch!" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}
    }

    if ( ( visualization_type != red_gray_type ) &&
	    ( visualization_type != red_green_type ) &&
	    ( visualization_type != gray_cyan_type ) ) {
	fprintf ( stderr, "visualize_hazards: invalid visualization_type!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    switch ( measurement_type ) {
	case reciprocal_measure:
	    reciprocal_scale = scale_parameter;
	    break;

	case linear_measure:
	    max_hazard = scale_parameter;
	    break;

	case Gaussian_measure:
	    Gaussian_sigma = scale_parameter;
	    break;

	default:
	    fprintf ( stderr,
		    "visualize_hazards: invalid measurement_type!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
    }

    /* make things easier to see */
    hazards_thickened = expand_3x_f ( hazards );
    if ( geometry_boundaries != NULL ) {
	geometry_thickened = expand_3x_i ( *geometry_boundaries );
    }

    visualization = DeVAS_RGB_image_new ( DeVAS_image_n_rows ( hazards ),
	    DeVAS_image_n_cols ( hazards ) );

    black.red = black.green = black.blue = 0;

    if ( mask != NULL ) {
	mask_color_f.red = COLOR_MASK_RED;
	mask_color_f.green = COLOR_MASK_GREEN;
	mask_color_f.blue = COLOR_MASK_BLUE;

	mask_color = RGBf_to_RGB ( mask_color_f );
    }

    if ( geometry_boundaries != NULL ) {
	geometry_color_f.red = COLOR_GEOMETRY_RED;
	geometry_color_f.green = COLOR_GEOMETRY_GREEN;
	geometry_color_f.blue = COLOR_GEOMETRY_BLUE;

	geometry_color = RGBf_to_RGB ( geometry_color_f );
    }

    for ( row = 0; row < DeVAS_image_n_rows ( hazards ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( hazards ); col++ ) {

	    if ( ( ROI != NULL ) && ! DeVAS_image_data ( ROI, row, col ) ) {
		DeVAS_image_data ( visualization, row, col ) = black;
		continue;
	    }

	    if ( ( mask != NULL ) && ( DeVAS_image_data ( mask, row, col ) ) ) {
		if ( ( geometry_boundaries != NULL ) &&
		      ( DeVAS_image_data ( geometry_thickened, row, col ) ) ) {
		    DeVAS_image_data ( visualization, row, col ) =
			geometry_color;
		} else {
		    DeVAS_image_data ( visualization, row, col ) = mask_color;
		}
		continue;
	    }

	    if ( DeVAS_image_data ( hazards_thickened, row, col ) >= 0.0 ) {
		/* geometry edge that should be color coded */

		switch ( measurement_type ) {

		    case reciprocal_measure:
			hazard_level = 1.0 - ( reciprocal_scale /
			    ( DeVAS_image_data ( hazards_thickened, row, col ) +
				  reciprocal_scale ) );
			break;

		    case linear_measure:
			hazard_level =
			    fmin ( DeVAS_image_data ( hazards_thickened, row,
					col ), max_hazard ) / max_hazard;
			break;

		    case Gaussian_measure:
			hazard_level = 1.0 -
			    exp ( -0.5 *
				    ( SQ ( DeVAS_image_data ( hazards_thickened,
					    row, col ) / Gaussian_sigma ) ) );
			break;

		    default:
			fprintf ( stderr,
				"visualize_hazards: internal error!\n" );
	    		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
			exit ( EXIT_FAILURE );

		}

		DeVAS_image_data ( visualization, row, col ) =
		    color_hazard_level ( hazard_level, visualization_type );

	    } else {
		/* not a  geometry edge */

		DeVAS_image_data ( visualization, row, col ) = black;
	    }

	}
    }

    if ( hazard_average != NULL ) {
	/*
	 * Need to do this separately since we don't want the thickened values.
	 */

	sum = 0.0;
	count = 0;
	for ( row = 0; row < DeVAS_image_n_rows ( hazards ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( hazards ); col++ ) {
		if ( ( DeVAS_image_data ( hazards, row, col ) ==
			    HAZARD_NO_EDGE ) ||
			( ( mask != NULL ) &&
			  ( DeVAS_image_data ( mask, row, col ) ) ) ||
			( ( ROI != NULL ) &&
			  ( ! DeVAS_image_data ( ROI, row, col ) ) ) ) {
		    continue;
		}

		/*
		 * For Hazard Visibility Score, high values are good, low
		 * values are bad.  (This is opposite the conventions for
		 * hazard visualization.)
		 */

		switch ( measurement_type ) {

		    case reciprocal_measure:
			sum += reciprocal_scale /
			    ( DeVAS_image_data ( hazards, row, col ) +
			      reciprocal_scale );
			break;

		    case linear_measure:
			sum +=
			    1.0 - ( fmin ( DeVAS_image_data ( hazards, row,
					    col ), max_hazard ) / max_hazard );
			break;

		    case Gaussian_measure:
			sum +=
			    exp ( -0.5 * ( SQ ( DeVAS_image_data ( hazards,
					    row, col ) / Gaussian_sigma ) ) );
			break;

		    default:
			fprintf ( stderr,
				"visualize_hazards: internal error!\n" );
			DeVAS_print_file_lineno ( __FILE__, __LINE__ );

		}

		count++;
	    }
	}

	*hazard_average = sum / (double) count;
    }

    DeVAS_float_image_delete ( hazards_thickened );
    if ( geometry_thickened != NULL ) {
	DeVAS_gray_image_delete ( geometry_thickened );
    }

    return ( visualization );
}

static DeVAS_RGB
color_hazard_level ( double hazard_level,
	Visualization_type visualization_type )
/*
 * Blend colors as defined by visualization_type, based on value of
 * hazard_level (in range [0.0 - 1.0]).  The larger the value, the
 * greater the potential hazard.
 */
{
    DeVAS_RGBf	color_mixed;
    DeVAS_RGB	vis_color;

    switch ( visualization_type ) {

	case red_gray_type:
	    color_mixed.red = ( hazard_level * COLOR_MAX_RED ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_RED_RO );
	    color_mixed.green = ( hazard_level * COLOR_MAX_GREEN ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_GREEN_RO );
	    color_mixed.blue = ( hazard_level * COLOR_MAX_BLUE ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_BLUE_RO );
	    vis_color = RGBf_to_RGB ( color_mixed );

	    break;

	case red_green_type:

	    color_mixed.red = ( hazard_level * COLOR_MAX_RED ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_RED_RG );
	    color_mixed.green = ( hazard_level * COLOR_MAX_GREEN ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_GREEN_RG );
	    color_mixed.blue = ( hazard_level * COLOR_MAX_BLUE ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_BLUE_RG );
	    vis_color = RGBf_to_RGB ( color_mixed );

	    break;

	case gray_cyan_type:

	    color_mixed.red = ( hazard_level * COLOR_MAX_RED_FP ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_RED_FP );
	    color_mixed.green = ( hazard_level * COLOR_MAX_GREEN_FP ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_GREEN_FP );
	    color_mixed.blue = ( hazard_level * COLOR_MAX_BLUE_FP ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_BLUE_FP );
	    vis_color = RGBf_to_RGB ( color_mixed );

	    break;

	default:
	    fprintf ( stderr,
		    "visualize_hazards: invalid visualization_type!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );

	    break;
    }

    return ( vis_color );
}

static DeVAS_float_image *
expand_3x_f ( DeVAS_float_image *image )
/*
 * 3x increase in size to make markings more visible.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    int			i, j;
    float		local_max;
    DeVAS_float_image	*new_image;

    n_rows = DeVAS_image_n_rows ( image );
    n_cols = DeVAS_image_n_cols ( image );

    new_image = DeVAS_float_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	DeVAS_image_data ( new_image, row, 0 ) = HAZARD_NO_EDGE;
	DeVAS_image_data ( new_image, row, n_cols - 1 ) = HAZARD_NO_EDGE;
    }

    for ( col = 1; col < n_cols - 1; col++ ) {
	DeVAS_image_data ( new_image, 0, col ) = HAZARD_NO_EDGE;
	DeVAS_image_data ( new_image, n_rows - 1, col ) = HAZARD_NO_EDGE;
    }

    for ( row = 1; row < DeVAS_image_n_rows ( image ) - 1; row++ ) {
	for ( col = 1; col < DeVAS_image_n_cols ( image ) - 1; col++ ) {
	    local_max = HAZARD_NO_EDGE;
	    for ( i = -1; i <= 1; i++ ) {
		for ( j = -1; j <= 1; j++ ) {
		    local_max = fmax ( local_max,
			    DeVAS_image_data ( image, row+i, col+j ) );
		}
	    }

	    DeVAS_image_data ( new_image, row, col ) = local_max;
	}
    }

    return ( new_image );
}

static DeVAS_gray_image *
expand_3x_i ( DeVAS_gray_image *image )
/*
 * 3x increase in size to make markings more visible.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    int			i, j;
    float		local_max;
    DeVAS_gray_image	*new_image;

    n_rows = DeVAS_image_n_rows ( image );
    n_cols = DeVAS_image_n_cols ( image );

    new_image = DeVAS_gray_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	DeVAS_image_data ( new_image, row, 0 ) = HAZARD_NO_EDGE_GRAY;
	DeVAS_image_data ( new_image, row, n_cols - 1 ) = HAZARD_NO_EDGE_GRAY;
    }

    for ( col = 1; col < n_cols - 1; col++ ) {
	DeVAS_image_data ( new_image, 0, col ) = HAZARD_NO_EDGE_GRAY;
	DeVAS_image_data ( new_image, n_rows - 1, col ) = HAZARD_NO_EDGE_GRAY;
    }

    for ( row = 1; row < DeVAS_image_n_rows ( image ) - 1; row++ ) {
	for ( col = 1; col < DeVAS_image_n_cols ( image ) - 1; col++ ) {
	    local_max = HAZARD_NO_EDGE_GRAY;
	    for ( i = -1; i <= 1; i++ ) {
		for ( j = -1; j <= 1; j++ ) {
		    local_max = imax ( local_max,
			    DeVAS_image_data ( image, row+i, col+j ) );
		}
	    }

	    DeVAS_image_data ( new_image, row, col ) = local_max;
	}
    }

    return ( new_image );
}
