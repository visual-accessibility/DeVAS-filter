#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "visualize-hazards.h"
#include "deva-visibility.h"
#include "deva-image.h"
#include "deva-license.h"

#define	SQ(x)	((x) * (x))

static DEVA_RGB		color_hazard_level ( double hazard_level,
			     Visualization_type visualization_type );
static DEVA_float_image	*expand_3x ( DEVA_float_image *image );

DEVA_RGB_image *
visualize_hazards ( DEVA_float_image *hazards,
	Measurement_type measurement_type, double scale_parameter,
	Visualization_type visualization_type )
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
    DEVA_float_image	*hazards_thickened;
    DEVA_RGB_image	*visualization;
    DEVA_RGB		black;

    if ( ( visualization_type != red_gray_type ) &&
	    ( visualization_type != red_green_type ) ) {
	fprintf ( stderr, "visualize_hazards: invalid visualization_type!\n" );
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
	    exit ( EXIT_FAILURE );
    }


    /* make things easier to see */
    hazards_thickened = expand_3x ( hazards );

    visualization = DEVA_RGB_image_new ( DEVA_image_n_rows ( hazards ),
	    DEVA_image_n_cols ( hazards ) );

    black.red = black.green = black.blue = 0;

    for ( row = 0; row < DEVA_image_n_rows ( hazards ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( hazards ); col++ ) {
	    if ( DEVA_image_data ( hazards_thickened, row, col ) >= 0.0 ) {
		/* geometry edge that should be color coded */

		switch ( measurement_type ) {

		    case reciprocal_measure:
			hazard_level = 1.0 - ( reciprocal_scale /
			    ( DEVA_image_data ( hazards_thickened, row, col ) +
				  reciprocal_scale ) );
			break;

		    case linear_measure:
			hazard_level =
			    fmin ( DEVA_image_data ( hazards_thickened, row,
					col ), max_hazard ) / max_hazard;
			break;

		    case Gaussian_measure:
			hazard_level = 1.0 -
			    exp ( -0.5 *
				    ( SQ ( DEVA_image_data ( hazards_thickened,
					    row, col ) / Gaussian_sigma ) ) );
			break;

		    default:
			fprintf ( stderr,
				"visualize_hazards: internal error!\n" );
			exit ( EXIT_FAILURE );

		}

		DEVA_image_data ( visualization, row, col ) =
		    color_hazard_level ( hazard_level, visualization_type );

	    } else {
		/* not a  geometry edge */

		DEVA_image_data ( visualization, row, col ) = black;
	    }
	}
    }

    DEVA_float_image_delete ( hazards_thickened );

    return ( visualization );
}

static DEVA_RGB
color_hazard_level ( double hazard_level,
	Visualization_type visualization_type )
/*
 * Blend colors as defined by visualization_type, based on value of
 * hazard_level (in range [0.0 - 1.0]).  The larger the value, the
 * greater the potential hazard.
 */
{
    DEVA_RGBf	color_mixed;
    DEVA_RGB	vis_color;

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

	default:
	    fprintf ( stderr,
		    "visualize_hazards: invalid visualization_type!\n" );
	    exit ( EXIT_FAILURE );

	    break;
    }

    return ( vis_color );
}

static DEVA_float_image *
expand_3x ( DEVA_float_image *image )
/*
 * 3x increase in size to make markings more visible.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    int			i, j;
    float		local_max;
    DEVA_float_image	*new_image;

    n_rows = DEVA_image_n_rows ( image );
    n_cols = DEVA_image_n_cols ( image );

    new_image = DEVA_float_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	DEVA_image_data ( new_image, row, 0 ) = HAZARD_NO_EDGE;
	DEVA_image_data ( new_image, row, n_cols - 1 ) = HAZARD_NO_EDGE;
    }

    for ( col = 1; col < n_cols - 1; col++ ) {
	DEVA_image_data ( new_image, 0, col ) = HAZARD_NO_EDGE;
	DEVA_image_data ( new_image, n_rows - 1, col ) = HAZARD_NO_EDGE;
    }

    for ( row = 1; row < DEVA_image_n_rows ( image ) - 1; row++ ) {
	for ( col = 1; col < DEVA_image_n_cols ( image ) - 1; col++ ) {
	    local_max = HAZARD_NO_EDGE;
	    for ( i = -1; i <= 1; i++ ) {
		for ( j = -1; j <= 1; j++ ) {
		    local_max = fmax ( local_max,
			    DEVA_image_data ( image, row+i, col+j ) );
		}
	    }

	    DEVA_image_data ( new_image, row, col ) = local_max;
	}
    }

    return ( new_image );
}
