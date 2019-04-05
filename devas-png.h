#ifndef __DeVAS_PNG_H
#define __DeVAS_PNG_H

#include "devas-image.h"

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_RGB_image	*DeVAS_RGB_image_from_filename_png ( char *filename );
DeVAS_RGB_image	*DeVAS_RGB_image_from_file_png ( FILE *input );

DeVAS_gray_image	*DeVAS_gray_image_from_filename_png ( char *filename );
DeVAS_gray_image	*DeVAS_gray_image_from_file_png ( FILE *input );

void		DeVAS_RGB_image_to_filename_png ( char *filename,
		    DeVAS_RGB_image *image );
void		DeVAS_RGB_image_to_file_png ( FILE *output,
		    DeVAS_RGB_image *image );

void		DeVAS_gray_image_to_filename_png ( char *filename,
		    DeVAS_gray_image *image );
void		DeVAS_gray_image_to_file_png ( FILE *output,
		    DeVAS_gray_image *image );

#ifdef __cplusplus
}
#endif
#endif  /* __DeVAS_PNG_H */
