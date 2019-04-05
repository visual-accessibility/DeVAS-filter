#include <stdlib.h>
#include <math.h>
#include "devas-margin.h"
#include "devas-image.h"
#include "devas-license.h"       /* DeVAS open source license */

#include "radianceIO.h"

static		DeVAS_xyY scale_xyY ( double distance, double background_value,
		    DeVAS_xyY value );
static		DeVAS_float scale_float ( double distance,
		    double background_value, DeVAS_float value );
static double	sigmoid ( double x );

DeVAS_xyY_image *
DeVAS_xyY_add_margin ( int v_margin, int h_margin, DeVAS_xyY_image *original )
/*
 * Add a margin around the input file to reduce FFT artifacts due to
 * top-bottom and left-right wraparound.
 *
 * If DeVAS_MARGIN_REFLECT is set, margin pixels come are a blend of the
 * reflection of the original image pixels around the image edge and the
 * average luminance of the original image (DeVAS_MARGIN_AVERAGE_ALL set) or
 * the average luminance of all bounder pixels in the original image
 * (DeVAS_MARGIN_AVERAGE_ALL not set).  If DeVAS_MARGIN_REFLECT is not set,
 * values in the margin are a blend of the nearest boundary pixel in the input
 * and the average of all edge pixels (DeVAS_MARGIN_AVERAGE_ALL set) or the
 * average luminance of all bounder pixels in the original image
 * (DeVAS_MARGIN_AVERAGE_ALL not set).  The blend is weighted by a sigmod
 * function of the distance from the boundary pixel.  Only the luminance
 * component is affected by this weighting, since that is the only part
 * processed by the FFT in the devas-filter code.
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
    DeVAS_xyY_image  *with_margin;
    int		    row, col;
    int		    n_rows, n_cols;
    int		    new_n_rows, new_n_cols;
    double	    average_luminance;
    double	    distance, row_diff, col_diff;

    n_rows = DeVAS_image_n_rows ( original );
    n_cols = DeVAS_image_n_cols ( original );

    if ( ( v_margin <= 0 ) || ( h_margin <= 0 ) ) {
	fprintf ( stderr, "devas_add_margin: margin size too small (%d, %d)!\n",
		v_margin, h_margin );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( n_rows < 2 ) {
	fprintf ( stderr,
		"DeVAS_xyY_add_margin: n_rows too small (%d)!\n", n_rows );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( n_cols < 2 ) {
	fprintf ( stderr,
		"DeVAS_xyY_add_margin: n_cols too small (%d)!\n", n_cols );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( v_margin > ( n_rows / 2 ) ) { 
	fprintf ( stderr,
		"DeVAS_xyY_add_margin: v_margin too large (%d)!\n", v_margin );
    }

    if ( h_margin > ( n_cols / 2 ) ) { 
	fprintf ( stderr,
		"DeVAS_xyY_add_margin: h_margin too large (%d)!\n", h_margin );
    }

    new_n_rows = n_rows + ( 2 * v_margin );
    new_n_cols = n_cols + ( 2 * h_margin );

    with_margin = DeVAS_xyY_image_new ( new_n_rows, new_n_cols );

    /* Reset FOV based on degrees/pixel, not on trignometry. */
    if ( ( DeVAS_image_view ( original ) . vert <= 0.0 ) ||
	    ( DeVAS_image_view ( original ) . horiz <= 0.0 ) ) {
	fprintf ( stderr,
		"DeVAS_xyY_add_margin: invalid or missing fov (%f, %f)!\n",
		    DeVAS_image_view ( original ) . vert,
		    DeVAS_image_view ( original ) .  horiz );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* copy VIEW record and update horizontal and vertical FOVs */
    DeVAS_image_view ( with_margin ) = DeVAS_image_view ( original );
    DeVAS_image_view ( with_margin ) . vert = ((double) new_n_rows ) *
	( DeVAS_image_view ( original ) . vert / ((double) n_rows ) );
    DeVAS_image_view ( with_margin ) . horiz = ((double) new_n_cols ) *
	( DeVAS_image_view ( original ) . horiz / ((double) n_cols ) );

#ifdef DeVAS_MARGIN_AVERAGE_ALL

    /* compute average luminance of all pixels in original image */
    average_luminance = 0.0;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    average_luminance += DeVAS_image_data ( original, row, col ).Y;
	}
    }

    average_luminance /= (double) ( n_rows * n_cols );

