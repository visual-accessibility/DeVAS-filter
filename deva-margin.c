#include <stdlib.h>
#include <math.h>
#include "deva-margin.h"
#include "deva-image.h"
#include "deva-license.h"       /* DEVA open source license */

static		DEVA_xyY scale ( double distance, double background_value,
		    DEVA_xyY value );
static double	sigmoid ( double x );

DEVA_xyY_image *
DEVA_xyY_add_margin ( int v_margin, int h_margin, DEVA_xyY_image *original )
/*
 * Add a margin around the input file to reduce FFT artifacts due to
 * top-bottom and left-right wraparound. Values in the margin are a blend
 * of the nearest edge pixel in the input and the average of all edge pixels.
 * The blend is weighted by a sigmod function of the distance from the edge
 * pixel.  Only the luminance component is affected by this weighting, since
 * that is the only part processed by the FFT in the deva-filter code.
 *
 * Field-of-view is updated in the output to keep degrees-per-pixel the
 * same as for the input.
 *
 * v_margin:	Vertical (row) margin to add, in pixels.  Total vertical
 * 		dimension increases by twice this value.
 * h_margin:	Horizontal (col) margin to add, in pixels.  Total horizontal
 * 		dimension increases by twice this value.
 */
{
    DEVA_xyY_image  *with_margin;
    int		    row, col;
    int		    n_rows, n_cols;
    int		    new_n_rows, new_n_cols;
    double	    average_luminance_at_border;
    double	    distance, row_diff, col_diff;

    n_rows = DEVA_image_n_rows ( original );
    n_cols = DEVA_image_n_cols ( original );

    if ( ( v_margin <= 0 ) || ( h_margin <= 0 ) ) {
	fprintf ( stderr, "deva_add_margin: margin size too small (%d, %d)!\n",
		v_margin, h_margin );
	exit ( EXIT_FAILURE );
    }

    if ( n_rows < 2 ) {
	fprintf ( stderr,
		"DEVA_xyY_add_margin: n_rows too small (%d)!\n", n_rows );
	exit ( EXIT_FAILURE );
    }
    if ( n_cols < 2 ) {
	fprintf ( stderr,
		"DEVA_xyY_add_margin: n_cols too small (%d)!\n", n_cols );
	exit ( EXIT_FAILURE );
    }

    new_n_rows = n_rows + ( 2 * v_margin );
    new_n_cols = n_cols + ( 2 * h_margin );

    with_margin = DEVA_xyY_image_new ( new_n_rows, new_n_cols );

    /* Reset FOV based on degrees/pixel, not on trignometry. */
    if ( ( DEVA_image_view ( original ) . vert <= 0.0 ) ||
	    ( DEVA_image_view ( original ) . horiz <= 0.0 ) ) {
	fprintf ( stderr,
		"DEVA_xyY_add_margin: invalid or missing fov (%f, %f)!\n",
		    DEVA_image_view ( original ) . vert,
		    DEVA_image_view ( original ) .  horiz );
	exit ( EXIT_FAILURE );
    }

    /* copy VIEW record and update horizontal and vertical FOVs */
    DEVA_image_view ( with_margin ) = DEVA_image_view ( original );
    DEVA_image_view ( with_margin ) . vert = ((double) new_n_rows ) *
	( DEVA_image_view ( original ) . vert / ((double) n_rows ) );
    DEVA_image_view ( with_margin ) . horiz = ((double) new_n_cols ) *
	( DEVA_image_view ( original ) . horiz / ((double) n_cols ) );

    /* computer average luminance of all border pixels in original image */
    average_luminance_at_border = 0.0;
    for ( col = 0; col < n_cols; col++ ) {
	average_luminance_at_border += DEVA_image_data ( original, 0, col ).Y;
	average_luminance_at_border += DEVA_image_data ( original,
							n_rows - 1 , col ).Y;
    }
    for ( row = 1; row < n_rows - 1; row++ ) {
	average_luminance_at_border += DEVA_image_data ( original, row, 0 ).Y;
	average_luminance_at_border += DEVA_image_data ( original,
							row, n_cols - 1 ).Y;
    }
    average_luminance_at_border /= (double) ( ( 2 * ( n_rows + n_cols ) ) - 2 );

    /* copy contents of original image to center of new image */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( with_margin, row + v_margin, col + h_margin ) =
		DEVA_image_data ( original, row, col );
	}
    }

    /* top, bottom margins */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( with_margin, row, col + h_margin ) =
		scale ( 1.0 - ( ((double) row ) / ((double) v_margin ) ),
			average_luminance_at_border,
			DEVA_image_data ( original, 0, col ) );
	    DEVA_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin ) =
		scale ( ( ((double) row ) / ((double) v_margin ) ),
			average_luminance_at_border,
			DEVA_image_data ( original, n_rows - 1, col ) );
	}
    }

    /* left, right margins */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {
	    DEVA_image_data ( with_margin, row + v_margin, col ) =
		scale ( 1.0 - ( ((double) col ) / ((double) h_margin ) ),
			average_luminance_at_border,
			DEVA_image_data ( original, row, 0 ) );
	    DEVA_image_data ( with_margin, row + v_margin,
		    col + h_margin + n_cols ) =
		scale ( ( ((double) col ) / ((double) h_margin ) ),
			average_luminance_at_border,
			DEVA_image_data ( original, row, n_cols - 1 ) );
	}
    }

    /* corners */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {

	    row_diff = ((double) ( v_margin - 1 - row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DEVA_image_data ( with_margin, row, col ) =
		scale ( distance, average_luminance_at_border,
			DEVA_image_data ( original, 0, 0 ) );

	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DEVA_image_data ( with_margin, row, col + h_margin + n_cols ) =
		scale ( distance, average_luminance_at_border,
			DEVA_image_data ( original, 0, n_cols - 1 ) );

	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DEVA_image_data ( with_margin, row + v_margin + n_rows, col ) =
		scale ( distance, average_luminance_at_border,
			DEVA_image_data ( original, n_rows - 1, 0 ) );

	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DEVA_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin + n_cols ) =
		scale ( distance, average_luminance_at_border,
			DEVA_image_data ( original, n_rows - 1, n_cols - 1 ) );
	}
    }

    return ( with_margin );
}

