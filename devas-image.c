/*
 * Establishes data types for a variety of fixed point and floating point
 * image pixel types.
 *
 * Defines dynamically allocatable 2-D image arrays for each of these types.
 * Image arrays have additional properties associated with them of use in
 * the devas-filter.
 *
 * Images are implemented as objects that can be created, destroyed, and
 * operated on by a variety of methods.
 *
 * Pixel values are strongly typed. Elements of multi-dimensional pixels
 * types are indexed by name, not by a  numeric array index.
 *
 * Supported pixel types:
 *
 *   DeVAS_gray      8 bit unsigned grayscale
 *   DeVAS_float     32 bit float
 *   DeVAS_double    64 bit float
 *   DeVAS_RGB       3 x 8 bit RGB
 *   DeVAS_RGBf      3 x 32 bit float RGB
 *   DeVAS_XYZ       3 x 32 bit float CIE XYC
 *   DeVAS_xyY       3 x 32 bit float CIE xyY
 *   DeVAS_complexf  32 bit float complex
 *   DeVAS_complexd  64 bit double complex
 *
 * To create an image object:
 *
 *   <DeVAS_type>_image_new ( <n_rows>, <n_cols> )
 *
 *   	Note that arguments are (n_rows, n_cols), not (width, height) or
 *   	(x_dim, y_dim)!
 *
 * To destroy an image object:
 *
 *   <DeVAS_type>_image_delete ( <image_object> )
 *
 * Methods on image objects:
 *
 *   DeVAS_image_data ( <image_object>, <row>, <col> )
 *
 *   	This is a C l-value, meaning that it can be either assigned a value
 *   	or used in an expression.
 *
 *   	If the C pre-processor variable DeVAS_CHECK_BOUNDS is defined before
 *   	any direct or indirect inclusion of devas-image.h, bounds checking is
 *   	done for the row and column index values. (Safest is to do the define
 *   	before including *any* include files.)
 *
 *   DeVAS_image_n_rows ( <devas_image> )
 *
 *   DeVAS_image_n_cols ( <devas_image> )
 *
 *   DeVAS_image_samesize ( <devas_image_1>, <devas_image_2> )
 *
 *   					TRUE if images are the same dimensions.
 *
 *   DeVAS_image_info ( <devas_image> )	The DeVAS_Image_Info information
 *   					associated with the image object.
 *   					This includes a Radiance VIEW
 *   					structure and a description sting.
 *   					The DeVAS_Image_Info information may
 *   					be either set or used in an expression.
 *
 *   DeVAS_image_view ( <devas_image> )	The Radiance VIEW structure.  May be
 *   					either set or used in an expression.
 *
 *   DeVAS_image_description ( <devas_image> )
 *
 *   					The description string. May be
 *   					either set or used in an expression.
 *
 *   DeVAS_image_exposure_set ( <devas_image> )
 *
 *					TRUE if an exposure value has been set.
 *
 *   DeVAS_image_exposure ( <devas_image> )
 *
 * 					The exposure value (only valid if
 *					DeVAS_image_exposure_set is TRUE).
 */

/*
 * This routine requires linking in the Radiance routines color.c and
 * spec_rgb.c to provide transform matricies.  Portability would be enhanced
 * if we simply copied over the needed Radiance values, instead of having to
 * include and compile full Radiance routines.
 */

#include <stdlib.h>
#include <assert.h>
#include "devas-image.h"
#include "devas-license.h"	/* DeVAS open source license */
#include "radiance/color.h"

/*
 * RGBf (floating point RGB) values are scaled as for rgbe-format Radiance
 * files (watts/steradian/sq.meter over the visible spectrum).
 *
 * XYZ (floating point CIE XYZ) values and the Y in xyY (floating point CIE
 * xyY) are scaled as for xyze-format Radiance files (candela/m^2).
 */

DeVAS_xyY
DeVAS_XYZ2xyY ( DeVAS_XYZ XYZ )
{
    DeVAS_xyY	xyY;
    float	norm;

    norm = XYZ.X + XYZ.Y + XYZ.Z;
    if ( norm <= 0.0 ) {	/* add epsilon tolerance? */
	xyY.x = DeVAS_x_WHITEPOINT;
	xyY.y = DeVAS_y_WHITEPOINT;
	xyY.Y = 0.0;
    } else {
	xyY.x = XYZ.X / norm;
	xyY.y = XYZ.Y / norm;
	xyY.Y = XYZ.Y;
    }

    return ( xyY );
}

