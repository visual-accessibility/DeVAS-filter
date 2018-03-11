#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "visualize-hazards.h"
#include "deva-visibility.h"
#include "deva-image.h"
#include "deva-license.h"

static DEVA_RGB		color_hazard_level ( double hazard_level,
			    int visualization_type );
static DEVA_float_image	*expand_3x ( DEVA_float_image *image );

DEVA_RGB_image *
visualize_hazards ( double max_hazard, DEVA_float_image *hazards,
       int visualization_type )
/*
 * Make displayable file showing predicted geometry discontinuities that are
 * not visible at specified level of low vision. 
 *
 * max_hazard:		Visual angle corresponding to maximum displayable
 *			hazard.  All hazard values at or above this level as
 *			displayed with the same red shade.
 *
 * hazards:		Visual angle from geometry boundaries to nearest
 *			luminance boundary.
 *
 * visualization_type:	DEVA_VIS_HAZ_RED_ONLY
 * 				redish => visibility hazard
 * 				grayish => probably OK
 * 			DEVA_VIS_HAZ_RED_GREEN
 * 				redish => visibility hazard
 *				greenish => probably OK
 */
{
    int			row, col;
    double		hazard_level;
    DEVA_float_image	*hazards_thickened;
    DEVA_RGB_image	*visualization;
    DEVA_RGB		black;

    if ( ( visualization_type != DEVA_VIS_HAZ_RED_ONLY ) &&
	    ( visualization_type != DEVA_VIS_HAZ_RED_GREEN ) ) {
	fprintf ( stderr, "visualize_hazards: invalid visualization_type!\n" );
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

		if ( DEVA_image_data ( hazards_thickened, row, col ) >=
						max_hazard ) {
		    /* normalize based on max_hazard */
		    hazard_level = 1.0;
		} else {
		    hazard_level =
			DEVA_image_data ( hazards_thickened, row, col ) /
				max_hazard;
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
color_hazard_level ( double hazard_level, int visualization_type )
/*
 * Blend colors as defined by visualization_type, based on value of
 * hazard_level (in range [0.0 - 1.0]).
 */
{
    DEVA_RGBf	color_mixed;
    DEVA_RGB	vis_color;

    switch ( visualization_type ) {

	case DEVA_VIS_HAZ_RED_ONLY:
	    color_mixed.red = ( hazard_level * COLOR_MAX_RED ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_RED_RO );
	    color_mixed.green = ( hazard_level * COLOR_MAX_GREEN ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_GREEN_RO );
	    color_mixed.blue = ( hazard_level * COLOR_MAX_BLUE ) +
		( ( 1.0 - hazard_level ) * COLOR_MIN_BLUE_RO );
	    vis_color = RGBf_to_RGB ( color_mixed );

	    break;

	case DEVA_VIS_HAZ_RED_GREEN:

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
