/*
 * 2-D Gaussian blur of floating point values.
 * Convolution is done using fftw.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "deva-gblur-fft.h"
#include "deva-image.h"
#include <fftw3.h>
#include "deva-utils.h"

#define	SQR(x)	((x) * (x))

static DEVA_float_image	*generate_gaussian_kernel ( int n_rows, int n_cols,
			    double st_dev );
static double		gaussian ( double r_sq, double st_dev );
static void		apply_weights ( DEVA_complexf_image *transformed_image,
			    DEVA_complexf_image *transformed_kernel );
static int		DEVA_float_image_samesize_local
			    ( DEVA_float_image *input,
				DEVA_float_image *output);
			/* there is also a samesize routine in DEVA-utils */
static int		DEVA_complexf_image_samesize_local
			    ( DEVA_complexf_image *input,
					DEVA_complexf_image *output);
static DEVA_complexf	cxc ( DEVA_complexf a, DEVA_complexf b );

DEVA_float_image  *DEVA_float_gblur_fft ( DEVA_float_image *input, float st_dev )
/*
 * Convolve input image with Gaussian of specified standard deviation,
 * returning result in a new output image.
 *
 * Some performance advantages may be achieved if input is allocated using
 * DEVA_float_image_new.
 */
{
    DEVA_float_image  *output;

    output = DEVA_float_image_new ( DEVA_image_n_rows ( input ),
	    DEVA_image_n_cols ( input ) );

    DEVA_float_gblur2_fft ( input, output, st_dev );

    return ( output );
}

void
DEVA_float_gblur2_fft ( DEVA_float_image *input, DEVA_float_image *output,
	float st_dev )
/*
 * Convolve input image with Gaussian of specified standard deviation,
 * returning result in a preallocated output image of the same size.
 *
 * Some performance advantages may be achieved if input and output are
 * allocated using DEVA_float_image_new.
 */
{
    int			n_rows_input, n_cols_input;
    int			n_rows_transform, n_cols_transform;
    DEVA_float_image	*gaussian_kernel;
    DEVA_complexf_image	*transformed_kernel;
    DEVA_complexf_image	*transformed_image;
    fftwf_plan		fft_plan_input;		/* also use this for kernel */
    fftwf_plan		fft_plan_inverse;

    int			row, col;
    float		norm;

    if ( st_dev < STD_DEV_MIN ) {
	fprintf ( stderr,
		"DEVA_float_gblur2_fft: st_dev too small to use (%g)\n",
		    st_dev );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );	/* can't blur this little! */
    }

    if ( !DEVA_float_image_samesize_local ( input, output ) ) {
	fprintf ( stderr,
	 "DEVA_float_gblur2_fft: input and output image sizes don't match!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    n_rows_input = DEVA_image_n_rows ( input );
    n_cols_input = DEVA_image_n_cols ( input );

    n_rows_transform = n_rows_input;
    n_cols_transform = ( n_cols_input / 2 ) + 1;

    transformed_kernel =
	DEVA_complexf_image_new ( n_rows_transform, n_cols_transform );
    		/* if possible, use fft3w allocator */

    transformed_image =
	DEVA_complexf_image_new ( n_rows_transform, n_cols_transform );
    		/* if possible, use fft3w allocator */

    fft_plan_input = fftwf_plan_dft_r2c_2d ( n_rows_input, n_cols_input,
	    &DEVA_image_data ( input, 0, 0 ),
	    (fftwf_complex *) &DEVA_image_data ( transformed_image, 0, 0 ),
	    FFTW_ESTIMATE );

    fftwf_execute ( fft_plan_input );

    /*
     * Generate a space domain Gaussian kernel with the specified standard
     * deviation and then convolve this with the input image using
     * multiplication in the frequency domain.  Some speedup might be
     * achieved if the transform of the kernel was replaced by an
     * anylitically computed transformed kernel.
     */
    gaussian_kernel = generate_gaussian_kernel ( n_rows_input, n_cols_input,
	    st_dev );

    /* repurpose fft_plan_input for use with Gaussian kernel */
    fftwf_execute_dft_r2c ( fft_plan_input,
	    &DEVA_image_data ( gaussian_kernel, 0, 0 ),
	    (fftwf_complex *) &DEVA_image_data ( transformed_kernel, 0, 0 ) );

    apply_weights ( transformed_image, transformed_kernel );

    fft_plan_inverse = fftwf_plan_dft_c2r_2d ( n_rows_input, n_cols_input,
	    (fftwf_complex *) &DEVA_image_data ( transformed_image, 0, 0 ),
	    &DEVA_image_data ( output, 0, 0), FFTW_ESTIMATE );

    fftwf_execute ( fft_plan_inverse );

    norm = 1.0 / (double) ( n_rows_input * n_cols_input );
    for ( row = 0; row < n_rows_input; row++ ) {
	for ( col = 0; col < n_cols_input; col++ ) {
	    DEVA_image_data ( output, row, col ) *= norm;
	}
    }

    fftwf_destroy_plan ( fft_plan_input );
    fftwf_destroy_plan ( fft_plan_inverse );
    fftwf_cleanup ( );
    DEVA_float_image_delete ( gaussian_kernel );
    DEVA_complexf_image_delete ( transformed_kernel );
    DEVA_complexf_image_delete ( transformed_image );
}

void
DEVA_gblur_fft_destroy ( void ) {
    /*
     * Do nothing (included for compatibility with gblur (...) ).
     */
}

static DEVA_float_image *
generate_gaussian_kernel ( int n_rows, int n_cols, double st_dev )
/*
 * Generate Gaussian kernel centered at [0][0].
 */
{
    DEVA_float_image  *gaussian_kernel;
    int		    row, col;
    int		    n_rows_reflect, n_cols_reflect;
    double	    r_sqr;

    gaussian_kernel = DEVA_float_image_new ( n_rows, n_cols );
    n_rows_reflect = ( n_rows + 1 ) / 2;
    n_cols_reflect = ( n_cols + 1 ) / 2;

    /*
     * Could speed things up a bit by using symmetry to reduce calls to
     * exp ( ... ).
     */

    /* first row is special because it contains the origin */
    row = 0;
    for ( col = 0; col < n_cols_reflect; col++ ) {
	r_sqr = SQR ( (double) col );
	DEVA_image_data ( gaussian_kernel, row, col ) =
	    gaussian ( r_sqr, st_dev );
    }
    for ( col = n_cols_reflect; col < n_cols; col++ ) {
	r_sqr = SQR ( (double) ( n_cols - col ) );
	DEVA_image_data ( gaussian_kernel, row, col ) =
	    gaussian ( r_sqr, st_dev );
    }

    /* remainder of upper rows */
    for ( row = 1; row < n_rows_reflect; row++ ) {
	for ( col = 0; col < n_cols_reflect; col++ ) {
	    r_sqr = SQR ( (double) row ) + SQR ( (double) col );
	    DEVA_image_data ( gaussian_kernel, row, col ) = gaussian ( r_sqr, st_dev );
	}
	for ( col = n_cols_reflect; col < n_cols; col++ ) {
	    r_sqr = SQR ( (double) row ) + SQR ( (double) ( n_cols - col ) );
	    DEVA_image_data ( gaussian_kernel, row, col ) =
		gaussian ( r_sqr, st_dev );
	}
    }

    /* lower rows */
    for ( row = n_rows_reflect; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols_reflect; col++ ) {
	    r_sqr = SQR ( (double) (n_rows - row ) ) + SQR ( (double) col );
	    DEVA_image_data ( gaussian_kernel, row, col ) =
		gaussian ( r_sqr, st_dev );
	}
	for ( col = n_cols_reflect; col < n_cols; col++ ) {
	    r_sqr = SQR ( (double) (n_rows - row ) ) +
					SQR ( (double) ( n_cols - col ) );
	    DEVA_image_data ( gaussian_kernel, row, col ) =
		gaussian ( r_sqr, st_dev );
	}
    }

    return ( gaussian_kernel );
}

