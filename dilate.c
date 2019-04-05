/*
 * Expand binary image by a given radius.
 *
 * This version uses the true Euclidean distance transformation descrbed
 * in Felzenszwalb and Huttenlocher (2012), "Distance Transforms of Sampled
 * Functions," Theory of Computing, 8(1).  As a result, execution time is
 * independent of the radius, and in general is quite fast.  For *very*
 * small radii, a splatting method may be faster.
 *
 * The dt_euclid_sq is exposed so that it can be used as a general distance
 * transform by routines that require such functionality.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "dilate.h"
#include "devas-license.h"	/* DeVAS open source license */

#define	SQ(x)	((x)*(x))

/*
 * See Felzenszwalb and Huttenlocher (2012) for the definition of these
 * variables.
 */
static int	*v;	/* temporary work space */
static float	*z;	/* temporary work space */
static float	*f;	/* temporary work space */
static float	*D_f;	/* temporary work space */
static float	inf;	/* larger than any valid distance^2 */

static void	dt_euclid_sq_1d ( int size, float *input, float *output );

DeVAS_gray_image *
DeVAS_gray_dilate ( DeVAS_gray_image *input, double radius )
/*
 * Dilate a binary image by a fixed radius.
 */
{
    DeVAS_gray_image  *output;

    output = DeVAS_gray_image_new ( DeVAS_image_n_rows ( input ),
	    DeVAS_image_n_cols ( input ) );

    DeVAS_gray_dilate_2 ( input, output, radius );

    return ( output );
}

void
DeVAS_gray_dilate_2 ( DeVAS_gray_image *input, DeVAS_gray_image *output,
	double radius )
/*
 * Dilate a binary image by a fixed radius, returning result in a previously
 * allocated image object.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    double		radius_squared;
    DeVAS_float_image	*euclid_sq;

    if ( !DeVAS_gray_image_samesize ( input, output ) ) {
	fprintf ( stderr,
	    "DeVAS_gray_dilate2: input and output image sizes don't match!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( radius < 1.0 ) {
	fprintf ( stderr, "DeVAS_gray_dilate: invalid radius (%f)!\n", radius );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    n_rows = DeVAS_image_n_rows ( input );
    n_cols = DeVAS_image_n_cols ( input );

    euclid_sq = dt_euclid_sq ( input );

    radius_squared = radius * radius;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( output, row, col ) =
		( DeVAS_image_data ( euclid_sq, row, col ) <= radius_squared );
	}
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( euclid_sq, row, col ) =
		sqrt ( DeVAS_image_data ( euclid_sq, row, col ) );
	}
    }

    DeVAS_float_image_delete ( euclid_sq );
}

DeVAS_float_image *
dt_euclid_sq ( DeVAS_gray_image *input )
/*
 * Compute 2D squared Euclidean distance transform of binary image using
 * the method described in Felzenszwalb and Huttenlocher (2012), "Distance
 * Transforms of Sampled Functions," Theory of Computing, 8(1).
 *
 * input:   TRUE if point from which distance should be evaluated,
 * 	    FALSE otherwise
 *
 * output:  Distance to nearest pixel in input, in inter-pixel units.
 */
{
    DeVAS_float_image	*output;

    output = DeVAS_float_image_new ( DeVAS_image_n_rows ( input ),
	    DeVAS_image_n_cols ( input ) );

    dt_euclid_sq_2 ( input, output );

    return ( output );
}

void
dt_euclid_sq_2 ( DeVAS_gray_image *input, DeVAS_float_image *output )
{
    unsigned int	n_rows, n_cols;
    unsigned int	row, col;
    unsigned int	max_n_rows_n_cols;

    n_rows = DeVAS_image_n_rows ( input );
    n_cols = DeVAS_image_n_cols ( input );

    if ( !DeVAS_image_samesize ( input, output ) ) {
	fprintf ( stderr, "dt_euclid_sq_2: input and output not same size!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* allocate temporary workspace */
    if ( n_rows > n_cols ) {
	max_n_rows_n_cols = n_rows;
    } else {
	max_n_rows_n_cols = n_cols;
    }

    v = (int *) malloc ( max_n_rows_n_cols * sizeof ( int ) );
    z = (float *) malloc ( ( max_n_rows_n_cols + 1 ) * sizeof ( float ) );
    f = (float *) malloc ( max_n_rows_n_cols * sizeof ( float ) );
    D_f = (float *) malloc ( max_n_rows_n_cols * sizeof ( float ) );

    if ( ( v == NULL ) || ( z == NULL ) || ( f == NULL ) || ( D_f == NULL ) ) {
	fprintf ( stderr, "dt_euclid_sq: malloc failed!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    inf = SQ ( n_rows + n_cols + 1 );	/* larger than any valid distance^s */

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( DeVAS_image_data ( input, row, col ) ) {
		DeVAS_image_data ( output, row, col ) = 0.0;
	    } else {
		DeVAS_image_data ( output, row, col ) = inf;
	    }
	}
    }

    /* transform columns */
    for ( col = 0; col < n_cols; col++ ) {
	for ( row = 0; row < n_rows; row++ ) {
	    f[row] = DeVAS_image_data ( output, row, col );
	}

	dt_euclid_sq_1d ( n_rows, f, D_f );

	for ( row = 0; row < n_rows; row++ ) {
	    DeVAS_image_data ( output, row, col ) = D_f[row];
	}
    }

    /* transform rows */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    f[col] = DeVAS_image_data ( output, row, col );
	}

	dt_euclid_sq_1d ( n_cols, f, D_f );

	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( output, row, col ) = D_f[col];
	}
    }

    free ( v );
    free ( z );
    free ( f );
    free ( D_f );
}

static void
dt_euclid_sq_1d ( int size, float *f, float *D_f )
/*
 * One-dimensional distance transform under the squared Euclidean distance.
 */
{
    unsigned int    k, q;
    float	    s;

    /* images v, z, and D_f preallocated */

    k = 0;		/* Index of rightmost parabola in lower envelope */

    v[0] = 0;		/* Locations of parabolas in lower envelope */
    z[0] = -inf;	/* Locations of boundaries between parabolas */
    z[1] = inf;

    for ( q = 1; q < size; q++ ) {	/* Compute lower envelope */
	s = ( ( f[q] + SQ ( (float) q ) ) -
		( f[v[k]] + SQ ( (float) v[k] ) ) ) /
	    ( ( 2.0 * ( (float) q ) ) - ( 2.0 * ( (float) v[k] ) ) );

	while ( s <= z[k] ) {
	    k--;
	    s = ( ( f[q] + SQ ( (float) q ) ) -
		    ( f[v[k]] + SQ ( (float) v[k] ) ) ) /
		( ( 2.0 * ( (float) q ) ) - ( 2.0 * ( (float) v[k] ) ) );
	}

	k++;

	v[k] = q;
	z[k] = s;
	z[k+1] = inf;
    }

    k = 0;
    for ( q = 0; q < size; q++ ) {  /* Fill in values of distance transform */
	while ( z[k+1] < q ) {
	    k++;
	}

	D_f[q] = ( (float) SQ ( ( q - v[k] ) ) ) + f[v[k]];
    }
}