#else

    /* compute average luminance of all border pixels in original image */
    average_luminance = 0.0;
    for ( col = 0; col < n_cols; col++ ) {
	average_luminance += DeVAS_image_data ( original, 0, col ).Y;
	average_luminance += DeVAS_image_data ( original, n_rows - 1 , col ).Y;
    }
    for ( row = 1; row < n_rows - 1; row++ ) {
	average_luminance += DeVAS_image_data ( original, row, 0 ).Y;
	average_luminance += DeVAS_image_data ( original, row, n_cols - 1 ).Y;
    }
    average_luminance /= (double) ( ( 2 * ( n_rows + n_cols ) ) - 2 );

#endif	/* DeVAS_MARGIN_AVERAGE_ALL */

    /* copy contents of original image to center of new image */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( with_margin, row + v_margin, col + h_margin ) =
		DeVAS_image_data ( original, row, col );
	}
    }

#ifdef DeVAS_MARGIN_REFLECT

    /*
     * Reflect portion of image inside the boundary to the margin,
     * rather than copying nearest edge pixel.
     */

    /* top, bottom margins */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( with_margin, row, col + h_margin ) =
		scale_xyY ( 1.0 - ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, v_margin - 1 - row, col ) );
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin ) =
		scale_xyY ( ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, n_rows - 1 - row, col ) );
	}
    }

    /* left, right margins */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {
	    DeVAS_image_data ( with_margin, row + v_margin, col ) =
		scale_xyY ( 1.0 - ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, h_margin - 1 - col ) );
	    DeVAS_image_data ( with_margin, row + v_margin,
		    col + h_margin + n_cols ) =
		scale_xyY ( ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, n_cols - 1 - col ) );
	}
    }

    /* corners */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {

	    /* upper left */
	    row_diff = ((double) ( v_margin - 1 - row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row, col ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, v_margin - 1 - row,
			    h_margin - 1 - col ) );

	    /* upper right */
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row, col + h_margin + n_cols ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, v_margin - 1 - row,
			    n_cols - 1 - col ) );

	    /* lower left */
	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows, col ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1 - row,
			    h_margin - 1 - col ) );

	    /* lower right */
	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin + n_cols ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1 - row,
			    n_cols - 1 - col ) );
	}
    }

#else

    /*
     * Create margin by flooding from nearest edge pixels.
     */

    /* top, bottom margins */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( with_margin, row, col + h_margin ) =
		scale_xyY ( 1.0 - ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, 0, col ) );
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin ) =
		scale_xyY ( ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, n_rows - 1, col ) );
	}
    }

    /* left, right margins */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {
	    DeVAS_image_data ( with_margin, row + v_margin, col ) =
		scale_xyY ( 1.0 - ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, 0 ) );
	    DeVAS_image_data ( with_margin, row + v_margin,
		    col + h_margin + n_cols ) =
		scale_xyY ( ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, n_cols - 1 ) );
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
	    DeVAS_image_data ( with_margin, row, col ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, 0, 0 ) );

	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row, col + h_margin + n_cols ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, 0, n_cols - 1 ) );

	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows, col ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1, 0 ) );

	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin + n_cols ) =
		scale_xyY ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1, n_cols - 1 ) );
	}
    }

#endif	/* DeVAS_MARGIN_REFLECT */

    return ( with_margin );
}

