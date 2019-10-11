#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include "devas-image.h"
#include "devas-png.h"
#include "devas-license.h"

/*
 * Read/write in-memory 8-bit DeVAS gray-scale and RGB images from/to PNG files.
 *
 * Assumes that PNG image files have contiguous, tightly packed pixels.
 */

DeVAS_RGB_image *
DeVAS_RGB_image_from_filename_png ( char *filename )
{
    FILE    *file;

    file = fopen ( filename, "rb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    return ( DeVAS_RGB_image_from_file_png ( file ) );
}

DeVAS_RGB_image *
DeVAS_RGB_image_from_file_png ( FILE *input )
{
    png_image	    png_input;
    DeVAS_RGB_image *image;

    /* Initialize the 'png_image' structure. */
    memset ( &png_input, 0, sizeof ( png_input ) );
    png_input.opaque = NULL;
    png_input.version = PNG_IMAGE_VERSION;
    png_input.format = PNG_FORMAT_RGB;

    if ( png_image_begin_read_from_stdio ( &png_input, input ) == 0 ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_from_file_png: error reading file!\n" );
	png_image_free ( &png_input );
	exit ( EXIT_FAILURE );
    }

    image = DeVAS_RGB_image_new ( png_input.height, png_input.width );

    if ( png_image_finish_read ( &png_input, NULL/*background*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/) == 0 ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_from_file_png: error reading file!\n" );
	png_image_free ( &png_input );
	exit ( EXIT_FAILURE );
    }

    png_image_free ( &png_input );

    return ( image );
}

DeVAS_gray_image *
DeVAS_gray_image_from_filename_png ( char *filename )
{
    FILE    *file;

    file = fopen ( filename, "rb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    return ( DeVAS_gray_image_from_file_png ( file ) );
}

DeVAS_gray_image *
DeVAS_gray_image_from_file_png ( FILE *input )
{
    png_image		png_input;
    DeVAS_gray_image	*image;

    /* Initialize the 'png_image' structure. */
    memset ( &png_input, 0, sizeof ( png_input ) );
    png_input.opaque = NULL;
    png_input.version = PNG_IMAGE_VERSION;
    png_input.format = PNG_FORMAT_GRAY;

    if ( png_image_begin_read_from_stdio ( &png_input, input ) == 0 ) {
	fprintf ( stderr,
		"DeVAS_gray_image_from_file_png: error reading file!\n" );
	png_image_free ( &png_input );
	exit ( EXIT_FAILURE );
    }

    png_input.format = PNG_FORMAT_GRAY;
    	/*
	 * Empirical evidence suggests that this has to come *after*
	 * the begin_read.
	 */

    image = DeVAS_gray_image_new ( png_input.height, png_input.width );

    if ( png_image_finish_read ( &png_input, NULL/*background*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/) == 0 ) {
	png_image_free ( &png_input );
	fprintf ( stderr,
		"DeVAS_gray_image_from_file_png: error reading file!\n" );
	exit ( EXIT_FAILURE );
    }

    png_image_free ( &png_input );

    return ( image );
}

void
DeVAS_RGB_image_to_filename_png ( char *filename, DeVAS_RGB_image *image )
{
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    DeVAS_RGB_image_to_file_png ( file, image );

    fclose ( file );
}

void
DeVAS_RGB_image_to_file_png ( FILE *output, DeVAS_RGB_image *image )
{
    png_image	png_output;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );

    png_output.opaque = NULL;
    png_output.version = PNG_IMAGE_VERSION;
    png_output.width = DeVAS_image_n_cols ( image );
    png_output.height = DeVAS_image_n_rows ( image );
    png_output.format = PNG_FORMAT_RGB;
    png_output.flags = 0;
    png_output.colormap_entries = 0;
    png_output.warning_or_error = 0;
    png_output.message[0] = '\0';

    if ( png_image_write_to_stdio ( &png_output, output, 0 /*convert_to_8bit*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/) == 0 ) {
	fprintf ( stderr,
		"DeVAS_RGB_image_to_file_png: error writing file!\n" );
	exit ( EXIT_FAILURE );
    }
}

void
DeVAS_gray_image_to_filename_png ( char *filename, DeVAS_gray_image *image )
{
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    DeVAS_gray_image_to_file_png ( file, image );

    fclose ( file );
}

void
DeVAS_gray_image_to_file_png ( FILE *output, DeVAS_gray_image *image )
{
    png_image	png_output;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );

    png_output.opaque = NULL;
    png_output.version = PNG_IMAGE_VERSION;
    png_output.width = DeVAS_image_n_cols ( image );
    png_output.height = DeVAS_image_n_rows ( image );
    png_output.format = PNG_FORMAT_GRAY;
    png_output.colormap_entries = 0;
    png_output.warning_or_error = 0;
    png_output.message[0] = '\0';

    png_image_write_to_stdio ( &png_output, output, 0 /*convert_to_8bit*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/ );
}
