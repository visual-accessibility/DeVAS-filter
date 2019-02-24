#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "deva-image.h"
#include "deva-utils.h"
#include "deva-visibility.h"
#include "visualize-hazards.h"
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
static DEVA_float_image *compute_hazards ( DEVA_gray_image *standard_boundaries,
			    DEVA_float_image *comparison_distance,
			    double degrees_per_pixel );

DEVA_float_image *
deva_visibility ( DEVA_xyY_image *filtered_image, DEVA_coordinates *coordinates,
       	DEVA_XYZ_image *xyz, DEVA_float_image *dist, DEVA_XYZ_image *nor,
	int position_patch_size, int orientation_patch_size,
	int position_threshold, int orientation_threshold,
	DEVA_gray_image **luminance_boundaries,
	DEVA_gray_image **geometry_boundaries,
	DEVA_float_image **false_positives )

/*
 * Computes visual angle from geometry boundaries to nearest luminance
 * boundary.
 *
 * filtered_image:	xyY image that is the result of the deva-filter
 * 			process.  This routine will extract luminance
 * 			boundaries by applying the Canny edge detector to
 * 			the Y channel of this image.
 *
 * coordinates:		A file specifying units of distance, coordinate
 * 			system orientationk and viewpoint for geometry
 * 			files.  Created by the make-coordinates-file program.
 * 			Note that currently all geometry files need to use
 * 			the same units.
 *
 * xyz:			Position data for every visible surface point,
 *			formatted as a Radiance ASCII file.
 *
 * dist:		Distance from viewpoint to every visible surface point,
 *			formatted as a Radiance ASCII file.  Not used in the
 *			current version, but included in the API for possible
 *			future use.
 *
 * nor:			Unit normal vector for every visible surface point,
 *			formatted as a Radiance ASCII file.
 *
 * position_path_size:	xyz discontinuities evaluated over a
 *			position_path_size x position_path_size region
 *			Units are pixels.
 *
 * position_path_size:	nor discontinuities evaluated over a
 *			orientation_path_size x orientation_path_size region
 *			Units are pixels.
 *
 * position_threshold:	Threshold for xyz discontinuities. Units are cm.
 *
 * orientation_threshold: Threshold for nor discontinuities. Units are degrees.
 *
 * luminance_boundaries:
 * 			Pointer to DEVA_image object in calling program.
 *
 * geometry_boundaries:	Pointer to DEVA_image object in calling program.
 *
 * false_positives:	Pointer to DEVA_image object in calling program.
 * 			Ignored if NULL.
 */
{
    DEVA_float_image	*filtered_image_luminance;
    DEVA_float_image	*luminance_edge_distance;
    DEVA_float_image	*geometry_edge_distance = NULL;
    DEVA_float_image	*hazards_image;
    double		degrees_per_pixel;

    /* pull out luminance channel from deva-filtered output image */
    filtered_image_luminance = DEVA_image_xyY_to_Y ( filtered_image );

    /* find luminance boudaries using (slightly) modified Canny edge detector */
    *luminance_boundaries = deva_canny_autothresh ( filtered_image_luminance,
	    CANNY_ST_DEV , NULL /* magnitude_p */, NULL /* orientation_p */ );

    /* clean up */
    DEVA_float_image_delete ( filtered_image_luminance );

    /* find geometric boundaries */
    *geometry_boundaries =
	geometry_discontinuities ( coordinates, xyz, dist, nor,
		position_patch_size, orientation_patch_size, position_threshold,
		orientation_threshold );

    /* consistency check */
    if ( !DEVA_image_samesize ( *luminance_boundaries,
		*geometry_boundaries ) ) {
	fprintf ( stderr, "deva_visibility: incorrect geometry file size!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* squared-distance transform */
    luminance_edge_distance = dt_euclid_sq ( *luminance_boundaries );

    /* conversion factor */
    degrees_per_pixel = fmax ( DEVA_image_view ( filtered_image ) . vert,
	    DEVA_image_view ( filtered_image ) . horiz ) /
	((double) imax ( DEVA_image_n_rows ( filtered_image ),
	    DEVA_image_n_cols ( filtered_image ) ) );

    /*
     * Compute distance, represented as a visual angle, from each geometric
     * boundary pixel to nearest luminance boundary pixel.
     */
    hazards_image = compute_hazards ( *geometry_boundaries,
	    luminance_edge_distance, degrees_per_pixel );

    if ( false_positives != NULL ) {
	geometry_edge_distance = dt_euclid_sq ( *geometry_boundaries );
	*false_positives = compute_hazards ( *luminance_boundaries,
		geometry_edge_distance, degrees_per_pixel );
	DEVA_float_image_delete ( geometry_edge_distance );
    }

    /* clean up */
    DEVA_float_image_delete ( luminance_edge_distance );

    return ( hazards_image );
}

static DEVA_float_image *
DEVA_image_xyY_to_Y ( DEVA_xyY_image *xyY_image )
/*
 * Return Y (luminance) channel of an xyY image object.
 */
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
compute_hazards ( DEVA_gray_image *standard_boundaries,
	DEVA_float_image *comparison_distance, double degrees_per_pixel )
/*
 * Compute distance, represented as a visual angle, from each geometric
 * boundary pixel to nearest luminance boundary pixel.
 *
 * standard_boundaries:	TRUE => pixel is a geometric edge.
 *
 * comparison_distance:	Squared distance to nearest luminance edge.
 *
 * degrees_per_pixel:	Angle in degrees between two horizontally or vertically
 * 			adjacent pixel locations.
 *
 * returned value:	DEVA_float_image image object. Non-negative pixel
 * 			values give distance from geometry boundary at
 * 			pixel location to nearest luminance boundary element.
 * 			Negative pixel values indicate that there is no
 * 			geometry boundary at that location.
 */
{
    int			row, col;
    DEVA_float_image	*hazards;

    if ( ! DEVA_image_samesize ( standard_boundaries, comparison_distance ) ) {
	fprintf ( stderr, "compute_hazards: argument size mismatch!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( degrees_per_pixel <= 0.0 ) {
	fprintf ( stderr, "compute_hazards: invalid degrees_per_pixel (%f)\n",
		degrees_per_pixel );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    hazards = DEVA_float_image_new ( DEVA_image_n_rows ( standard_boundaries ),
	    DEVA_image_n_cols ( standard_boundaries ) );

    for ( row = 0; row < DEVA_image_n_rows ( standard_boundaries ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( standard_boundaries );
		col++ ) {
	    if ( DEVA_image_data ( standard_boundaries, row, col ) ) {
		DEVA_image_data ( hazards, row, col ) = degrees_per_pixel *
		    sqrt ( DEVA_image_data ( comparison_distance, row, col ) );
	    } else {
		DEVA_image_data ( hazards, row, col ) = HAZARD_NO_EDGE;
	    }
	}
    }

#if defined(DEBUG_HAZARDS)

    /*
     * Output displayable version of hazards data.
     */
    DEVA_gray_image *display_hazards;
    double	    max_hazard;

    max_hazard = -2.0;
    for ( row = 0; row < DEVA_image_n_rows ( standard_boundaries ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( standard_boundaries );
		col++ ) {
	    max_hazard =
		fmax ( max_hazard, DEVA_image_data ( hazards, row, col ) );
	}
    }
    fprintf ( stderr, "max_hazard = %f\n", max_hazard );

    display_hazards =
	DEVA_gray_image_new ( DEVA_image_n_rows ( standard_boundaries ),
		DEVA_image_n_cols ( standard_boundaries ) );
    for ( row = 0; row < DEVA_image_n_rows ( standard_boundaries ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( standard_boundaries );
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