DeVAS_XYZ
DeVAS_xyY2XYZ ( DeVAS_xyY xyY )
{
    DeVAS_XYZ	XYZ;

    if ( xyY.y <= 0.0 ) {	/* add epsilon tolerance? */
	XYZ.X = XYZ.Y = XYZ.Z = 0.0;
    } else {
	XYZ.X = ( xyY.x * xyY.Y ) / xyY.y;
	XYZ.Y = xyY.Y;
	XYZ.Z = ( ( 1.0 - xyY.x - xyY.y ) * xyY.Y ) / xyY.y;
    }

    return ( XYZ );
}

DeVAS_RGBf
DeVAS_XYZ2RGBf ( DeVAS_XYZ XYZ )
/* Use Radiance definition of RGB primaries. */
{
    DeVAS_RGBf	RGBf;
    COLOR	rad_rgb, rad_xyz;

    colval ( rad_xyz, CIEX ) = XYZ.X;
    colval ( rad_xyz, CIEY ) = XYZ.Y;
    colval ( rad_xyz, CIEZ ) = XYZ.Z;

    colortrans ( rad_rgb, xyz2rgbmat, rad_xyz );

    RGBf.red = colval ( rad_rgb, RED ) / WHTEFFICACY;
    RGBf.green = colval ( rad_rgb,GRN ) / WHTEFFICACY;
    RGBf.blue = colval ( rad_rgb,BLU ) / WHTEFFICACY;

    return ( RGBf );
}

DeVAS_XYZ
DeVAS_RGBf2XYZ ( DeVAS_RGBf RGBf )
/* Use Radiance definition of RGB primaries. */
{
    DeVAS_XYZ	XYZ;
    COLOR	rad_rgb, rad_xyz;

    colval ( rad_rgb, RED ) = RGBf.red;
    colval ( rad_rgb, GRN ) = RGBf.green;
    colval ( rad_rgb, BLU ) = RGBf.blue;

    colortrans ( rad_xyz, rgb2xyzmat, rad_rgb );

    XYZ.X = colval ( rad_xyz, CIEX ) * WHTEFFICACY;
    XYZ.Y = colval ( rad_xyz, CIEY ) * WHTEFFICACY;
    XYZ.Z = colval ( rad_xyz, CIEZ ) * WHTEFFICACY;

    return ( XYZ );
}

DeVAS_RGBf
DeVAS_xyY2RGBf ( DeVAS_xyY xyY )
/* Use Radiance definition of RGB primaries. */
{
    return ( DeVAS_XYZ2RGBf ( DeVAS_xyY2XYZ ( xyY ) ) );
}

DeVAS_xyY
DeVAS_RGBf2xyY ( DeVAS_RGBf RGBf )
/* Use Radiance definition of RGB primaries. */
{
    return ( DeVAS_XYZ2xyY ( DeVAS_RGBf2XYZ ( RGBf ) ) );
}

DeVAS_RGBf
DeVAS_Y2RGBf ( DeVAS_float Y )
{
    DeVAS_RGBf	RGBf;

    RGBf.red = RGBf.green = RGBf.blue = Y / WHTEFFICACY;

    return ( RGBf );
}

float
DeVAS_RGBf2Y ( DeVAS_RGBf RGBf )
/* Use Radiance specification of RGB primaries. */
{
    COLOR	rad_rgb;

    colval ( rad_rgb, RED ) = RGBf.red;
    colval ( rad_rgb, GRN ) = RGBf.green;
    colval ( rad_rgb, BLU ) = RGBf.blue;

    return ( luminance ( rad_rgb ) );
}