static DeVAS_xyY
scale_xyY ( double distance, double background_value, DeVAS_xyY value )
/* distance in range [0.0 - 1.0] */
{
    DeVAS_xyY	new_value;
    double	weight;

    if ( ( distance < 0.0 ) || ( distance > 1.0 ) ) {
	fprintf ( stderr, "devas-margin:scale: invalid distance (%f)\n",
		distance );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
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

DeVAS_xyY_image *
DeVAS_xyY_strip_margin ( int v_margin, int h_margin,
	DeVAS_xyY_image *with_margin )
/*
 * Remove a margin around the input file.
 *
 * v_margin:	Vertical (row) margin to remove, in pixels.  Total vertical
 * 		dimension decreases by twice this value.
 * h_margin:	Horizontal (col) margin to remove, in pixels.  Total horizontal
 * 		dimension decreases by twice this value.
 */
{
    DeVAS_xyY_image  *with_margin_stripped;
    int		    row, col;
    int		    n_rows, n_cols;
    int		    new_n_rows, new_n_cols;

    n_rows = DeVAS_image_n_rows ( with_margin );
    n_cols = DeVAS_image_n_cols ( with_margin );

    new_n_rows = n_rows - ( 2 * v_margin );
    new_n_cols = n_cols - ( 2 * h_margin );

    if ( ( new_n_rows < 1 ) || ( new_n_cols < 1 ) ) {
	fprintf ( stderr,
  "DeVAS_xyY_strip_margin: margins too big for image (%d, %d), (%d, %d)!\n",
	  	v_margin, h_margin, n_rows, n_cols );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    with_margin_stripped = DeVAS_xyY_image_new ( new_n_rows, new_n_cols );

    /* Reset FOV based on degrees/pixel, not on trignometry! */
    if ( ( DeVAS_image_view ( with_margin ) . vert <= 0.0 ) ||
	    ( DeVAS_image_view ( with_margin ) . horiz <= 0.0 ) ) {
	fprintf ( stderr,
		"DeVAS_xyY_strip_margin: invalid or missing fov (%f, %f)!\n",
		    DeVAS_image_view ( with_margin ) . vert,
		    DeVAS_image_view ( with_margin ) .  horiz );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* copy center of image with margins to new image */
    DeVAS_image_view ( with_margin_stripped ) =
	DeVAS_image_view ( with_margin );
    DeVAS_image_view ( with_margin_stripped ) . vert = ((double) new_n_rows ) *
	( DeVAS_image_view ( with_margin ) . vert / ((double) n_rows ) );
    DeVAS_image_view ( with_margin_stripped ) . horiz = ((double) new_n_cols ) *
	( DeVAS_image_view ( with_margin ) . horiz / ((double) n_cols ) );

    for ( row = 0; row < new_n_rows; row++ ) {
	for ( col = 0; col < new_n_cols; col++ ) {
	    DeVAS_image_data ( with_margin_stripped, row, col ) =
	      DeVAS_image_data ( with_margin, row + v_margin, col + h_margin );
	}
    }

    return ( with_margin_stripped );
}

DeVAS_float_image *
DeVAS_float_add_margin ( int v_margin, int h_margin,
	DeVAS_float_image *original )
/*
 * Add a margin around the input file to reduce FFT artifacts due to
 * top-bottom and left-right wraparound. Values in the margin are a blend
 * of the nearest edge pixel in the input and the average of all edge pixels.
 * The blend is weighted by a sigmod function of the distance from the edge
 * pixel.
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
    DeVAS_float_image	*with_margin;
    int		    	row, col;
    int		    	n_rows, n_cols;
    int		    	new_n_rows, new_n_cols;
    double	    	average_luminance;
    double	    	distance, row_diff, col_diff;

    n_rows = DeVAS_image_n_rows ( original );
    n_cols = DeVAS_image_n_cols ( original );

    if ( ( v_margin <= 0 ) || ( h_margin <= 0 ) ) {
	fprintf ( stderr, "devas_add_margin: margin size too small (%d, %d)!\n",
		v_margin, h_margin );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( n_rows < 2 ) {
	fprintf ( stderr,
		"DeVAS_float_add_margin: n_rows too small (%d)!\n", n_rows );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( n_cols < 2 ) {
	fprintf ( stderr,
		"DeVAS_float_add_margin: n_cols too small (%d)!\n", n_cols );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( v_margin > ( n_rows / 2 ) ) { 
	fprintf ( stderr,
	    "DeVAS_float_add_margin: v_margin too large (%d)!\n", v_margin );
    }

    if ( h_margin > ( n_cols / 2 ) ) { 
	fprintf ( stderr,
	    "DeVAS_float_add_margin: h_margin too large (%d)!\n", h_margin );
    }

    new_n_rows = n_rows + ( 2 * v_margin );
    new_n_cols = n_cols + ( 2 * h_margin );

    with_margin = DeVAS_float_image_new ( new_n_rows, new_n_cols );

    /* Reset FOV based on degrees/pixel, not on trignometry. */
    if ( ( DeVAS_image_view ( original ) . vert <= 0.0 ) ||
	    ( DeVAS_image_view ( original ) . horiz <= 0.0 ) ) {
	fprintf ( stderr,
		"DeVAS_float_add_margin: invalid or missing fov (%f, %f)!\n",
		    DeVAS_image_view ( original ) . vert,
		    DeVAS_image_view ( original ) .  horiz );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* copy VIEW record and update horizontal and vertical FOVs */
    DeVAS_image_view ( with_margin ) = DeVAS_image_view ( original );
    DeVAS_image_view ( with_margin ) . vert = ((double) new_n_rows ) *
	( DeVAS_image_view ( original ) . vert / ((double) n_rows ) );
    DeVAS_image_view ( with_margin ) . horiz = ((double) new_n_cols ) *
	( DeVAS_image_view ( original ) . horiz / ((double) n_cols ) );

#ifdef DeVAS_MARGIN_AVERAGE_ALL

    /* computer average luminance of all pixels in original image */
    average_luminance = 0.0;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    average_luminance += DeVAS_image_data ( original, row, col );
	}
    }

    average_luminance /= (double) ( n_rows * n_cols );

#else

    /* computer average luminance of all border pixels in original image */
    average_luminance = 0.0;
    for ( col = 0; col < n_cols; col++ ) {
	average_luminance += DeVAS_image_data ( original, 0, col );
	average_luminance += DeVAS_image_data ( original, n_rows - 1 , col );
    }
    for ( row = 1; row < n_rows - 1; row++ ) {
	average_luminance += DeVAS_image_data ( original, row, 0 );
	average_luminance += DeVAS_image_data ( original, row, n_cols - 1 );
    }
    average_luminance /= (double) ( ( 2 * ( n_rows + n_cols ) ) - 2 );

#endif	/* DeVAS_MARGIN_AVERAGE_ALL */

    /* copy contents of original image to center of new image */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( with_margin, row + v_margin, col + h_margin ) =
		DeVAS_image_data ( original, row, col );
	}
    }

#ifdef DeVAS_MARGIN_REFLECT

    /*
     * Reflect portion of image inside the boundary to the margin,
     * rather than copying nearest edge pixel.
     */

    /* top, bottom margins */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( with_margin, row, col + h_margin ) =
		scale_float ( 1.0 - ( ((double) row ) / ((double) v_margin ) ),
		    average_luminance,
		      DeVAS_image_data ( original, v_margin - 1 - row, col ) );
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin ) =
		scale_float ( ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, n_rows - 1 - row, col ) );
	}
    }

    /* left, right margins */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {
	    DeVAS_image_data ( with_margin, row + v_margin, col ) =
		scale_float ( 1.0 - ( ((double) col ) / ((double) h_margin ) ),
		    average_luminance,
		      DeVAS_image_data ( original, row, h_margin - 1 - col ) );
	    DeVAS_image_data ( with_margin, row + v_margin,
		    col + h_margin + n_cols ) =
		scale_float ( ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, n_cols - 1 - col ) );
	}
    }

    /* corners */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {

	    /* upper left */
	    row_diff = ((double) ( v_margin - 1 - row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row, col ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, v_margin - 1 - row,
			    h_margin - 1 - col ) );

	    /* upper right */
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row, col + h_margin + n_cols ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, v_margin - 1 - row,
			    n_cols - 1 - col ) );

	    /* lower left */
	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows, col ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1 - row,
			    h_margin - 1 - col ) );

	    /* lower right */
	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin + n_cols ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1 - row,
			    n_cols - 1 - col ) );
	}
    }

