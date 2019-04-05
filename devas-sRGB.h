/*
 * Conversion between sRGB and XYZ, and more.
 *
 * Uses DeVAS data types.
 *
 * Uses algorithmic definition of sRGB, not color management software
 */

#ifndef __DeVAS_sRGB_CONVERT_H
#define __DeVAS_sRGB_CONVERT_H

#include "devas-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_float	gray_to_Y ( DeVAS_gray gray );
DeVAS_float	graylinear_to_Y ( DeVAS_gray gray );

DeVAS_RGB	Y_to_sRGB ( DeVAS_float Y );
DeVAS_RGB	Y_to_RGB ( DeVAS_float Y );
DeVAS_gray	Y_to_gray ( DeVAS_float Y );
DeVAS_gray	Y_to_graylinear ( DeVAS_float Y );

DeVAS_RGBf	sRGB_to_RGBf (DeVAS_RGB sRGB );
DeVAS_RGBf	RGB_to_RGBf (DeVAS_RGB RGB );
DeVAS_XYZ	sRGB_to_XYZ ( DeVAS_RGB sRGB );
DeVAS_XYZ	RGB_to_XYZ ( DeVAS_RGB RGB );
DeVAS_float	sRGB_to_Y ( DeVAS_RGB sRGB );
DeVAS_float	RGB_to_Y ( DeVAS_RGB RGB );

DeVAS_RGB	RGBf_to_sRGB ( DeVAS_RGBf RGBf );
DeVAS_RGB	RGBf_to_RGB ( DeVAS_RGBf RGBf );
DeVAS_float	RGBf_to_Y ( DeVAS_RGBf RGBf );
DeVAS_XYZ	RGBf_to_XYZ ( DeVAS_RGBf RGBf );

DeVAS_RGB	XYZ_to_sRGB ( DeVAS_XYZ XYZ );
DeVAS_RGB	XYZ_to_RGB ( DeVAS_XYZ XYZ );
DeVAS_RGBf	XYZ_to_RGBf ( DeVAS_XYZ XYZ );

DeVAS_xyY	XYZ_to_xyY ( DeVAS_XYZ XYZ );
DeVAS_XYZ	xyY_to_XYZ ( DeVAS_xyY xyY );

#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_sRGB_CONVERT_H */
