/*
 * Establishes data types for a variety of fixed point and floating point
 * image pixel types.
 *
 * Defines dynamically allocatable 2-D arrays for each of these types.
 * Arrays have additional properties associated with them of use in the
 * deva-filter.
 */

#include <stdlib.h>
#include <assert.h>
#include "deva-image.h"
#include "deva-license.h"	/* DEVA open source license */
#include "radiance/color.h"

/*
 * RGBf (floating point RGB) values are scaled as for rgbe-format Radiance
 * files (watts/steradian/sq.meter over the visible spectrum).
 *
 * XYZ (floating point CIE XYZ) values and the Y in xyY (floating point CIE
 * xyY) are scaled as for xyze-format Radiance files (candela/m^2).
 */

DEVA_xyY
DEVA_XYZ2xyY ( DEVA_XYZ XYZ )
{
    DEVA_xyY	xyY;
    float	norm;

    norm = XYZ.X + XYZ.Y + XYZ.Z;
    if ( norm <= 0.0 ) {	/* add epsilon tolerance? */
	xyY.x = DEVA_x_WHITEPOINT;
	xyY.y = DEVA_y_WHITEPOINT;
	xyY.Y = 0.0;
    } else {
	xyY.x = XYZ.X / norm;
	xyY.y = XYZ.Y / norm;
	xyY.Y = XYZ.Y;
    }

    return ( xyY );
}

DEVA_XYZ
DEVA_xyY2XYZ ( DEVA_xyY xyY )
{
    DEVA_XYZ	XYZ;

    if ( xyY.y <= 0.0 ) {	/* add epsilon tolerance? */
	XYZ.X = XYZ.Y = XYZ.Z = 0.0;
    } else {
	XYZ.X = ( xyY.x * xyY.Y ) / xyY.y;
	XYZ.Y = xyY.Y;
	XYZ.Z = ( ( 1.0 - xyY.x - xyY.y ) * xyY.Y ) / xyY.y;
    }

    return ( XYZ );
}

