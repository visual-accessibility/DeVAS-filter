/*
 * Routines for reading and writing Radiance image files.
 */

#ifndef __DeVAS_RADIANCEIO_H
#define __DeVAS_RADIANCEIO_H

#include "devas-image.h"
#include "radiance-header.h"
#include "devas-license.h"       /* DeVAS open source license */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_float_image   *DeVAS_brightness_image_from_radfilename ( char *filename );
DeVAS_float_image   *DeVAS_brightness_image_from_radfile ( FILE *radiance_fp );
void		    DeVAS_brightness_image_to_radfilename ( char *filename,
			DeVAS_float_image *brightness );
void		    DeVAS_brightness_image_to_radfile ( FILE *radiance_fp,
			DeVAS_float_image *brightness );

DeVAS_float_image   *DeVAS_luminance_image_from_radfilename ( char *filename );
DeVAS_float_image   *DeVAS_luminance_image_from_radfile ( FILE *radiance_fp );
void		    DeVAS_luminance_image_to_radfilename ( char *filename,
			DeVAS_float_image *luminance );
void		    DeVAS_luminance_image_to_radfile ( FILE *radiance_fp,
			DeVAS_float_image *luminance );

DeVAS_RGBf_image    *DeVAS_RGBf_image_from_radfilename ( char *filename );
DeVAS_RGBf_image    *DeVAS_RGBf_image_from_radfile ( FILE *radiance_fp );
void		    DeVAS_RGBf_image_to_radfilename ( char *filename,
			DeVAS_RGBf_image *RGBf );
void		    DeVAS_RGBf_image_to_radfile ( FILE *radiance_fp,
			DeVAS_RGBf_image *RGBf );

DeVAS_XYZ_image	    *DeVAS_XYZ_image_from_radfilename ( char *filename );
DeVAS_XYZ_image	    *DeVAS_XYZ_image_from_radfile ( FILE *radiance_fp );
void		    DeVAS_XYZ_image_to_radfilename ( char *filename,
			DeVAS_XYZ_image *XYZ );
void		    DeVAS_XYZ_image_to_radfile ( FILE *radiance_fp,
			DeVAS_XYZ_image *XYZ );

DeVAS_xyY_image	    *DeVAS_xyY_image_from_radfilename ( char *filename );
DeVAS_xyY_image	    *DeVAS_xyY_image_from_radfile ( FILE *radiance_fp );
void		    DeVAS_xyY_image_to_radfilename ( char *filename,
			DeVAS_xyY_image *xyY );
void		    DeVAS_xyY_image_to_radfile ( FILE *radiance_fp,
			DeVAS_xyY_image *xyY );

#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_RADIANCEIO_H */

