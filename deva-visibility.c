#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "deva-image.h"
#include "deva-utils.h"
#include "deva-visibility.h"
#include "deva-canny.h"
#include "geometry-discontinuities.h"
#include "dilate.h"
#include "DEVA-png.h"

/*******************
#define	DEBUG_HAZARDS		"deva-visibility-debug-hazards.png"
 *******************/

#if defined(DEBUG_HAZARDS)
#include "visualize-hazards.h"
#endif

static DEVA_float_image	*DEVA_image_xyY_to_Y ( DEVA_xyY_image *xyY_image );
static DEVA_float_image *compute_hazards ( DEVA_gray_image *geometry_boundaries,
			    DEVA_float_image *edge_distance,
			    double degrees_per_pixel );
static void		make_visible ( DEVA_gray_image *boundaries );

DEVA_float_image *
deva_visibility ( DEVA_xyY_image *filtered_image, DEVA_coordinates *coordinates,
       	DEVA_XYZ_image *xyz, DEVA_float_image *dist, DEVA_XYZ_image *nor,
	int position_patch_size, int orientation_patch_size,
	int position_threshold, int orientation_threshold,
	char *luminance_boundaries_file_name,
	char *geometry_boundaries_file_name )
{
    DEVA_float_image	*filtered_image_luminance;
    DEVA_gray_image	*luminance_boundaries;
    DEVA_gray_image	*geometry_boundaries;
    DEVA_float_image	*luminance_edge_distance;
    DEVA_float_image	*hazards_image;
    double		degrees_per_pixel;

    filtered_image_luminance = DEVA_image_xyY_to_Y ( filtered_image );

    luminance_boundaries = deva_canny_autothresh ( filtered_image_luminance,
	    CANNY_ST_DEV , NULL /* magnitude_p */, NULL /* orientation_p */ );

    if ( luminance_boundaries_file_name != NULL ) {
	make_visible ( luminance_boundaries );
	DEVA_gray_image_to_filename_png ( luminance_boundaries_file_name,
		luminance_boundaries );
    }

    DEVA_float_image_delete ( filtered_image_luminance );

    geometry_boundaries =
	geometry_discontinuities ( coordinates, xyz, dist, nor,
		position_patch_size, orientation_patch_size, position_threshold,
		orientation_threshold );

    if ( geometry_boundaries_file_name != NULL ) {
	make_visible ( geometry_boundaries );
	DEVA_gray_image_to_filename_png ( geometry_boundaries_file_name,
		geometry_boundaries );
    }

    if ( !DEVA_image_samesize ( luminance_boundaries, geometry_boundaries ) ) {
	fprintf ( stderr, "deva_visibility: incorrect geometry file size!\n" );
	exit ( EXIT_FAILURE );
    }

    luminance_edge_distance = dt_euclid_sq ( luminance_boundaries );

    degrees_per_pixel = fmax ( DEVA_image_view ( filtered_image ) . vert,
	    DEVA_image_view ( filtered_image ) . horiz ) /
	((double) imax ( DEVA_image_n_rows ( filtered_image ),
	    DEVA_image_n_cols ( filtered_image ) ) );

    hazards_image = compute_hazards ( geometry_boundaries,
	    luminance_edge_distance, degrees_per_pixel );

    DEVA_gray_image_delete ( geometry_boundaries );
    DEVA_gray_image_delete ( luminance_boundaries );
    DEVA_float_image_delete ( luminance_edge_distance );

    return ( hazards_image );
}

static DEVA_float_image *
DEVA_image_xyY_to_Y ( DEVA_xyY_image *xyY_image )
{
    DEVA_float_image	*Y_image;
    int			row, col;
    int			n_rows, n_cols;

    n_rows = DEVA_image_n_rows ( xyY_image );
    n_cols = DEVA_image_n_cols ( xyY_image );

    Y_image = DEVA_float_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( Y_image, row, col ) =
		DEVA_image_data ( xyY_image, row, col ) .  Y;
	}
    }

    return ( Y_image );
}

static DEVA_float_image *
compute_hazards ( DEVA_gray_image *geometry_boundaries,
	DEVA_float_image *edge_distance, double degrees_per_pixel )
{
    int			row, col;
    DEVA_float_image	*hazards;

    if ( ! DEVA_image_samesize ( geometry_boundaries, edge_distance ) ) {
	fprintf ( stderr, "compute_hazards: argument size mismatch!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( degrees_per_pixel <= 0.0 ) {
	fprintf ( stderr, "compute_hazards: invalid degrees_per_pixel (%f)\n",
		degrees_per_pixel );
	exit ( EXIT_FAILURE );
    }

    hazards = DEVA_float_image_new ( DEVA_image_n_rows ( geometry_boundaries ),
	    DEVA_image_n_cols ( geometry_boundaries ) );

    for ( row = 0; row < DEVA_image_n_rows ( geometry_boundaries ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( geometry_boundaries );
		col++ ) {
	    if ( DEVA_image_data ( geometry_boundaries, row, col ) ) {
		DEVA_image_data ( hazards, row, col ) = degrees_per_pixel *
		    sqrt ( DEVA_image_data ( edge_distance, row, col ) );
	    } else {
		DEVA_image_data ( hazards, row, col ) = HAZARD_NO_EDGE;
	    }
	}
    }

#if defined(DEBUG_HAZARDS)
    DEVA_gray_image *display_hazards;
    double	    max_hazard;

    max_hazard = -2.0;
    for ( row = 0; row < DEVA_image_n_rows ( geometry_boundaries ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( geometry_boundaries );
		col++ ) {
	    max_hazard =
		fmax ( max_hazard, DEVA_image_data ( hazards, row, col ) );
	}
    }
    fprintf ( stderr, "max_hazard = %f\n", max_hazard );

    display_hazards =
	DEVA_gray_image_new ( DEVA_image_n_rows ( geometry_boundaries ),
		DEVA_image_n_cols ( geometry_boundaries ) );
    for ( row = 0; row < DEVA_image_n_rows ( geometry_boundaries ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( geometry_boundaries );
		col++ ) {
	    if ( DEVA_image_data ( hazards, row, col ) == HAZARD_NO_EDGE ) {
		DEVA_image_data ( display_hazards, row, col ) = 0;
	    } else {
		DEVA_image_data ( display_hazards, row, col ) =
		    127.9 *
		    fmin ( DEVA_image_data ( hazards, row, col ), MAX_HAZARD );
	    }
	}
    }

    DEVA_gray_image_to_filename_png ( DEBUG_HAZARDS, display_hazards );

#endif

    return ( hazards );
}

static void
make_visible ( DEVA_gray_image *boundaries )
/*
 * Change 0,1 values in a boolian file to 0,255.  This preserves TRUE/FALSE,
 * while making the file directly displayable.
 */
{
    int     row, col;

    for ( row = 0; row < DEVA_image_n_rows ( boundaries ); row ++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( boundaries );
		col ++ ) {
	    if ( DEVA_image_data ( boundaries, row,
			col ) ) {
		DEVA_image_data ( boundaries, row, col ) = 255;
	    }
	}
    }
}