DEVA_RGBf
DEVA_XYZ2RGBf ( DEVA_XYZ XYZ )
/* Use Radiance definition of RGB primaries. */
{
    DEVA_RGBf	RGBf;
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

DEVA_XYZ
DEVA_RGBf2XYZ ( DEVA_RGBf RGBf )
/* Use Radiance definition of RGB primaries. */
{
    DEVA_XYZ	XYZ;
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

DEVA_RGBf
DEVA_xyY2RGBf ( DEVA_xyY xyY )
/* Use Radiance definition of RGB primaries. */
{
    return ( DEVA_XYZ2RGBf ( DEVA_xyY2XYZ ( xyY ) ) );
}

DEVA_xyY
DEVA_RGBf2xyY ( DEVA_RGBf RGBf )
/* Use Radiance definition of RGB primaries. */
{
    return ( DEVA_XYZ2xyY ( DEVA_RGBf2XYZ ( RGBf ) ) );
}

DEVA_RGBf
DEVA_Y2RGBf ( DEVA_float Y )
{
    DEVA_RGBf	RGBf;

    RGBf.red = RGBf.green = RGBf.blue = Y / WHTEFFICACY;

    return ( RGBf );
}

float
DEVA_RGBf2Y ( DEVA_RGBf RGBf )
/* Use Radiance specification of RGB primaries. */
{
    COLOR	rad_rgb;

    colval ( rad_rgb, RED ) = RGBf.red;
    colval ( rad_rgb, GRN ) = RGBf.green;
    colval ( rad_rgb, BLU ) = RGBf.blue;

    return ( luminance ( rad_rgb ) );
}

#define DEVA_IMAGE_NEW( TYPE )						\
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
	fprintf ( stderr, "DEVA_image_new: malloc failed!" );		\
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
	fprintf ( stderr, "DEVA_image_new: malloc failed!" );		\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    line_pointers = (TYPE **) malloc ( n_rows * sizeof ( TYPE * ) );	\
    if ( line_pointers == NULL ) {					\
	fprintf ( stderr, "DEVA_image_new: malloc failed!" );	\
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

#define DEVA_IMAGE_NEW_FFTW_SINGLE( TYPE )				\
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
	fprintf ( stderr, "DEVA_image_new: malloc failed!" );		\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    assert ( sizeof ( DEVA_complexf ) == sizeof ( float [2] ) );	\
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
	fprintf ( stderr, "DEVA_image_new: malloc failed!" );	\
        exit ( EXIT_FAILURE );						\
    }									\
									\
    line_pointers = (TYPE **) malloc ( n_rows * sizeof ( TYPE * ) );	\
    if ( line_pointers == NULL ) {					\
	fprintf ( stderr, "DEVA_image_new: malloc failed!" );	\
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

DEVA_IMAGE_NEW ( DEVA_gray )
DEVA_IMAGE_NEW ( DEVA_double )
DEVA_IMAGE_NEW ( DEVA_RGB )
DEVA_IMAGE_NEW ( DEVA_RGBf )
DEVA_IMAGE_NEW ( DEVA_XYZ )
DEVA_IMAGE_NEW ( DEVA_xyY )
#ifndef	DEVA_USE_FFTW3_ALLOCATORS
DEVA_IMAGE_NEW ( DEVA_float )
DEVA_IMAGE_NEW ( DEVA_complexf )
/* DEVA_IMAGE_NEW ( DEVA_complexd ) */
#else
DEVA_IMAGE_NEW_FFTW_SINGLE ( DEVA_float )
DEVA_IMAGE_NEW_FFTW_SINGLE ( DEVA_complexf )
/* DEVA_IMAGE_NEW_FFTW_DOUBLE ( DEVA_complexd ) */
#endif	/* DEVA_USE_FFTW3_ALLOCATORS */

#define DEVA_IMAGE_DELETE( TYPE )					\
void									\
TYPE##_image_delete ( TYPE##_image *image )				\
{									\
    if ( image->image_info.description != NULL ) {			\
	free ( image->image_info.description );				\
	image->image_info.description = NULL;				\
    }									\
    free ( image->start_data );						\
    free ( image->data );						\
    free ( image );							\
}

#define DEVA_IMAGE_DELETE_FFTW_SINGLE( TYPE )				\
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

DEVA_IMAGE_DELETE ( DEVA_gray )
DEVA_IMAGE_DELETE ( DEVA_double )
DEVA_IMAGE_DELETE ( DEVA_RGB )
DEVA_IMAGE_DELETE ( DEVA_RGBf )
DEVA_IMAGE_DELETE ( DEVA_XYZ )
DEVA_IMAGE_DELETE ( DEVA_xyY )
#ifndef	DEVA_USE_FFTW3_ALLOCATORS
DEVA_IMAGE_DELETE ( DEVA_float )
DEVA_IMAGE_DELETE ( DEVA_complexf )
/* DEVA_IMAGE_DELETE ( DEVA_complexd ) */
#else
DEVA_IMAGE_DELETE_FFTW_SINGLE ( DEVA_float )
DEVA_IMAGE_DELETE_FFTW_SINGLE ( DEVA_complexf )
/* DEVA_IMAGE_DELETE_FFTW_SINGLE ( DEVA_complexd ) */
#endif	/* DEVA_USE_FFTW3_ALLOCATORS */

void
DEVA_image_check_bounds ( DEVA_gray_image *deva_image, int row, int col,
       int line, char *file )
{
    if ( ( row < 0 ) || ( row >= DEVA_image_n_rows ( deva_image ) ) ||
	    ( col < 0 ) || ( col >= DEVA_image_n_cols ( deva_image ) ) ) {
	fprintf ( stderr, "DEVA_image_data out-of-bounds reference!\n" );
	fprintf ( stderr, "line %d in file %s\n", line, file );
	exit ( EXIT_FAILURE );
    }
}

#define	DEVA_IMAGE_SAMESIZE( TYPE )					\
int									\
TYPE##_image_samesize ( TYPE##_image *i1, TYPE##_image *i2 )		\
{									\
    return ( ( DEVA_image_n_rows ( i1 ) == DEVA_image_n_rows ( i2 ) ) && \
	    ( DEVA_image_n_cols ( i1 ) == DEVA_image_n_cols ( i2 ) ) );	\
}

DEVA_IMAGE_SAMESIZE ( DEVA_gray )
DEVA_IMAGE_SAMESIZE ( DEVA_double )
DEVA_IMAGE_SAMESIZE ( DEVA_RGB )
DEVA_IMAGE_SAMESIZE ( DEVA_RGBf )
DEVA_IMAGE_SAMESIZE ( DEVA_XYZ )
DEVA_IMAGE_SAMESIZE ( DEVA_xyY )
DEVA_IMAGE_SAMESIZE ( DEVA_float )
DEVA_IMAGE_SAMESIZE ( DEVA_complexf )
/* DEVA_IMAGE_SAMESIZE ( DEVA_complexd ) */

#define	DEVA_IMAGE_SETVALUE( TYPE )					\
void									\
TYPE##_image_setvalue ( TYPE##_image *image, TYPE value )		\
{									\
    int	    row, col;							\
									\
    for ( row = 0; row < DEVA_image_n_rows ( image ); row++ ) {		\
	for ( col = 0; col < DEVA_image_n_cols ( image ); col++ ) {	\
	    DEVA_image_data ( image, row, col ) = value;		\
	}								\
    }									\
}

DEVA_IMAGE_SETVALUE ( DEVA_gray )
DEVA_IMAGE_SETVALUE ( DEVA_double )
DEVA_IMAGE_SETVALUE ( DEVA_RGB )
DEVA_IMAGE_SETVALUE ( DEVA_RGBf )
DEVA_IMAGE_SETVALUE ( DEVA_XYZ )
DEVA_IMAGE_SETVALUE ( DEVA_xyY )
DEVA_IMAGE_SETVALUE ( DEVA_float )
DEVA_IMAGE_SETVALUE ( DEVA_complexf )
/* DEVA_IMAGE_SETVALUE ( DEVA_complexd ) */
