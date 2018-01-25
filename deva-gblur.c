/*
 * 2-D Gaussian blur of floating point values.
 * Convolution is done using separable kernels.
 * Portion of kernel outside edges are is ignored.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "deva-gblur.h"
#include "deva-image.h"

#define	NDEBUG
#include <assert.h>

#define	K_SIZE_MULT	7.0	/* ratio of kernel size to standard deviation */
#define	K_SIZE_MIN	3	/* make kernel size at least this big */
#define	OVERSAMP	10	/* to get better convolution kernel */

static double   *save_normalize = NULL;	/* make this global so we can free */
					/* it when gblur no longer needed */

static void	gblur_float_convolve_1d ( DEVA_float *in, DEVA_float *out,
		    int in_size, float *kernel, int kernel_size, int first );
/***********************************************
 * In gblur.h, since some application routines may care
 * static int	DEVA_float_gblur_kernel_size ( float st_deviation );
***********************************************/
static int	imax ( int x, int y );
static int	imin ( int x, int y );
static float	*DEVA_float_gblur_kernel ( float st_deviation,
		    int kernel_size );

DEVA_float_image  *deva_float_gblur ( DEVA_float_image *input, float st_dev )
/*
 * Convolve input image with Gaussian of specified standard deviation,
 * returning result in a new output image.
 */
{
    DEVA_float_image  *output;

    output = DEVA_float_image_new ( DEVA_image_n_rows ( input ),
	    DEVA_image_n_cols ( input ) );

    deva_float_gblur2 ( input, output, st_dev );

    return ( output );
}

void
deva_float_gblur2 ( DEVA_float_image *input, DEVA_float_image *output,
	float st_dev )
