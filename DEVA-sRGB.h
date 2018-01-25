/*
 * Conversion between sRGB and XYZ, and more.
 *
 * Uses DEVA data types.
 *
 * Uses algorithmic definition of sRGB, not color management software
 */

#ifndef __DEVA_sRGB_CONVERT_H
#define __DEVA_sRGB_CONVERT_H

#include "deva-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DEVA_float	gray_to_Y ( DEVA_gray gray );
DEVA_float	graylinear_to_Y ( DEVA_gray gray );

DEVA_RGB	Y_to_sRGB ( DEVA_float Y );
DEVA_RGB	Y_to_RGB ( DEVA_float Y );
DEVA_gray	Y_to_gray ( DEVA_float Y );
DEVA_gray	Y_to_graylinear ( DEVA_float Y );

DEVA_RGBf	sRGB_to_RGBf (DEVA_RGB sRGB );
DEVA_RGBf	RGB_to_RGBf (DEVA_RGB RGB );
DEVA_XYZ	sRGB_to_XYZ ( DEVA_RGB sRGB );
DEVA_XYZ	RGB_to_XYZ ( DEVA_RGB RGB );
DEVA_float	sRGB_to_Y ( DEVA_RGB sRGB );
DEVA_float	RGB_to_Y ( DEVA_RGB RGB );

DEVA_RGB	RGBf_to_sRGB ( DEVA_RGBf RGBf );
DEVA_RGB	RGBf_to_RGB ( DEVA_RGBf RGBf );
DEVA_float	RGBf_to_Y ( DEVA_RGBf RGBf );
DEVA_XYZ	RGBf_to_XYZ ( DEVA_RGBf RGBf );

DEVA_RGB	XYZ_to_sRGB ( DEVA_XYZ XYZ );
DEVA_RGB	XYZ_to_RGB ( DEVA_XYZ XYZ );
DEVA_RGBf	XYZ_to_RGBf ( DEVA_XYZ XYZ );

DEVA_xyY	XYZ_to_xyY ( DEVA_XYZ XYZ );
DEVA_XYZ	xyY_to_XYZ ( DEVA_xyY xyY );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_sRGB_CONVERT_H */
