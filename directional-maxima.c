#include <stdlib.h>
#include "deva-image.h"
#include "directional-maxima.h"
#include "deva-license.h"       /* DEVA open source license */

DEVA_gray_image *
find_directional_maxima ( int patch_size, double threshold,
	DEVA_float_image *values )
/*
 * Find direction local maxima of an array of floating point values.
 *
 * patch_size:	Check if center pixel in a patch_size x patch_size region
 * 		is a directional local maxima.  patch_size must be an
 * 		odd integer >= 3.
 *
 * threshold:	Ignore directional local maxima if value is < threshold.
 *
 * values:	Values to evaluate.
 *
 * Returned value:
 * 		Boolean values, TRUE if corresponding pixel in values is
 * 		an above threshold directional local maxima.
 */
{
    DEVA_gray_image *directional_maxima;
    int		    n_rows, n_cols;
    int		    row, col;
    int		    i;
    float	    center_value;
    int	    	    pad_size;
    int		    local_maxima = TRUE;

    if ( patch_size < 3 ) {
	fprintf ( stderr,
		"find_directional_maxima: invalid patch_size (%d)!\n",
		patch_size );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
    if ( ( patch_size % 2 ) != 1 ) {
	fprintf ( stderr,
		"find_directional_maxima: patch_size must be odd!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    n_rows = DEVA_image_n_rows ( values );
    n_cols = DEVA_image_n_cols ( values );

    directional_maxima = DEVA_gray_image_new ( n_rows, n_cols );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( directional_maxima, row, col ) = FALSE;
	}
    }

    pad_size = ( patch_size - 1 ) / 2;	/* can't get any closer than this to */
    					/* edge of search patch */

    /*
     * Default is to look over 4-connected region (horizontal and vertical).
     * If EIGHT_CONNECTED is defined, search is over horizontal, vertical,
     * and two diagonal directions.
     *
     * Ties are broken by requiring center pixel value to be strictly larger
     * than neighborhood values in one direction, while requring only that
     * the value be greater of equal to neighborhood values in the other
     * direction.
     */

    /* special case for 3x3 patch */
    if ( patch_size == 3 ) {
	for ( row = pad_size; row < n_rows - pad_size; row++ ) {
	    for ( col = pad_size; col < n_cols - pad_size; col++ ) {
		center_value = DEVA_image_data ( values, row, col );

		if ( center_value < threshold ) {
		    continue;
		}

		if ( ( ( center_value >=		/* up-down */
				DEVA_image_data ( values, row-1, col ) ) &&
			    ( center_value >
			      DEVA_image_data ( values, row+1, col ) ) ) ||
#ifdef EIGHT_CONNECTED
			( ( center_value >		/* negative diagonal */
			    DEVA_image_data ( values, row-1, col-1 ) ) &&
			  ( center_value >=
			    DEVA_image_data ( values, row+1, col+1 ) ) ) ||
			( ( center_value >		/* positive diagonal */
			    DEVA_image_data ( values, row-1, col+1 ) ) &&
			  ( center_value >=
			    DEVA_image_data ( values, row+1, col-1 ) ) ) ||
#endif	/* EIGHT_CONNECTED */
			( ( center_value >=		/* side-to-side */
			    DEVA_image_data ( values, row, col-1 ) ) &&
			  ( center_value >
			    DEVA_image_data ( values, row, col+1 ) ) ) ) {
		    DEVA_image_data ( directional_maxima, row, col ) = TRUE;
		}
	    }
	}

    } else {
	/* search over arbitrarily sized patch */
	for ( row = pad_size; row < n_rows - pad_size; row++ ) {
	    for ( col = pad_size; col < n_cols - pad_size; col++ ) {
		center_value = DEVA_image_data ( values, row, col );

		if ( center_value < threshold )
		    continue;

		/* up-down */

		local_maxima = TRUE;

		for ( i = 1; i < pad_size; i++ ) {
		    if ( ( center_value <
				DEVA_image_data ( values, row-i, col ) ) ||
			    ( center_value <=
			      DEVA_image_data ( values, row+i, col ) ) ) {
			local_maxima = FALSE;
			break;
		    }
		}

		if ( local_maxima ) {
		    DEVA_image_data ( directional_maxima, row, col ) = TRUE;
		    continue;
		}

		/* side-to-side */

		local_maxima = TRUE;

		for ( i = 1; i < pad_size; i++ ) {
		    if ( ( center_value <
				DEVA_image_data ( values, row, col-i ) ) ||
			    ( center_value <=
			      DEVA_image_data ( values, row, col+i ) ) ) {
			local_maxima = FALSE;
			break;
		    }
		}

		if ( local_maxima ) {
		    DEVA_image_data ( directional_maxima, row, col ) = TRUE;
		    continue;
		}

#ifdef EIGHT_CONNECTED

		/* negative diagonal */

		local_maxima = TRUE;

		for ( i = 1; i < pad_size; i++ ) {
		    if ( ( center_value >
				DEVA_image_data ( values, row-i, col-i ) ) ||
			    ( center_value >=
			      DEVA_image_data ( values, row+i, col+i ) ) ) {
			local_maxima = FALSE;
			break;
		    }
		}

		if ( local_maxima ) {
		    DEVA_image_data ( directional_maxima, row, col ) = TRUE;
		    continue;
		}

		/* positive diagonal */

		local_maxima = TRUE;

		for ( i = 1; i < pad_size; i++ ) {
		    if ( ( center_value <	
				DEVA_image_data ( values, row-i, col+i ) ) ||
			    ( center_value <=
			      DEVA_image_data ( values, row+i, col-i ) ) ) {
			local_maxima = FALSE;
			break;
		    }
		}

		if ( local_maxima ) {
		    DEVA_image_data ( directional_maxima, row, col ) = TRUE;
		    continue;
		}

#endif	/* EIGHT_CONNECTED */

	    }
	}
    }

    return ( directional_maxima );
}

DEVA_float_image *
gblur_3x3 ( DEVA_float_image *values )
/*
 * 3x3 Gaussian blur of float values, with sigma=0.5.
 * Kernel values from <http://dev.theomader.com/gaussian-kernel-calculator/>.
 */
{
    DEVA_float_image	*blurred_image;
    float		kernel[3][3] = {
	{0.024879, 0.107973, 0.024879},
	{0.107973, 0.468592, 0.107973},
	{0.024879, 0.107973, 0.024879}
    };
    int			n_rows, n_cols;
    int			row, col;
    int			i, j;
    double		sum;

    n_rows = DEVA_image_n_rows ( values );
    n_cols = DEVA_image_n_cols ( values );
    if ( ( n_rows < 3 ) || ( n_cols < 3 ) ) {
	fprintf ( stderr, "gblur_3x3: image too small!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    blurred_image = DEVA_float_image_new ( n_rows, n_cols );

    for ( row = 1; row < n_rows - 1; row++ ) {
	for ( col = 1; col < n_cols -1; col++ ) {
	    sum = 0.0;
	    for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
		    sum += kernel[i][j] *
			DEVA_image_data ( values, row-1+i, col-1+j );
		}
	    }

	    DEVA_image_data ( blurred_image, row, col ) = sum;
	}
    }

    for ( row = 1; row < n_rows - 1; row++ ) {
	DEVA_image_data ( blurred_image, row, 0 ) = 
	    DEVA_image_data ( blurred_image, row, 1 );
	DEVA_image_data ( blurred_image, row, n_cols-1 ) =
	    DEVA_image_data ( blurred_image, row, n_cols-2 );
    }

    for ( col = 1; col < n_cols -1; col++ ) {
	DEVA_image_data ( blurred_image, 0, col ) =
	    DEVA_image_data ( blurred_image, 1, col );
	DEVA_image_data ( blurred_image, n_rows-1, col ) =
	    DEVA_image_data ( blurred_image, n_rows-2, col );
    }

    DEVA_image_data ( blurred_image, 0, 0 ) =
	DEVA_image_data ( blurred_image, 1, 1 );
    DEVA_image_data ( blurred_image, 0, n_cols - 1 ) =
	DEVA_image_data ( blurred_image, 1, n_cols - 2 );
    DEVA_image_data ( blurred_image, n_rows - 1 , 0 ) =
	DEVA_image_data ( blurred_image, n_rows - 2, 1 );
    DEVA_image_data ( blurred_image, n_rows - 1 , n_cols - 1 ) =
	DEVA_image_data ( blurred_image, n_rows - 2, n_cols - 2 );

    return ( blurred_image );
}

DEVA_XYZ_image *
gblur_3x3_3d ( DEVA_XYZ_image *values )
/*
 * 3x3 Gaussian blur of XYZ (3 x float) values, with sigma=0.5.
 * Kernel values from <http://dev.theomader.com/gaussian-kernel-calculator/>.
 */
{
    DEVA_XYZ_image	*blurred_image;
    double		kernel[3][3] = {
	{0.024879, 0.107973, 0.024879},
	{0.107973, 0.468592, 0.107973},
	{0.024879, 0.107973, 0.024879}
    };
    int			n_rows, n_cols;
    int			row, col;
    int			i, j;
    double		sum_X, sum_Y, sum_Z;	/* no double XYZ format */
    DEVA_XYZ		smoothed_value;

    n_rows = DEVA_image_n_rows ( values );
    n_cols = DEVA_image_n_cols ( values );
    if ( ( n_rows < 3 ) || ( n_cols < 3 ) ) {
	fprintf ( stderr, "gblur_3x3: image too small!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    blurred_image = DEVA_XYZ_image_new ( n_rows, n_cols );

    for ( row = 1; row < n_rows - 1; row++ ) {
	for ( col = 1; col < n_cols -1; col++ ) {
	    sum_X = sum_Y = sum_Z = 0.0;
	    for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
		    sum_X += kernel[i][j] *
			DEVA_image_data ( values, row-1+i, col-1+j ) . X;
		    sum_Y += kernel[i][j] *
			DEVA_image_data ( values, row-1+i, col-1+j ) . Y;
		    sum_Z += kernel[i][j] *
			DEVA_image_data ( values, row-1+i, col-1+j ) . Z;
		}
	    }

	    smoothed_value.X = sum_X;
	    smoothed_value.Y = sum_Y;
	    smoothed_value.Z = sum_Z;

	    DEVA_image_data ( blurred_image, row, col ) = smoothed_value;

	}
    }

    for ( row = 1; row < n_rows - 1; row++ ) {
	DEVA_image_data ( blurred_image, row, 0 ) = 
	    DEVA_image_data ( blurred_image, row, 1 );
	DEVA_image_data ( blurred_image, row, n_cols-1 ) =
	    DEVA_image_data ( blurred_image, row, n_cols-2 );
    }

    for ( col = 1; col < n_cols -1; col++ ) {
	DEVA_image_data ( blurred_image, 0, col ) =
	    DEVA_image_data ( blurred_image, 1, col );
	DEVA_image_data ( blurred_image, n_rows-1, col ) =
	    DEVA_image_data ( blurred_image, n_rows-2, col );
    }

    DEVA_image_data ( blurred_image, 0, 0 ) =
	DEVA_image_data ( blurred_image, 1, 1 );
    DEVA_image_data ( blurred_image, 0, n_cols - 1 ) =
	DEVA_image_data ( blurred_image, 1, n_cols - 2 );
    DEVA_image_data ( blurred_image, n_rows - 1 , 0 ) =
	DEVA_image_data ( blurred_image, n_rows - 2, 1 );
    DEVA_image_data ( blurred_image, n_rows - 1 , n_cols - 1 ) =
	DEVA_image_data ( blurred_image, n_rows - 2, n_cols - 2 );

    return ( blurred_image );
}
