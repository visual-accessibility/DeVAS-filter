/*
 * Establishes data types for a variety of fixed point and floating point
 * image pixel types.
 *
 * Defines dynamically allocatable 2-D arrays for each of these types.
 * Arrays have additional properties associated with them of use in the
 * devas-filter.
 */

#ifndef __DeVAS_IMAGE_H
#define __DeVAS_IMAGE_H

#include <stdlib.h>
#include <stdio.h>		/* needed indirectly for view.h */
#include <stdint.h>		/* for uint8_t */
#include "radiance/fvect.h"	/* for FVECT type */
#include "radiance/view.h"	/* for VIEW structure */
#include "devas-license.h"	/* DeVAS open source license */

#ifdef DeVAS_USE_FFTW3_ALLOCATORS
/*
 * For better alignment -- applies only to float/double and complexf/complexd.
 * To use, define DeVAS_USE_FFTW3_ALLOCATORS before including devas-image.h.
 */
#include <fftw3.h>
#endif	/* DeVAS_USE_FFTW3_ALLOCATORS */

#ifndef	TRUE
#define	TRUE		1
#endif	/* TRUE */
#ifndef	FALSE
#define	FALSE		0
#endif	/* FALSE */

#define	DeVAS_WHTEFFICACY	179.0	/* uniform white light */
					/* (from Radiance color.h) */

#define DeVAS_radiometric2photometric(v)	( (v) * DeVAS_WHTEFFICACY )
			/* convert from radiance rgbe units */
			/* (watts/steradian/m^2 over the visible spectrum) */
			/* to luminance units (cd/m^2) */

#define	DeVAS_photometric2radiometric(v)	( (v) / DeVAS_WHTEFFICACY )
			/* convert to radiance rgbe units */
			/* (watts/steradian/m^2 over the visible spectrum) */
			/* from luminance units (cd/m^2) */

#define	DeVAS_x_WHITEPOINT	0.3333	/* Radiance CIE_x_w */
#define	DeVAS_y_WHITEPOINT	0.3333	/* Radiance CIE_y_w */
					/* note: not 1.0/3.0 !!! */

/* pixel types: */

typedef uint8_t	DeVAS_gray;	/* 8 bit grayscale */
typedef float	DeVAS_float;	/* 32 bit float */
typedef double	DeVAS_double;	/* 64 bit double */

typedef struct {		/* 8 bit RGB */
    uint8_t  red;
    uint8_t  green;
    uint8_t  blue;
} DeVAS_RGB;

typedef struct {		/* 32 bit float RGB */
    float    red;
    float    green;
    float    blue;
} DeVAS_RGBf;

typedef struct {		/* 32 bit float CIE XYZ */
    float    X;
    float    Y;
    float    Z;
} DeVAS_XYZ;

typedef struct {		/* 32 bit float CIE xyY */
    float    x;
    float    y;
    float    Y;
} DeVAS_xyY;

typedef struct {		/* 32 bit float complex */
    float   real;
    float   imaginary;
} DeVAS_complexf;

typedef struct {		/* 64 bit double complex */
    double  real;
    double  imaginary;
} DeVAS_complexd;

typedef struct {
    VIEW    view;		/* RADIANCE VIEW structure */
    char    *description;	/* value should be malloc'ed */
    				/* often, this will be history info */
} DeVAS_Image_Info;

#define	NULLVIEW	{'\0',{0.,0.,0.},{0.,0.,0.},{0.,0.,0.}, \
				0.,0.,0.,0.,0.,0.,0., \
				{0.,0.,0.},{0.,0.,0.},0.,0.}

/* Note that order is (n_rows,n_cols), not (x,y) or (width,height)!!! */
#define DeVAS_DEFINE_IMAGE_TYPE( TYPE )                                        \
typedef struct {							      \
    int		    n_rows, n_cols;	/* order reversed from x,y! */	      \
    					/* Radiance treats these as signed */ \
    int		    exposure_set;	/* exposure not set is not quite */   \
    					/* the same as exposure=1.0! */	      \
    double	    exposure;		/* as in Radiance file */	      \
    					/* 1.0 if exposure not explicitly */  \
					/* set */			      \
    DeVAS_Image_Info image_info;		/* info needed by devas-filter */      \
    TYPE            *start_data;        /* start of allocated data block */   \
    TYPE            **data;             /* array of pointers to array rows */ \
} TYPE##_image;

DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_gray )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_float )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_double )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_RGB )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_RGBf )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_XYZ )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_xyY )
DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_complexf )
   					 /* make sure to link against fftw3f! */
/* DeVAS_DEFINE_IMAGE_TYPE ( DeVAS_complexd ) */

