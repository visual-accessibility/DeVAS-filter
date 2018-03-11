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
