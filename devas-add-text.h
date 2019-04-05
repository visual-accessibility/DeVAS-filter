#ifndef	__DeVAS_ADD_TEXT_H
#define	__DeVAS_ADD_TEXT_H

#include <stdlib.h>
#include <stdio.h>
#include <cairo/cairo.h>
#include "devas-image.h"

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void		devas_add_text ( DeVAS_RGB_image *DeVAS_image, double start_row,
		    double start_col, double font_size, DeVAS_RGBf text_color,
		    char *text );

cairo_surface_t	*DeVAS_RGB_cairo_open ( DeVAS_RGB_image *DeVAS_image );
void		DeVAS_cairo_add_text ( cairo_surface_t *cairo_surface,
		    double start_row, double start_col, double font_size,
		    DeVAS_RGBf text_color, char *text );
DeVAS_RGB_image	*DeVAS_RGB_cairo_close ( cairo_surface_t *cairo_surface );
void		DeVAS_RGB_cairo_close_inplace ( cairo_surface_t *cairo_surface,
       		    DeVAS_RGB_image *DeVAS_image );

#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_ADD_TEXT_H */
