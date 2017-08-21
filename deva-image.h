/*
 * Establishes data types for a variety of fixed point and floating point
 * image pixel types.
 *
 * Defines dynamically allocatable 2-D arrays for each of these types.
 * Arrays have additional properties associated with them of use in the
 * deva-filter.
 */

#ifndef __DEVAIMAGE_H
#define __DEVAIMAGE_H

#include <stdlib.h>
#include <stdio.h>		/* needed indirectly for view.h */
#include <stdint.h>		/* for uint8_t */
#include "radiance/fvect.h"	/* for FVECT type */
#include "radiance/view.h"	/* for VIEW structure */
#include "deva-license.h"	/* DEVA open source license */

#ifdef DEVA_USE_FFTW3_ALLOCATORS
/*
 * For better alignment -- applies only to float/double and complexf/complexd.
 * To use, define DEVA_USE_FFTW3_ALLOCATORS before including deva-image.h.
 */
#include <fftw3.h>
#endif	/* DEVA_USE_FFTW3_ALLOCATORS */

#ifndef	TRUE
#define	TRUE		1
#endif	/* TRUE */
#ifndef	FALSE
#define	FALSE		0
#endif	/* FALSE */

#define	DEVA_WHTEFFICACY	179.0	/* uniform white light */
					/* (from Radiance color.h) */

#define DEVA_radiometric2photometric(v)	( (v) * DEVA_WHTEFFICACY )
			/* convert from radiance rgbe units */
			/* (watts/steradian/m^2 over the visible spectrum) */
			/* to luminance units (cd/m^2) */

#define	DEVA_photometric2radiometric(v)	( (v) / DEVA_WHTEFFICACY )
			/* convert to radiance rgbe units */
			/* (watts/steradian/m^2 over the visible spectrum) */
			/* from luminance units (cd/m^2) */

#define	DEVA_x_WHITEPOINT	0.3333	/* Radiance CIE_x_w */
#define	DEVA_y_WHITEPOINT	0.3333	/* Radiance CIE_y_w */
					/* note: not 1.0/3.0 !!! */

/* pixel types: */

typedef uint8_t	DEVA_gray;	/* 8 bit grayscale */
typedef float	DEVA_float;	/* 32 bit float */
typedef double	DEVA_double;	/* 64 bit double */

typedef struct {		/* 8 bit RGB */
    uint8_t  red;
    uint8_t  green;
    uint8_t  blue;
} DEVA_RGB;

typedef struct {		/* 32 bit float RGB */
    float    red;
    float    green;
    float    blue;
} DEVA_RGBf;

typedef struct {		/* 32 bit float CIE XYZ */
    float    X;
    float    Y;
    float    Z;
} DEVA_XYZ;

typedef struct {		/* 32 bit float CIE xyY */
    float    x;
    float    y;
    float    Y;
} DEVA_xyY;

typedef struct {		/* 32 bit float complex */
    float   real;
    float   imaginary;
} DEVA_complexf;

typedef struct {		/* 64 bit double complex */
    double  real;
    double  imaginary;
} DEVA_complexd;

typedef struct {
    VIEW    view;		/* RADIANCE VIEW structure */
    char    *description;	/* value should be malloc'ed */
    				/* often, this will be history info */
} DEVA_Image_Info;

#define	NULLVIEW	{'\0',{0.,0.,0.},{0.,0.,0.},{0.,0.,0.}, \
				0.,0.,0.,0.,0.,0.,0., \
				{0.,0.,0.},{0.,0.,0.},0.,0.}

/* Note that order is (n_rows,n_cols), not (x,y) or (width,height)!!! */
#define DEVA_DEFINE_IMAGE_TYPE( TYPE )                                        \
typedef struct {							      \
    int		    n_rows, n_cols;	/* order reversed from x,y! */	      \
    					/* Radiance treats these as signed */ \
    int		    exposure_set;	/* exposure not set is not quite */   \
    					/* the same as exposure=1.0! */	      \
    double	    exposure;		/* as in Radiance file */	      \
    					/* 1.0 if exposure not explicitly */  \
					/* set */			      \
    DEVA_Image_Info image_info;		/* info needed by deva-filter */      \
    TYPE            *start_data;        /* start of allocated data block */   \
    TYPE            **data;             /* array of pointers to array rows */ \
} TYPE##_image;

DEVA_DEFINE_IMAGE_TYPE ( DEVA_gray )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_float )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_double )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_RGB )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_RGBf )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_XYZ )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_xyY )
DEVA_DEFINE_IMAGE_TYPE ( DEVA_complexf ) /* make sure to link against fftw3f! */
/* DEVA_DEFINE_IMAGE_TYPE ( DEVA_complexd ) */