#else

    /*
     * Create margin by flooding from nearest edge pixels.
     */

    /* top, bottom margins */
    for ( row = 0; row < v_margin; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DeVAS_image_data ( with_margin, row, col + h_margin ) =
		scale_float ( 1.0 - ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, 0, col ) );
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin ) =
		scale_float ( ( ((double) row ) / ((double) v_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, n_rows - 1, col ) );
	}
    }

    /* left, right margins */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < h_margin; col++ ) {
	    DeVAS_image_data ( with_margin, row + v_margin, col ) =
		scale_float ( 1.0 - ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, 0 ) );
	    DeVAS_image_data ( with_margin, row + v_margin,
		    col + h_margin + n_cols ) =
		scale_float ( ( ((double) col ) / ((double) h_margin ) ),
			average_luminance,
			DeVAS_image_data ( original, row, n_cols - 1 ) );
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
	    DeVAS_image_data ( with_margin, row, col ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, 0, 0 ) );

	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row, col + h_margin + n_cols ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, 0, n_cols - 1 ) );

	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( h_margin - 1 - col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows, col ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1, 0 ) );

	    row_diff = ((double) ( row ) ) /
		((double) ( v_margin - 1 ) );
	    col_diff = ((double) ( col ) ) /
		((double) ( h_margin - 1 ) );
	    distance =
		sqrt ( ( row_diff * row_diff ) + ( col_diff * col_diff ) );
	    if ( distance > 1.0 ) {
		distance = 1.0;
	    }
	    DeVAS_image_data ( with_margin, row + v_margin + n_rows,
		    col + h_margin + n_cols ) =
		scale_float ( distance, average_luminance,
			DeVAS_image_data ( original, n_rows - 1, n_cols - 1 ) );
	}
    }