static double
gaussian ( double r_sqr, double st_dev )
/*
 * Coefficients of symmetric bivariate normal distribution.
 */
{
    return ( ( 1.0 / ( 2.0 * M_PI * st_dev * st_dev ) ) *
	    exp ( -r_sqr / ( 2.0 * SQR ( st_dev ) ) ) );
}

void
apply_weights ( DEVA_complexf_image *transformed_image,
	DEVA_complexf_image *transformed_kernel )
{
    int	    row, col;
    int	    n_rows, n_cols;

    if ( !DEVA_complexf_image_samesize_local ( transformed_image,
		transformed_kernel ) ) {
	fprintf ( stderr, "apply_weights: image sizes don't match!\n" );
	DEVA_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    n_rows = DEVA_image_n_rows ( transformed_image );
    n_cols = DEVA_image_n_cols ( transformed_image );

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    DEVA_image_data ( transformed_image, row, col ) =
		cxc ( DEVA_image_data ( transformed_image, row, col ),
			DEVA_image_data ( transformed_kernel, row, col ) );
	}
    }
}

static int
DEVA_float_image_samesize_local ( DEVA_float_image *input,
	DEVA_float_image *output )
{
    return ( ( DEVA_image_n_rows ( input ) == DEVA_image_n_rows ( output ) ) &&
	    ( DEVA_image_n_cols ( input ) == DEVA_image_n_cols ( output ) ) );
}

static int
DEVA_complexf_image_samesize_local ( DEVA_complexf_image *input,
	DEVA_complexf_image *output)
{
    return ( ( DEVA_image_n_rows ( input ) == DEVA_image_n_rows ( output ) ) &&
	    ( DEVA_image_n_cols ( input ) == DEVA_image_n_cols ( output ) ) );
}

static DEVA_complexf
cxc ( DEVA_complexf a, DEVA_complexf b )
/*
 * Multiply two complex numbers
 */
{
    DEVA_complexf	product;

    product.real = ( ( a.real * b.real ) - ( a.imaginary * b.imaginary ) );
    product.imaginary = ( ( a.real * b.imaginary ) + ( a.imaginary * b.real ) );

    return ( product );
}
