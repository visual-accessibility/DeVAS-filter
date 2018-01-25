/*
 * Like DEVA_RGB_image_from_filename, DEVA_RGB_image_from_file,
 * DEVA_RGB_image_to_filename, and DEVA_RGB_image to file, execept
 * works with PNG files.
 *
 * Read routines are special cased to return value of
 * EXIF_TAG_FOCAL_LENGTH_IN_35MM_FILM tag, if present.
 * (General version of these routings, without the tag
 * processing, need to be written at some point.
 *
 * Based on example.c from jpeg-6b source, but appears to
 * also work with jpeg-8.
 */

#ifndef __DEVA_PNG_H
#define __DEVA_PNG_H

#include "deva-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DEVA_RGB_image	*DEVA_RGB_image_from_filename_png ( char *filename );
DEVA_RGB_image	*DEVA_RGB_image_from_file_png ( FILE *input );

DEVA_gray_image	*DEVA_gray_image_from_filename_png ( char *filename );
DEVA_gray_image	*DEVA_gray_image_from_file_png ( FILE *input );

void		DEVA_RGB_image_to_filename_png ( char *filename,
		    DEVA_RGB_image *image );
void		DEVA_RGB_image_to_file_png ( FILE *output,
		    DEVA_RGB_image *image );

void		DEVA_gray_image_to_filename_png ( char *filename,
		    DEVA_gray_image *image );
void		DEVA_gray_image_to_file_png ( FILE *output,
		    DEVA_gray_image *image );

#ifdef __cplusplus
}
#endif
#endif  /* __DEVA_PNG_H */
