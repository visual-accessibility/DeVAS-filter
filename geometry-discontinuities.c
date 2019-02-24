/*
 * Find locations of rapid change in geometric structure.
 * Two types of discontinuities are found, one corresponding to occlusion
 * boundaries and the other corresponding to orientation changes ("creases").
 */

#include <stdlib.h>
#include <math.h>
#include "geometry-discontinuities.h"
#include "deva-utils.h"
#include "deva-image.h"
#include "read-geometry.h"
#include "directional-maxima.h"
#include "deva-license.h"	/* DEVA open source license */

/*******************
#define	DEBUG_POSITION		"deva-visibility-debug-position.png"
#define	DEBUG_ORIENTATION	"deva-visibility-debug-orientation.png"
#define	DEBUG_COMBINED		"deva-visibility-debug-combined.png"
 *******************/

#if defined(DEBUG_POSITION)||defined(DEBUG_ORIENTATION)||defined(DEBUG_COMBINED)
#include "DEVA-png.h"
#endif

static DEVA_float_image	*compute_position_deviation ( int position_patch_size,
			    DEVA_XYZ_image *position,
			    DEVA_XYZ_image *surface_normal );
static DEVA_float_image	*compute_orientation_deviation (
			    int orientation_patch_size,
			    DEVA_XYZ_image *surface_normal );
#ifdef DEVA_CONVEX	/* needed to distinguish between convex and concave */
			/* creases */
static double		DEVA_distance_point_plane ( DEVA_XYZ point,
			    DEVA_XYZ surface_normal, DEVA_XYZ point_on_plane );
#endif	/* DEVA_CONVEX */
static DEVA_XYZ		v3d_subtract ( DEVA_XYZ v1, DEVA_XYZ v2 );
static double		v3d_dotprod ( DEVA_XYZ v1, DEVA_XYZ v2 );
static DEVA_gray_image	*DEVA_gray_or ( DEVA_gray_image *i1,
			    DEVA_gray_image *i2 );
#if defined(DEBUG_POSITION)||defined(DEBUG_ORIENTATION)||defined(DEBUG_COMBINED)
static void		make_visible ( DEVA_gray_image *boundaries );
#endif

DEVA_gray_image *
geometry_discontinuities ( DEVA_coordinates *coordinates, DEVA_XYZ_image *xyz,
	DEVA_float_image *dist, DEVA_XYZ_image *nor,
	int position_patch_size, int orientation_patch_size,
	int position_threshold, int orientation_threshold )
