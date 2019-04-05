#include <stdlib.h>
#include <stdio.h>
#include <cairo/cairo.h>
#include "devas-add-text.h"
#include "devas-image.h"

void
devas_add_text ( DeVAS_RGB_image *DeVAS_image, double start_row,
	double start_col, double font_size, DeVAS_RGBf text_color, char *text )
{
    cairo_surface_t *cairo_surface;

    cairo_surface = DeVAS_RGB_cairo_open ( DeVAS_image );

    DeVAS_cairo_add_text ( cairo_surface, start_row, start_col, font_size,
	    text_color, text );

    DeVAS_RGB_cairo_close_inplace ( cairo_surface, DeVAS_image );
}

void
DeVAS_cairo_add_text ( cairo_surface_t *cairo_surface, double start_row,
	double start_col, double font_size, DeVAS_RGBf text_color, char *text )
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
DeVAS_RGB_cairo_open ( DeVAS_RGB_image *DeVAS_image )
{
    int		    n_rows, n_cols;
    int		    row, col;
    cairo_surface_t *cairo_surface;
    int		    stride;
    unsigned char   *current_row;
    uint32_t	    *pixel_pt;
    uint32_t	    red, green, blue;

    n_rows = DeVAS_image_n_rows ( DeVAS_image );
    n_cols = DeVAS_image_n_cols ( DeVAS_image );

    cairo_surface = cairo_image_surface_create ( CAIRO_FORMAT_RGB24, n_cols,
	    n_rows );
    if ( cairo_surface_status ( cairo_surface ) != CAIRO_STATUS_SUCCESS ) {
	fprintf ( stderr,
		"cairo_from_DeVAS_RGB: failed to create cairo surface!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    cairo_surface_flush ( cairo_surface );
    current_row = cairo_image_surface_get_data ( cairo_surface );
    stride = cairo_image_surface_get_stride ( cairo_surface );

    for ( row = 0; row < n_rows; row++ ) {

	pixel_pt = (void *) current_row;

	for ( col = 0; col < n_cols; col++ ) {
	    red = DeVAS_image_data ( DeVAS_image, row, col ) . red;
	    green = DeVAS_image_data ( DeVAS_image, row, col ) . green;
	    blue = DeVAS_image_data ( DeVAS_image, row, col ) . blue;

	    pixel_pt[col] = ( red << 16 ) | ( green << 8 ) | blue;
	}

	current_row += stride;
    }

    cairo_surface_mark_dirty ( cairo_surface );

    return ( cairo_surface );
}

DeVAS_RGB_image *
DeVAS_RGB_cairo_close ( cairo_surface_t *cairo_surface )
{
    DeVAS_RGB_image  *DeVAS_image;
    int		    n_rows, n_cols;

    n_rows = cairo_image_surface_get_height ( cairo_surface );
    n_cols = cairo_image_surface_get_width ( cairo_surface );

    DeVAS_image = DeVAS_RGB_image_new ( n_rows, n_cols );

    DeVAS_RGB_cairo_close_inplace ( cairo_surface, DeVAS_image );

    return ( DeVAS_image );
}

void
DeVAS_RGB_cairo_close_inplace ( cairo_surface_t *cairo_surface,
       DeVAS_RGB_image *DeVAS_image )
{
    int		    n_rows, n_cols;
    int		    row, col;
    int		    stride;
    unsigned char   *current_row;
    uint32_t	    pixel, *pixel_pt;
    uint32_t	    red, green, blue;

    if ( cairo_surface_status ( cairo_surface ) != CAIRO_STATUS_SUCCESS ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_from_cairo: invalid cairo surface!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    cairo_surface_flush ( cairo_surface );
    current_row = cairo_image_surface_get_data ( cairo_surface );
    stride = cairo_image_surface_get_stride ( cairo_surface );

    n_rows = cairo_image_surface_get_height ( cairo_surface );
    n_cols = cairo_image_surface_get_width ( cairo_surface );

    if ( ( n_rows != DeVAS_image_n_rows ( DeVAS_image ) ) ||
	    ( n_cols != DeVAS_image_n_cols ( DeVAS_image ) ) ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_from_cairo: size mismatch!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {

	pixel_pt = (void *) current_row;

	for ( col = 0; col < n_cols; col++ ) {
	    pixel = pixel_pt[col];

	    red = ( pixel >> 16 ) & 0xff;
	    green = ( pixel >> 8 ) & 0xff;
	    blue = pixel & 0xff;

	    DeVAS_image_data ( DeVAS_image, row, col ) . red = red;
	    DeVAS_image_data ( DeVAS_image, row, col ) . green = green;
	    DeVAS_image_data ( DeVAS_image, row, col ) . blue = blue;
	}

	current_row += stride;
    }

    cairo_surface_destroy ( cairo_surface );
}
