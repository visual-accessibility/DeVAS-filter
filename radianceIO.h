/*
 * Routines for reading and writing Radiance image files.
 */

#ifndef __DEVA_RADIANCEIO_H
#define __DEVA_RADIANCEIO_H

#include "deva-image.h"
#include "deva-license.h"       /* DEVA open source license */

typedef	enum { unknown, rgbe, xyze } RadianceColorFormat;

#ifdef __cplusplus
extern "C" {
#endif

DEVA_float_image    *DEVA_brightness_from_radfilename ( char *filename );
DEVA_float_image    *DEVA_brightness_from_radfile ( FILE *radiance_fp );
void		    DEVA_brightness_to_radfilename ( char *filename,
			DEVA_float_image *brightness );
void		    DEVA_brightness_to_radfile ( FILE *radiance_fp,
			DEVA_float_image *brightness );

DEVA_float_image    *DEVA_luminance_from_radfilename ( char *filename );
DEVA_float_image    *DEVA_luminance_from_radfile ( FILE *radiance_fp );
void		    DEVA_luminance_to_radfilename ( char *filename,
			DEVA_float_image *luminance );
void		    DEVA_luminance_to_radfile ( FILE *radiance_fp,
			DEVA_float_image *luminance );

DEVA_RGBf_image     *DEVA_RGBf_from_radfilename ( char *filename );
DEVA_RGBf_image     *DEVA_RGBf_from_radfile ( FILE *radiance_fp );
void		    DEVA_RGBf_to_radfilename ( char *filename,
			DEVA_RGBf_image *RGBf );
void		    DEVA_RGBf_to_radfile ( FILE *radiance_fp,
			DEVA_RGBf_image *RGBf );

DEVA_XYZ_image	    *DEVA_XYZ_from_radfilename ( char *filename );
DEVA_XYZ_image	    *DEVA_XYZ_from_radfile ( FILE *radiance_fp );
void		    DEVA_XYZ_to_radfilename ( char *filename,
			DEVA_XYZ_image *XYZ );
void		    DEVA_XYZ_to_radfile ( FILE *radiance_fp,
			DEVA_XYZ_image *XYZ );

DEVA_xyY_image	    *DEVA_xyY_from_radfilename ( char *filename );
DEVA_xyY_image	    *DEVA_xyY_from_radfile ( FILE *radiance_fp );
void		    DEVA_xyY_to_radfilename ( char *filename,
			DEVA_xyY_image *xyY );
void		    DEVA_xyY_to_radfile ( FILE *radiance_fp,
			DEVA_xyY_image *xyY );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_RADIANCEIO_H */