#endif	/* DeVAS_MARGIN_REFLECT */

    return ( with_margin );
}

static DeVAS_float
scale_float ( double distance, double background_value, DeVAS_float value )
/* distance in range [0.0 - 1.0] */
{
    DeVAS_float	new_value;
    double	weight;

    if ( ( distance < 0.0 ) || ( distance > 1.0 ) ) {
	fprintf ( stderr, "devas-margin:scale: invalid distance (%f)\n",
		distance );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* hardwired constant (6.0) in sigmoid to make results plausible */
    weight = 2.0 * ( sigmoid ( 6.0 * ( 1.0 - distance ) ) - 0.5 );
    new_value = ( weight * value ) +
	( ( 1.0 - weight ) * background_value );

    return ( new_value );
}

DeVAS_float_image *
DeVAS_float_strip_margin ( int v_margin, int h_margin,
	DeVAS_float_image *with_margin )
/*
 * Remove a margin around the input file.
 *
 * v_margin:	Vertical (row) margin to remove, in pixels.  Total vertical
 * 		dimension decreases by twice this value.
 * h_margin:	Horizontal (col) margin to remove, in pixels.  Total horizontal
 * 		dimension decreases by twice this value.
 */
{
    DeVAS_float_image	*with_margin_stripped;
    int		    	row, col;
    int		    	n_rows, n_cols;
    int		    	new_n_rows, new_n_cols;

    n_rows = DeVAS_image_n_rows ( with_margin );
    n_cols = DeVAS_image_n_cols ( with_margin );

    new_n_rows = n_rows - ( 2 * v_margin );
    new_n_cols = n_cols - ( 2 * h_margin );

    if ( ( new_n_rows < 1 ) || ( new_n_cols < 1 ) ) {
	fprintf ( stderr,
  "DeVAS_float_strip_margin: margins too big for image (%d, %d), (%d, %d)!\n",
	  	v_margin, h_margin, n_rows, n_cols );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    with_margin_stripped = DeVAS_float_image_new ( new_n_rows, new_n_cols );

    /* Reset FOV based on degrees/pixel, not on trignometry! */
    if ( ( DeVAS_image_view ( with_margin ) . vert <= 0.0 ) ||
	    ( DeVAS_image_view ( with_margin ) . horiz <= 0.0 ) ) {
	fprintf ( stderr,
		"DeVAS_float_strip_margin: invalid or missing fov (%f, %f)!\n",
		    DeVAS_image_view ( with_margin ) . vert,
		    DeVAS_image_view ( with_margin ) .  horiz );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    /* copy center of image with margins to new image */
    DeVAS_image_view ( with_margin_stripped ) =
	DeVAS_image_view ( with_margin );
    DeVAS_image_view ( with_margin_stripped ) . vert = ((double) new_n_rows ) *
	( DeVAS_image_view ( with_margin ) . vert / ((double) n_rows ) );
    DeVAS_image_view ( with_margin_stripped ) . horiz = ((double) new_n_cols ) *
	( DeVAS_image_view ( with_margin ) . horiz / ((double) n_cols ) );

    for ( row = 0; row < new_n_rows; row++ ) {
	for ( col = 0; col < new_n_cols; col++ ) {
	    DeVAS_image_data ( with_margin_stripped, row, col ) =
	      DeVAS_image_data ( with_margin, row + v_margin, col + h_margin );
	}
    }

    return ( with_margin_stripped );
}