static DEVA_xyY
scale ( double distance, double background_value, DEVA_xyY value )
/* distance in range [0.0 - 1.0] */
{
    DEVA_xyY	new_value;
    double	weight;

    if ( ( distance < 0.0 ) || ( distance > 1.0 ) ) {
	fprintf ( stderr, "deva-margin:scale: invalid distance (%f)\n",
		distance );
	exit ( EXIT_FAILURE );
    }

    new_value.x = value.x;
    new_value.y = value.y;

    /* hardwired constant (6.0) in sigmoid to make results plausible */
    weight = 2.0 * ( sigmoid ( 6.0 * ( 1.0 - distance ) ) - 0.5 );
    new_value.Y = ( weight * value.Y ) +
	( ( 1.0 - weight ) * background_value );

    return ( new_value );
}

static double
sigmoid ( double x )
{
    return ( 1.0 / ( 1.0 + exp ( -x ) ) );
}

DEVA_xyY_image *
DEVA_xyY_strip_margin ( int v_margin, int h_margin,
	DEVA_xyY_image *with_margin )
/*
 * Remove a margin around the input file.
 *
 * v_margin:	Vertical (row) margin to remove, in pixels.  Total vertical
 * 		dimension decreases by twice this value.
 * h_margin:	Horizontal (col) margin to remove, in pixels.  Total horizontal
 * 		dimension decreases by twice this value.
 */
{
    DEVA_xyY_image  *with_margin_stripped;
    int		    row, col;
    int		    n_rows, n_cols;
    int		    new_n_rows, new_n_cols;

    n_rows = DEVA_image_n_rows ( with_margin );
    n_cols = DEVA_image_n_cols ( with_margin );

    new_n_rows = n_rows - ( 2 * v_margin );
    new_n_cols = n_cols - ( 2 * h_margin );

    if ( ( new_n_rows < 1 ) || ( new_n_cols < 1 ) ) {
	fprintf ( stderr,
  "DEVA_xyY_strip_margin: margins too big for image (%d, %d), (%d, %d)!\n",
	  	v_margin, h_margin, n_rows, n_cols );
	exit ( EXIT_FAILURE );
    }

    with_margin_stripped = DEVA_xyY_image_new ( new_n_rows, new_n_cols );

    /* Reset FOV based on degrees/pixel, not on trignometry! */
    if ( ( DEVA_image_view ( with_margin ) . vert <= 0.0 ) ||
	    ( DEVA_image_view ( with_margin ) . horiz <= 0.0 ) ) {
	fprintf ( stderr,
		"DEVA_xyY_strip_margin: invalid or missing fov (%f, %f)!\n",
		    DEVA_image_view ( with_margin ) . vert,
		    DEVA_image_view ( with_margin ) .  horiz );
	exit ( EXIT_FAILURE );
    }

    /* copy center of image with margins to new image */
    DEVA_image_view ( with_margin_stripped ) = DEVA_image_view ( with_margin );
    DEVA_image_view ( with_margin_stripped ) . vert = ((double) new_n_rows ) *
	( DEVA_image_view ( with_margin ) . vert / ((double) n_rows ) );
    DEVA_image_view ( with_margin_stripped ) . horiz = ((double) new_n_cols ) *
	( DEVA_image_view ( with_margin ) . horiz / ((double) n_cols ) );

    for ( row = 0; row < new_n_rows; row++ ) {
	for ( col = 0; col < new_n_cols; col++ ) {
	    DEVA_image_data ( with_margin_stripped, row, col ) =
		DEVA_image_data ( with_margin, row + v_margin, col + h_margin );
	}
    }

    return ( with_margin_stripped );
}