/*
 * Find geometric boundaries using two tests, one which looks for
 * discontinuities in position due to occlusion and the other which look
 * for discontinuities in orientation due to surface creases.
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
 * Returned value:	Boolean image object, TRUE if pixel corresponds to
 * 			a geometric boundary.
 */
{
    int			min_image_size;
    DEVA_float_image	*position_deviations;	    /* position difference */
    DEVA_gray_image	*position_discontinuities;  /* directional local max */
    						    /* of pos differences */
    DEVA_float_image	*orientation_deviations;    /* orientation diffs */
    DEVA_gray_image	*orientation_discontinuities;/* directional local max */
    						     /* of pos differences */
    DEVA_gray_image	*combined_discontinuities;  /* union of positional */
    						    /* and orientational */
    						    /* discontinuities */

    /* sanity check of arguments */

    if ( !DEVA_image_samesize ( xyz, dist ) ||
	    !DEVA_image_samesize ( xyz, nor ) ) {
	fprintf ( stderr,
		"geometry_discontinuities: geometry image size mismatch!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( position_patch_size < 3 ) {
	fprintf ( stderr,
		"geometry_discontinuities: position_patch_size must >= 3!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( orientation_patch_size < 3 ) {
	fprintf ( stderr,
	    "geometry_discontinuities: orientation_patch_size must >= 3!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( ( position_patch_size % 2 ) != 1 ) {
	fprintf ( stderr,
	    "geometry_discontinuities: position_patch_size must be odd!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( ( orientation_patch_size % 2 ) != 1 ) {
	fprintf ( stderr,
	    "geometry_discontinuities: orientation_patch_size must be odd!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( DEVA_image_n_rows ( xyz ) < DEVA_image_n_cols ( xyz ) ) {
	min_image_size = DEVA_image_n_rows ( xyz );
    } else {
	min_image_size = DEVA_image_n_cols ( xyz );
    }

    if ( position_patch_size > min_image_size ) {
	fprintf ( stderr,
      "geometry_discontinuities: position_patch_size exceeds data size!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( orientation_patch_size > min_image_size ) {
	fprintf ( stderr,
      "geometry_discontinuities: orientation_patch_size exceeds data size!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* compute position deviations, directional local maxima of same */
    position_deviations = compute_position_deviation ( position_patch_size,
	    xyz, nor );
    position_discontinuities = find_directional_maxima ( DMAX_PATCH_SIZE,
	    position_threshold, position_deviations );

#ifdef DEBUG_POSITION
    /* output viewable indication of position discontinuities */
    make_visible ( position_discontinuities );
    DEVA_gray_image_to_filename_png ( DEBUG_POSITION,
	    position_discontinuities );
#endif	/* DEBUG_POSITION */

    /* compute orientation deviations, directional local maxima of same */
    orientation_deviations =
	compute_orientation_deviation ( orientation_patch_size, nor );
    orientation_discontinuities = find_directional_maxima ( DMAX_PATCH_SIZE,
	    orientation_threshold, orientation_deviations );

#ifdef DEBUG_ORIENTATION
    /* output viewable indication of orientation discontinuities */
    make_visible ( orientation_discontinuities );
    DEVA_gray_image_to_filename_png ( DEBUG_ORIENTATION,
	    orientation_discontinuities );
#endif	/* DEBUG_ORIENTATION */

    /* compute union of two types of discontinuities */
    combined_discontinuities = DEVA_gray_or ( position_discontinuities,
	    orientation_discontinuities );

#ifdef DEBUG_COMBINED
    /* output viewable indication of combined discontinuities */
    make_visible ( combined_discontinuities );
    DEVA_gray_image_to_filename_png ( DEBUG_COMBINED,
	    combined_discontinuities );
#endif	/* DEBUG_COMBINED */

    /* clean up */
    DEVA_float_image_delete ( position_deviations );
    DEVA_gray_image_delete ( position_discontinuities );
    DEVA_float_image_delete ( orientation_deviations );
    DEVA_gray_image_delete ( orientation_discontinuities );

    return ( combined_discontinuities );
}

static DEVA_float_image *
compute_position_deviation ( int position_patch_size, DEVA_XYZ_image *position,
	DEVA_XYZ_image *surface_normal )
/*
 * Measure is based on average over patch of distance from pixel positions
 * to a plane going through the center pixel and oriented perpendicularly
 * to the surface normal of the center pixel.  Only positions behind this
 * plane from the perspective of the viewpoint are considered.  As a result,
 * the measure has high values for pixels at the boundary of occluding
 * surfaces.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    DEVA_float_image	*position_deviation;
    int			half_patch_size;	/* excluding center */
    int			i, j;
    double		deviation;
    double		total_deviation;
    DEVA_XYZ		center_position;
    DEVA_XYZ		center_normal;
    DEVA_XYZ		deviation_vector;

    n_rows = DEVA_image_n_rows ( position );
    n_cols = DEVA_image_n_cols ( position );

    if ( !DEVA_image_samesize ( position, surface_normal ) ) {
	fprintf ( stderr,
		"compute_position_deviation: image sizes don't match!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );	/* error return */
    }

    if ( position_patch_size > imin ( n_rows, n_cols ) ) {
	fprintf ( stderr,
		"compute_position_deviation: patch size exceeds data size!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );	/* error return */
    }

    if ( ( position_patch_size % 2 ) != 1 ) {
	fprintf ( stderr,
	    "compute_position_deviation: patch-size must be odd!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );	/* error return */
    }

    position_deviation = DEVA_float_image_new ( n_rows, n_cols );

    DEVA_float_image_setvalue ( position_deviation, 0.0 );
    					/* initialize to all 0.0 */

    half_patch_size = ( position_patch_size - 1 ) / 2;

    for ( row = half_patch_size; row < ( n_rows - half_patch_size ); row++ ) {
	for ( col = half_patch_size; col < ( n_cols - half_patch_size );
		col++ ) {
	    center_position = DEVA_image_data ( position, row, col );
	    center_normal = DEVA_image_data ( surface_normal, row, col );

	    total_deviation = 0.0;

	    for ( i = -half_patch_size; i <= half_patch_size; i++ ) {
		for ( j = -half_patch_size; j <= half_patch_size; j++ ) {
		    /*
		     * Generate vector from patch point to center position.
		     */
		    deviation_vector =
			v3d_subtract ( DEVA_image_data ( position, row + i,
				    col + j ), center_position );

		    /*
		     * Project onto unit normal vector of patch center, which
		     * yields minimum difference from patch point to plane
		     * going through center position and oriented
		     * perpendicularly to normal vector of patch center.
		     */
		    deviation = v3d_dotprod ( center_normal,
			    deviation_vector );

		    total_deviation += deviation;
		}
	    }

	    /*
	     * Only consider potential boundaries where non-center surface
	     * seems to be behind center pixel.
	     *
	     * Normalize by number of elements in patch on "far" side of 
	     * potential boundary.
	     */

	    if ( total_deviation < 0.0 ) {
		/* behind center point */
		DEVA_image_data ( position_deviation, row, col ) =
		    -total_deviation /
		    	((double) ( half_patch_size * position_patch_size ) );
			/* normalization assumes occluding surface is flat */
	    } else {
		DEVA_image_data ( position_deviation, row, col ) = 0.0;
	    }
	}
    }

    return ( position_deviation );
}

static DEVA_float_image *
compute_orientation_deviation ( int orientation_patch_size,
	DEVA_XYZ_image *surface_normal )
/*
 * Measure is based on average angular distance of orientation vectors at
 * equal but opposite distances from the center of the patch.  One consequence
 * of this is that the detected orientation edges lie between two adjacent
 * patch pixels, with is different from the detected position differences.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    DEVA_float_image	*orientation_deviation;
#ifdef SMOOTH_ORIENTATION
    /* define this is smoothing of orientation vectors is requested */
    DEVA_float_image	*smoothed_orientation_deviation;
#endif	/* SMOOTH_ORIENTATION */
    int			half_patch_size;	/* excluding center */
    int			i, j;
    double		total_deviation;
    double		deviation_angle;

    n_rows = DEVA_image_n_rows ( surface_normal );
    n_cols = DEVA_image_n_cols ( surface_normal );

    if ( orientation_patch_size > imin ( n_rows, n_cols ) ) {
	fprintf ( stderr,
		"compute_position_deviation: patch size exceeds data size!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );  /* error return */
    }

    if ( ( orientation_patch_size % 2 ) != 1 ) {
	fprintf ( stderr,
		"compute_position_deviation: patch-size must be odd!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );  /* error return */
    }

    orientation_deviation = DEVA_float_image_new ( n_rows, n_cols );
    DEVA_float_image_setvalue ( orientation_deviation, 0.0 );

    half_patch_size = ( orientation_patch_size - 1 ) / 2;

    for ( row = half_patch_size; row < ( n_rows - half_patch_size ); row++ ) {
	for ( col = half_patch_size; col < ( n_cols - half_patch_size );
		col++ ) {

	    total_deviation = 0.0;

	    for ( i = -half_patch_size; i < 0; i++ ) {
		for ( j = -half_patch_size; j <= half_patch_size; j++ ) {
		    /*
		     * Get average angular distance of pairs of point on
		     * opposite sides of center by computing arccosine of
		     * dot product of pairs of surface normals.
		     *
		     * Need fmin to avoid problems with acos due to precision 
		     * limitations in dot produce. (May need to deal with
		     * lower bound as well.)
		     */
		    deviation_angle = radian2degree ( acos ( fmin (
			v3d_dotprod (
			    DEVA_image_data ( surface_normal, row + i,
				    col + j ),
			    DEVA_image_data ( surface_normal, row - i,
				    col - j ) ),
			1.0 ) ) );

		    total_deviation += deviation_angle;
		}
	    }

	    for ( j = -half_patch_size; j < 0; j++ ) {
		deviation_angle = radian2degree ( acos ( fmin (
			v3d_dotprod (
			    DEVA_image_data ( surface_normal, row, col + j ),
			    DEVA_image_data ( surface_normal, row, col - j ) ),
			1.0 ) ) );

		total_deviation += deviation_angle;
	    }

	    DEVA_image_data ( orientation_deviation, row, col ) =
		total_deviation /
		    ((double) ( ( orientation_patch_size + 1 ) *
			half_patch_size ) );
	}
    }

#ifdef SMOOTH_ORIENTATION

    /*
     * If SMOOTH_ORIENTATION is defined, some smoothing is done before
     * looking for directional local maxima
     */

    smoothed_orientation_deviation = gblur_3x3 ( orientation_deviation );
    DEVA_float_image_delete ( orientation_deviation );

    return ( smoothed_orientation_deviation );
#else
    return ( orientation_deviation );
#endif	/* SMOOTH_ORIENTATION */
}

#ifdef DEVA_CONVEX
/*
 * Used to differentiate between convex and concave corners.
 */
static double
DEVA_distance_point_plane ( DEVA_XYZ point, DEVA_XYZ surface_normal,
	DEVA_XYZ point_on_plane )
/*
 * From <http://mathworld.wolfram.com/Point-PlaneDistance.html>.
 */
{
    DEVA_XYZ	w;
    double	distance;	/* signed distance! */

    w = v3d_subtract ( point, point_on_plane );

    distance = v3d_dotprod ( surface_normal, w );

    return ( distance );
}
#endif	/* DEVA_CONVEX */

static DEVA_XYZ
v3d_subtract ( DEVA_XYZ v1, DEVA_XYZ v2 )
{
    DEVA_XYZ	diff;

    diff.X = v1.X - v2.X;
    diff.Y = v1.Y - v2.Y;
    diff.Z = v1.Z - v2.Z;

    return ( diff );
}

double
v3d_dotprod ( DEVA_XYZ v1, DEVA_XYZ v2 )
{
    double	prod;

    prod = ( v1.X * v2.X ) + ( v1.Y * v2.Y ) + ( v1.Z * v2.Z );

    return ( prod );
}

static DEVA_gray_image *
DEVA_gray_or ( DEVA_gray_image *i1, DEVA_gray_image *i2 )
{
    DEVA_gray_image *new;
    int		    n_rows, n_cols;
    int		    row, col;

    if ( !DEVA_image_samesize ( i1, i2 ) ) {
	fprintf ( stderr, "DEVA_gray_or: image sizes don't match!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );	/* error return */
    }

    n_rows = DEVA_image_n_rows ( i1 );
    n_cols = DEVA_image_n_cols ( i1 );

    new = DEVA_gray_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( new, row, col ) =
		DEVA_image_data ( i1, row, col ) ||
		DEVA_image_data ( i2, row, col );
	}
    }

    return ( new );
}

#if defined(DEBUG_POSITION)||defined(DEBUG_ORIENTATION)||defined(DEBUG_COMBINED)
static void
make_visible ( DEVA_gray_image *boundaries )
{
    int     row, col;

    for ( row = 0; row < DEVA_image_n_rows ( boundaries ); row ++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( boundaries ); col ++ ) {
	    if ( DEVA_image_data ( boundaries, row, col ) ) {
		DEVA_image_data ( boundaries, row, col ) = 255;
	    }
	}
    }
}
#endif