/*
 * methods on DeVAS_image objects:
 */

/*
 * For bounds checking, define DeVAS_CHECK_BOUNDS before including this file
 * or any other file including this file.  Safest is to do the define before
 * including *any* include files.
 */
#ifdef	DeVAS_CHECK_BOUNDS
#define DeVAS_image_data(devas_image,row,col)				\
	    (devas_image)->data[DeVAS_image_check_bounds (		\
		(DeVAS_gray_image *) devas_image, row, col,		\
		    __LINE__, __FILE__ ) , row][col] 
#else
#define	DeVAS_image_data(devas_image,row,col)	(devas_image)->data[row][col]
    			/* read/write */
#endif

#define	DeVAS_image_n_rows(devas_image)		(devas_image)->n_rows
    			/* read only (but not enforced ) */

#define	DeVAS_image_n_cols(devas_image)		(devas_image)->n_cols
    			/* read only (but not enforced ) */

#define	DeVAS_image_samesize(devas_image_1,devas_image_2)		\
		( ( DeVAS_image_n_rows ( devas_image_1 ) ==		\
		    DeVAS_image_n_rows ( devas_image_2 ) ) &&		\
		  ( DeVAS_image_n_cols ( devas_image_1 ) ==		\
		    DeVAS_image_n_cols ( devas_image_2 ) ) )

#define	DeVAS_image_info(devas_image)		(devas_image)->image_info
    			/* read/write */

#define	DeVAS_image_view(devas_image)		(devas_image)->image_info.view
    			/* read/write */

#define	DeVAS_image_description(devas_image) \
					(devas_image)->image_info.description
    			/* read/write */

#define	DeVAS_image_exposure_set(devas_image)	(devas_image)->exposure_set
    			/* read/write */

#define	DeVAS_image_exposure(devas_image)	(devas_image)->exposure
    			/* read/write */

/*
 * function prototypes:
 */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_xyY    DeVAS_XYZ2xyY ( DeVAS_XYZ XYZ );
DeVAS_XYZ    DeVAS_xyY2XYZ ( DeVAS_xyY xyY );

DeVAS_RGBf   DeVAS_XYZ2RGBf ( DeVAS_XYZ XYZ );
DeVAS_XYZ    DeVAS_RGBf2XYZ ( DeVAS_RGBf RGBf );

DeVAS_RGBf   DeVAS_xyY2RGBf ( DeVAS_xyY xyY );
DeVAS_xyY    DeVAS_RGBf2xyY ( DeVAS_RGBf RGBf );

DeVAS_RGBf   DeVAS_Y2RGBf ( DeVAS_float Y );
float	    DeVAS_RGBf2Y ( DeVAS_RGBf RGBf );

/* Note that order is (n_rows,n_cols), not (x,y) or (width,height)!!! */
#define DeVAS_PROTOTYPE_IMAGE_NEW( TYPE )				\
TYPE##_image    *TYPE##_image_new ( unsigned int n_rows, unsigned int n_cols );

DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_gray )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_float )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_double )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_RGB )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_RGBf )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_XYZ )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_xyY )
DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_complexf )
/* DeVAS_PROTOTYPE_IMAGE_NEW ( DeVAS_complexd ) */

#define DeVAS_PROTOTYPE_IMAGE_DELETE( TYPE )				\
void    TYPE##_image_delete ( TYPE##_image *i );

DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_gray )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_float )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_double )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_RGB )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_RGBf )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_XYZ )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_xyY )
DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_complexf )
/* DeVAS_PROTOTYPE_IMAGE_DELETE ( DeVAS_complexd ) */

void	DeVAS_image_check_bounds ( DeVAS_gray_image *devas_image, int row,
	    int col, int lineno, char *file );

void	DeVAS_print_file_lineno ( char *file, int line );

#define DeVAS_PROTOTYPE_IMAGE_SAMESIZE( TYPE )				\
int	TYPE##_image_samesize ( TYPE##_image *i1, TYPE##_image *i2 );

DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_gray )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_float )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_double )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_RGB )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_RGBf )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_XYZ )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_xyY )
DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_complexf )
/* DeVAS_PROTOTYPE_IMAGE_SAMESIZE ( DeVAS_complexd ) */

#define DeVAS_PROTOTYPE_IMAGE_SETVALUE( TYPE )				\
void	TYPE##_image_setvalue ( TYPE##_image *image, TYPE value );

DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_gray )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_float )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_double )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_RGB )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_RGBf )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_XYZ )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_xyY )
DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_complexf )
/* DeVAS_PROTOTYPE_IMAGE_SETVALUE ( DeVAS_complexd ) */

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_IMAGE_H */