/*
 * Convolve input image with Gaussian of specified standard deviation,
 * returning result in a preallocated output image of the same size.
 */
{
    int		n_rows, n_cols;
    float	*kernel;
    int		kernel_size;
    int		row, col;
    int		first;		/* to help optimize gblur_float_convolve_1d */
    DEVA_float	*tmp_1, *tmp_2;

    if ( st_dev < GBLUR_STD_DEV_MIN ) {
	fprintf ( stderr, "DEVA_float_gblur2: st_dev too small to use (%g)\n",
		st_dev );
	exit ( EXIT_FAILURE );	/* can't blur this little! */
    }

    if ( !DEVA_float_image_samesize ( input, output ) ) {
	fprintf ( stderr,
	    "DEVA_float_gblur2: input and output image sizes don't match!\n" );
	exit ( EXIT_FAILURE );
    }

    n_rows = DEVA_image_n_rows ( input );
    n_cols = DEVA_image_n_cols ( input );

    /* pre-compute kernel values */

    kernel_size = DEVA_float_gblur_kernel_size ( st_dev );
    kernel = DEVA_float_gblur_kernel ( st_dev, kernel_size );
    
    /*
     * Copy rows and columns of data to 1-D temporary vector to simplify
     * separable convolution code.  This adds very little time.
     */

    tmp_1 = (DEVA_float *) malloc ( n_cols * sizeof ( DEVA_float ) );
    if ( tmp_1 == NULL ) {
	fprintf ( stderr, "malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }
    tmp_2 = (DEVA_float *) malloc ( n_cols * sizeof ( DEVA_float ) );
    if ( tmp_2 == NULL ) {
	fprintf ( stderr, "malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    first = TRUE;
    /* blur rows */
    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    tmp_1[col] = DEVA_image_data ( input, row, col );
	}

	gblur_float_convolve_1d ( tmp_1, tmp_2, n_cols, kernel, kernel_size,
	       first );
	first = FALSE;

	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( output, row, col ) = tmp_2[col];
	}
    }

    /* blur columns */

    first = TRUE;
    if ( n_rows != n_cols ) {
	free ( tmp_1 );
	free ( tmp_2 );

	tmp_1 = (DEVA_float *) malloc ( n_rows * sizeof ( DEVA_float ) );
	if ( tmp_1 == NULL ) {
	    fprintf ( stderr, "malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
	tmp_2 = (DEVA_float *) malloc ( n_rows * sizeof ( DEVA_float ) );
	if ( tmp_2 == NULL ) {
	    fprintf ( stderr, "malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    for ( col = 0; col < n_cols; col++ ) {
	for ( row = 0; row < n_rows; row++ ) {
	    tmp_1[row] = DEVA_image_data ( output, row, col );
	}

	gblur_float_convolve_1d ( tmp_1, tmp_2, n_rows, kernel, kernel_size,
	       first );
	first = FALSE;

	for ( row = 0; row < n_rows; row++ ) {
	    DEVA_image_data ( output, row, col ) = tmp_2[row];
	}
    }

    free ( tmp_1 );
    free ( tmp_2 );
    free ( kernel );
}

static void
gblur_float_convolve_1d ( DEVA_float *in, DEVA_float *out, int in_size,
	float *kernel, int kernel_size, int first )
/*
 * 1-D arbitrary convolution on vectors.
 *
 * Kernel is assumed (without checking) to sum to 1.0.
 *
 * When kernel only partially overlaps input vector, only the overlapping
 * portions are used.  This requires determining the correct normalization
 * value.
 *
 * To be safe, normalization is computed for *all* positions of the kernel.
 * To save time, this is only done when the value of <first> is TRUE;
 */
{
    int		    i, j;
    int		    j_start, j_end;
    int		    half_kernel_size;
    double	    total;
    double	    normalize;

    if ( ( kernel_size % 2 ) != 1 ) {
	fprintf ( stderr,
		"gblur_float_convolve_1d: kernel size must be odd!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( first ) {
	if ( save_normalize != NULL ) {
	    free ( save_normalize );
	}
	save_normalize = (double *) malloc ( in_size * sizeof ( double ) );
	if ( save_normalize == NULL ) {
	    fprintf ( stderr, "gblur_float_convolve_1d: malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    half_kernel_size = ( kernel_size - 1 ) / 2;	/* kernel_size should be odd */

    for ( i = 0; i < in_size; i++ ) {
	j_start = imax ( 0, half_kernel_size - i );
	j_end = imin ( kernel_size, kernel_size - ( i + half_kernel_size -
		    in_size + 1) );
	total = 0.0;

	if ( first ) {
	    normalize = 0.0;
	    for ( j = j_start; j < j_end; j++ ) {
		assert ( ( i - half_kernel_size + j ) >= 0 );
		assert ( ( i - half_kernel_size + j ) < in_size );
		assert ( j >= 0 );
		assert ( j < kernel_size );
		total += kernel[j] * in[i - half_kernel_size + j];
		normalize += kernel[j];
	    }
	    assert ( normalize > 0.0 );
	    save_normalize[i] = normalize;
	} else {
	    for ( j = j_start; j < j_end; j++ ) {
		assert ( ( i - half_kernel_size + j ) >= 0 );
		assert ( ( i - half_kernel_size + j ) < in_size );
		assert ( j >= 0 );
		assert ( j < kernel_size );
		total += kernel[j] * in[i - half_kernel_size + j];
	    }
	}

	out[i] = total / save_normalize[i];
    }
}

/* static */ int /* expose this, since some application routines may care */
DEVA_float_gblur_kernel_size ( float st_deviation )
{
    int		kernel_size;

    kernel_size = imax ( (int) rint ( K_SIZE_MULT * st_deviation ), K_SIZE_MIN);

    /* odd is good... */
    if ( kernel_size % 2 == 0 )
	kernel_size++;

    return ( kernel_size );
}

static int
imax ( int x, int y )
{
    if ( x > y ) {
	return ( x );
    } else {
	return ( y );
    }
}

static int
imin ( int x, int y )
{
    if ( x < y ) {
	return ( x );
    } else {
	return ( y );
    }
}

static float *
DEVA_float_gblur_kernel ( float st_deviation, int kernel_size )
{
    int		i, i1, i2, j;
    double	radius;
    double	norm, two_sd_sq;
    double	x, xsq;
    double	f_k_sum;
    float	*kernel;
    double	inc, total;

    if ( kernel_size <= 0 ) {
	fprintf ( stderr,
		"fgaussian: invalid kernel_size (%i)\n", kernel_size );
	exit ( EXIT_FAILURE );
    }

    if ( st_deviation <= 0.0 ) {
	fprintf ( stderr,
		"fgaussian: invalid st_deviation (%f)\n", st_deviation );
	exit ( EXIT_FAILURE );
    }

    kernel = (float *) malloc ( kernel_size * sizeof ( float ) );
    if ( kernel == NULL ) {
	fprintf ( stderr, "malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    radius = 0.5 * ((float) ( kernel_size - 1 ) );

    /* Values will be renormalized, but might as well do it correctly */
    /* here too. */
    norm = 1.0 / (sqrt(2.0 * M_PI) * st_deviation);
    two_sd_sq = 2.0 * st_deviation * st_deviation;

    for ( i = 0; i <= radius; i++ ) {
	x = radius - ((double) i );
	xsq = x * x;

	i1 = i;
	i2 = kernel_size - i - 1;

	/* Do fancy numerical integration trick to */
	/* get better estimates. */

	inc = 1.0 / ( (double) OVERSAMP );
	total = 0.0;
	x = radius - ((double) i ) - 0.5 + ( inc / 2 );
	for ( j = 0; j < OVERSAMP; j++ ) {
	    xsq = x * x;
	    total += norm * exp ( -xsq / two_sd_sq );
	    x += inc;
	}

	kernel[i1] = kernel[i2] = total / (double) OVERSAMP;
    }

    /* Want sum of each kernel to be 1.0 */
    f_k_sum = 0.0;
    for ( i = 0; i < kernel_size; i++ ) {
	f_k_sum += kernel[i];
    }

    /* Produce normalized floating point kernels. */
    /* (Shouldn't be necessary!) */
    for ( i = 0; i < kernel_size; i++ ) {
	kernel[i] /= f_k_sum;
    }

    return ( kernel );
}

void
deva_gblur_destroy ( void )
/*
 * Free resources save across calls.
 */
{
    if ( save_normalize != NULL ) {
	free ( save_normalize );
    }

    save_normalize = NULL;
}
