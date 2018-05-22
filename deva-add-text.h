#ifndef	__DEVA_ADD_TEXT_H
#define	__DEVA_ADD_TEXT_H

#include <stdlib.h>
#include <stdio.h>
#include <cairo/cairo.h>
#include "deva-image.h"

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void		deva_add_text ( DEVA_RGB_image *DEVA_image, double start_row,
		    double start_col, double font_size, DEVA_RGBf text_color,
		    char *text );

cairo_surface_t	*DEVA_RGB_cairo_open ( DEVA_RGB_image *DEVA_image );
void		DEVA_cairo_add_text ( cairo_surface_t *cairo_surface,
		    double start_row, double start_col, double font_size,
		    DEVA_RGBf text_color, char *text );
DEVA_RGB_image	*DEVA_RGB_cairo_close ( cairo_surface_t *cairo_surface );
void		DEVA_RGB_cairo_close_inplace ( cairo_surface_t *cairo_surface,
       		    DEVA_RGB_image *DEVA_image );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_ADD_TEXT_H */