/*
 * methods on DEVA_image objects:
 */

/*
 * For bounds checking, define DEVA_CHECK_BOUNDS before including this file
 * or any other file including this file.  Safest is to do the define before
 * including *any* include files.
 */
#ifdef	DEVA_CHECK_BOUNDS
#define DEVA_image_data(deva_image,row,col)				\
	    (deva_image)->data[DEVA_image_check_bounds (		\
		(DEVA_gray_image *) deva_image, row, col,		\
		    __LINE__, __FILE__ ) , row][col] 
#else
#define	DEVA_image_data(deva_image,row,col)	(deva_image)->data[row][col]
    			/* read/write */
#endif

#define	DEVA_image_n_rows(deva_image)		(deva_image)->n_rows
    			/* read only (but not enforced ) */

#define	DEVA_image_n_cols(deva_image)		(deva_image)->n_cols
    			/* read only (but not enforced ) */

#define	DEVA_image_samesize(deva_image_1,deva_image_2)			\
		( ( DEVA_image_n_rows ( deva_image_1 ) ==		\
		    DEVA_image_n_rows ( deva_image_2 ) ) &&		\
		  ( DEVA_image_n_cols ( deva_image_1 ) ==		\
		    DEVA_image_n_cols ( deva_image_2 ) ) )

#define	DEVA_image_info(deva_image)		(deva_image)->image_info
    			/* read/write */

#define	DEVA_image_view(deva_image)		(deva_image)->image_info.view
    			/* read/write */

#define	DEVA_image_description(deva_image) (deva_image)->image_info.description
    			/* read/write */

#define	DEVA_image_exposure_set(deva_image)	(deva_image)->exposure_set
    			/* read/write */

#define	DEVA_image_exposure(deva_image)		(deva_image)->exposure
    			/* read/write */

/*
 * function prototypes:
 */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_xyY    DEVA_XYZ2xyY ( DEVA_XYZ XYZ );
DEVA_XYZ    DEVA_xyY2XYZ ( DEVA_xyY xyY );

DEVA_RGBf   DEVA_XYZ2RGBf ( DEVA_XYZ XYZ );
DEVA_XYZ    DEVA_RGBf2XYZ ( DEVA_RGBf RGBf );

DEVA_RGBf   DEVA_xyY2RGBf ( DEVA_xyY xyY );
DEVA_xyY    DEVA_RGBf2xyY ( DEVA_RGBf RGBf );

DEVA_RGBf   DEVA_Y2RGBf ( DEVA_float Y );
float	    DEVA_RGBf2Y ( DEVA_RGBf RGBf );

/* Note that order is (n_rows,n_cols), not (x,y) or (width,height)!!! */
#define DEVA_PROTOTYPE_IMAGE_NEW( TYPE )				\
TYPE##_image    *TYPE##_image_new ( unsigned int n_rows, unsigned int n_cols );

DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_gray )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_float )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_double )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_RGB )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_RGBf )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_XYZ )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_xyY )
DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_complexf )
/* DEVA_PROTOTYPE_IMAGE_NEW ( DEVA_complexd ) */

#define DEVA_PROTOTYPE_IMAGE_DELETE( TYPE )				\
void    TYPE##_image_delete ( TYPE##_image *i );

DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_gray )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_float )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_double )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_RGB )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_RGBf )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_XYZ )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_xyY )
DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_complexf )
/* DEVA_PROTOTYPE_IMAGE_DELETE ( DEVA_complexd ) */

void	DEVA_image_check_bounds ( DEVA_gray_image *deva_image, int row,
	    int col, int lineno, char *file );

#define DEVA_PROTOTYPE_IMAGE_SAMESIZE( TYPE )				\
int	TYPE##_image_samesize ( TYPE##_image *i1, TYPE##_image *i2 );

DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_gray )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_float )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_double )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_RGB )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_RGBf )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_XYZ )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_xyY )
DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_complexf )
/* DEVA_PROTOTYPE_IMAGE_SAMESIZE ( DEVA_complexd ) */

#define DEVA_PROTOTYPE_IMAGE_SETVALUE( TYPE )				\
void	TYPE##_image_setvalue ( TYPE##_image *image, TYPE value );

DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_gray )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_float )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_double )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_RGB )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_RGBf )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_XYZ )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_xyY )
DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_complexf )
/* DEVA_PROTOTYPE_IMAGE_SETVALUE ( DEVA_complexd ) */

#ifdef __cplusplus
}
#endif

#endif	/* __DEVAIMAGE_H */