#define DeVAS_IMAGE_NEW( TYPE )						\
TYPE##_image *								\
TYPE##_image_new ( unsigned int n_rows, unsigned int n_cols  )		\
{									\
    TYPE##_image    *new_image;						\
    int		    row;						\
    TYPE	    **line_pointers;					\
    VIEW	    nullview = NULLVIEW;				\
									\
    new_image = ( TYPE##_image * ) malloc ( sizeof ( TYPE##_image ) );	\
    if ( new_image == NULL ) {						\
	fprintf ( stderr, "DeVAS_image_new: malloc failed!" );		\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    new_image->n_rows = n_rows;						\
    new_image->n_cols = n_cols;						\
									\
    new_image->exposure_set = FALSE;					\
    new_image->exposure = 1.0;	/* default value */			\
									\
    new_image->image_info.view = nullview;				\
    new_image->image_info.description = NULL;				\
									\
    new_image->start_data = (TYPE *) malloc ( n_rows * n_cols *		\
	        sizeof ( TYPE ) );					\
    if ( new_image->start_data == NULL ) {				\
	fprintf ( stderr, "DeVAS_image_new: malloc failed!" );		\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    line_pointers = (TYPE **) malloc ( n_rows * sizeof ( TYPE * ) );	\
    if ( line_pointers == NULL ) {					\
	fprintf ( stderr, "DeVAS_image_new: malloc failed!" );		\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    for ( row = 0; row < n_rows; row++ ) {				\
	line_pointers[row] = new_image->start_data + ( row * n_cols );	\
    }									\
									\
    new_image->data = &line_pointers[0];				\
									\
    return ( new_image );						\
}

#define DeVAS_IMAGE_NEW_FFTW_SINGLE( TYPE )				\
TYPE##_image *								\
TYPE##_image_new ( unsigned int n_rows, unsigned int n_cols  )		\
{									\
    TYPE##_image    *new_image;						\
    int		    row;						\
    TYPE	    **line_pointers;					\
    VIEW	    nullview = NULLVIEW;				\
									\
    new_image = ( TYPE##_image * ) malloc ( sizeof ( TYPE##_image ) );	\
    if ( new_image == NULL ) {						\
	fprintf ( stderr, "DeVAS_image_new: malloc failed!" );		\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    assert ( sizeof ( DeVAS_complexf ) == sizeof ( float [2] ) );	\
    	/* make sure padding doesn't cause problems */			\
									\
    new_image->n_rows = n_rows;						\
    new_image->n_cols = n_cols;						\
									\
    new_image->exposure_set = FALSE;					\
    new_image->exposure = 1.0;	/* default value */			\
									\
    new_image->image_info.view = nullview;				\
    new_image->image_info.description = NULL;				\
									\
    new_image->start_data = (TYPE *) fftwf_malloc ( n_rows * n_cols *	\
	        sizeof ( TYPE ) );					\
    if ( new_image->start_data == NULL ) {				\
	fprintf ( stderr, "DeVAS_image_new: malloc failed!" );		\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    line_pointers = (TYPE **) malloc ( n_rows * sizeof ( TYPE * ) );	\
    if ( line_pointers == NULL ) {					\
	fprintf ( stderr, "DeVAS_image_new: malloc failed!" );		\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    for ( row = 0; row < n_rows; row++ ) {				\
	line_pointers[row] = new_image->start_data + ( row * n_cols );	\
    }									\
									\
    new_image->data = &line_pointers[0];				\
									\
    return ( new_image );						\
}

DeVAS_IMAGE_NEW ( DeVAS_gray )
DeVAS_IMAGE_NEW ( DeVAS_double )
DeVAS_IMAGE_NEW ( DeVAS_RGB )
DeVAS_IMAGE_NEW ( DeVAS_RGBf )
DeVAS_IMAGE_NEW ( DeVAS_XYZ )
DeVAS_IMAGE_NEW ( DeVAS_xyY )
#ifndef	DeVAS_USE_FFTW3_ALLOCATORS
DeVAS_IMAGE_NEW ( DeVAS_float )
DeVAS_IMAGE_NEW ( DeVAS_complexf )
/* DeVAS_IMAGE_NEW ( DeVAS_complexd ) */
#else
DeVAS_IMAGE_NEW_FFTW_SINGLE ( DeVAS_float )
DeVAS_IMAGE_NEW_FFTW_SINGLE ( DeVAS_complexf )
/* DeVAS_IMAGE_NEW_FFTW_DOUBLE ( DeVAS_complexd ) */
#endif	/* DeVAS_USE_FFTW3_ALLOCATORS */

#define DeVAS_IMAGE_DELETE( TYPE )					\
void									\
TYPE##_image_delete ( TYPE##_image *image )				\
{									\
    if ( image == NULL ) {						\
	fprintf ( stderr,						\
		"Attempt to delete empty image object (warning)\n" );	\
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );			\
	return;								\
    }									\
    if ( image->image_info.description != NULL ) {			\
	free ( image->image_info.description );				\
	image->image_info.description = NULL;				\
    }									\
    free ( image->start_data );						\
    free ( image->data );						\
    free ( image );							\
}

#define DeVAS_IMAGE_DELETE_FFTW_SINGLE( TYPE )				\
void									\
TYPE##_image_delete ( TYPE##_image *image )				\
{									\
    if ( image->image_info.description != NULL ) {			\
	free ( image->image_info.description );				\
	image->image_info.description = NULL;				\
    }									\
    fftwf_free ( image->start_data );					\
    free ( image->data );						\
    free ( image );							\
}

DeVAS_IMAGE_DELETE ( DeVAS_gray )
DeVAS_IMAGE_DELETE ( DeVAS_double )
DeVAS_IMAGE_DELETE ( DeVAS_RGB )
DeVAS_IMAGE_DELETE ( DeVAS_RGBf )
DeVAS_IMAGE_DELETE ( DeVAS_XYZ )
DeVAS_IMAGE_DELETE ( DeVAS_xyY )
#ifndef	DeVAS_USE_FFTW3_ALLOCATORS
DeVAS_IMAGE_DELETE ( DeVAS_float )
DeVAS_IMAGE_DELETE ( DeVAS_complexf )
/* DeVAS_IMAGE_DELETE ( DeVAS_complexd ) */
#else
DeVAS_IMAGE_DELETE_FFTW_SINGLE ( DeVAS_float )
DeVAS_IMAGE_DELETE_FFTW_SINGLE ( DeVAS_complexf )
/* DeVAS_IMAGE_DELETE_FFTW_SINGLE ( DeVAS_complexd ) */
#endif	/* DeVAS_USE_FFTW3_ALLOCATORS */

void
DeVAS_image_check_bounds ( DeVAS_gray_image *devas_image, int row, int col,
       int line, char *file )
{
    if ( ( row < 0 ) || ( row >= DeVAS_image_n_rows ( devas_image ) ) ||
	    ( col < 0 ) || ( col >= DeVAS_image_n_cols ( devas_image ) ) ) {
	fprintf ( stderr, "DeVAS_image_data out-of-bounds reference!\n" );
	DeVAS_print_file_lineno ( file, line );
	exit ( EXIT_FAILURE );
    }
}

void
DeVAS_print_file_lineno ( char *file, int line )
{
    fprintf ( stderr, "line %d in file %s\n", line, file );
}

#define	DeVAS_IMAGE_SAMESIZE( TYPE )					\
int									\
TYPE##_image_samesize ( TYPE##_image *i1, TYPE##_image *i2 )		\
{									\
    return ( ( DeVAS_image_n_rows ( i1 ) == DeVAS_image_n_rows ( i2 ) ) && \
	    ( DeVAS_image_n_cols ( i1 ) == DeVAS_image_n_cols ( i2 ) ) ); \
}

DeVAS_IMAGE_SAMESIZE ( DeVAS_gray )
DeVAS_IMAGE_SAMESIZE ( DeVAS_double )
DeVAS_IMAGE_SAMESIZE ( DeVAS_RGB )
DeVAS_IMAGE_SAMESIZE ( DeVAS_RGBf )
DeVAS_IMAGE_SAMESIZE ( DeVAS_XYZ )
DeVAS_IMAGE_SAMESIZE ( DeVAS_xyY )
DeVAS_IMAGE_SAMESIZE ( DeVAS_float )
DeVAS_IMAGE_SAMESIZE ( DeVAS_complexf )
/* DeVAS_IMAGE_SAMESIZE ( DeVAS_complexd ) */

#define	DeVAS_IMAGE_SETVALUE( TYPE )					\
void									\
TYPE##_image_setvalue ( TYPE##_image *image, TYPE value )		\
/*									\
 * Set every pixel to a given value.					\
 */									\
{									\
    int	    row, col;							\
									\
    for ( row = 0; row < DeVAS_image_n_rows ( image ); row++ ) {	\
	for ( col = 0; col < DeVAS_image_n_cols ( image ); col++ ) {	\
	    DeVAS_image_data ( image, row, col ) = value;		\
	}								\
    }									\
}

DeVAS_IMAGE_SETVALUE ( DeVAS_gray )
DeVAS_IMAGE_SETVALUE ( DeVAS_double )
DeVAS_IMAGE_SETVALUE ( DeVAS_RGB )
DeVAS_IMAGE_SETVALUE ( DeVAS_RGBf )
DeVAS_IMAGE_SETVALUE ( DeVAS_XYZ )
DeVAS_IMAGE_SETVALUE ( DeVAS_xyY )
DeVAS_IMAGE_SETVALUE ( DeVAS_float )
DeVAS_IMAGE_SETVALUE ( DeVAS_complexf )
/* DeVAS_IMAGE_SETVALUE ( DeVAS_complexd ) */
