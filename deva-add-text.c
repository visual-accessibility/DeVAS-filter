#include <stdlib.h>
#include <stdio.h>
#include <cairo/cairo.h>
#include "deva-add-text.h"
#include "deva-image.h"

void
deva_add_text ( DEVA_RGB_image *DEVA_image, double start_row, double start_col,
	double font_size, DEVA_RGBf text_color, char *text )
{
    cairo_surface_t *cairo_surface;

    cairo_surface = DEVA_RGB_cairo_open ( DEVA_image );

    DEVA_cairo_add_text ( cairo_surface, start_row, start_col, font_size,
	    text_color, text );

    DEVA_RGB_cairo_close_inplace ( cairo_surface, DEVA_image );
}

void
DEVA_cairo_add_text ( cairo_surface_t *cairo_surface, double start_row,
	double start_col, double font_size, DEVA_RGBf text_color, char *text )
/*
 * Add text in place to existing cairo surface.
 */
{
    cairo_t	    *cr;

    cr = cairo_create ( cairo_surface );

    cairo_set_source_rgb ( cr, text_color.red, text_color.green,
	    text_color.blue );

    cairo_select_font_face ( cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL,
	    CAIRO_FONT_WEIGHT_NORMAL );

    cairo_set_font_size ( cr, font_size );

    cairo_move_to ( cr, start_col, start_row );
    cairo_show_text ( cr, text );

    cairo_stroke ( cr );

    cairo_surface_flush ( cairo_surface );
    cairo_destroy ( cr );
}

cairo_surface_t *
DEVA_RGB_cairo_open ( DEVA_RGB_image *DEVA_image )
{
    int		    n_rows, n_cols;
    int		    row, col;
    cairo_surface_t *cairo_surface;
    int		    stride;
    unsigned char   *current_row;
    uint32_t	    *pixel_pt;
    uint32_t	    red, green, blue;

    n_rows = DEVA_image_n_rows ( DEVA_image );
    n_cols = DEVA_image_n_cols ( DEVA_image );

    cairo_surface = cairo_image_surface_create ( CAIRO_FORMAT_RGB24, n_cols,
	    n_rows );
    if ( cairo_surface_status ( cairo_surface ) != CAIRO_STATUS_SUCCESS ) {
	fprintf ( stderr,
		"cairo_from_DEVA_RGB: failed to create cairo surface!\n" );
	exit ( EXIT_FAILURE );
    }

    cairo_surface_flush ( cairo_surface );
    current_row = cairo_image_surface_get_data ( cairo_surface );
    stride = cairo_image_surface_get_stride ( cairo_surface );

    for ( row = 0; row < n_rows; row++ ) {

	pixel_pt = (void *) current_row;

	for ( col = 0; col < n_cols; col++ ) {
	    red = DEVA_image_data ( DEVA_image, row, col ) . red;
	    green = DEVA_image_data ( DEVA_image, row, col ) . green;
	    blue = DEVA_image_data ( DEVA_image, row, col ) . blue;

	    pixel_pt[col] = ( red << 16 ) | ( green << 8 ) | blue;
	}

	current_row += stride;
    }

    cairo_surface_mark_dirty ( cairo_surface );

    return ( cairo_surface );
}

DEVA_RGB_image *
DEVA_RGB_cairo_close ( cairo_surface_t *cairo_surface )
{
    DEVA_RGB_image  *DEVA_image;
    int		    n_rows, n_cols;

    n_rows = cairo_image_surface_get_height ( cairo_surface );
    n_cols = cairo_image_surface_get_width ( cairo_surface );

    DEVA_image = DEVA_RGB_image_new ( n_rows, n_cols );

    DEVA_RGB_cairo_close_inplace ( cairo_surface, DEVA_image );

    return ( DEVA_image );
}

void
DEVA_RGB_cairo_close_inplace ( cairo_surface_t *cairo_surface,
       DEVA_RGB_image *DEVA_image )
{
    int		    n_rows, n_cols;
    int		    row, col;
    int		    stride;
    unsigned char   *current_row;
    uint32_t	    pixel, *pixel_pt;
    uint32_t	    red, green, blue;

    if ( cairo_surface_status ( cairo_surface ) != CAIRO_STATUS_SUCCESS ) {
	fprintf ( stderr,
		"DEVA_RGB_image_from_cairo: invalid cairo surface!\n" );
	exit ( EXIT_FAILURE );
    }

    cairo_surface_flush ( cairo_surface );
    current_row = cairo_image_surface_get_data ( cairo_surface );
    stride = cairo_image_surface_get_stride ( cairo_surface );

    n_rows = cairo_image_surface_get_height ( cairo_surface );
    n_cols = cairo_image_surface_get_width ( cairo_surface );

    if ( ( n_rows != DEVA_image_n_rows ( DEVA_image ) ) ||
	    ( n_cols != DEVA_image_n_cols ( DEVA_image ) ) ) {
	fprintf ( stderr,
		"DEVA_RGB_image_from_cairo: size mismatch!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {

	pixel_pt = (void *) current_row;

	for ( col = 0; col < n_cols; col++ ) {
	    pixel = pixel_pt[col];

	    red = ( pixel >> 16 ) & 0xff;
	    green = ( pixel >> 8 ) & 0xff;
	    blue = pixel & 0xff;

	    DEVA_image_data ( DEVA_image, row, col ) . red = red;
	    DEVA_image_data ( DEVA_image, row, col ) . green = green;
	    DEVA_image_data ( DEVA_image, row, col ) . blue = blue;
	}

	current_row += stride;
    }

    cairo_surface_destroy ( cairo_surface );
}
