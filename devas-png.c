/*
 * Read/write DeVAS RGB/gray image object from/to PNG files.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include "devas-image.h"
#include "devas-png.h"
#include "devas-license.h"

DeVAS_RGB_image *
DeVAS_RGB_image_from_filename_png ( char *filename )
/*
 * Read a PNG file and return pixel values as a DeVAS_RGB_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as RGB color values.
 *
 * filename:	name of an existing PNG file.
 */
{
    FILE    *file;

    file = fopen ( filename, "rb" );
    if ( file == NULL ) {
	perror ( filename );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    return ( DeVAS_RGB_image_from_file_png ( file ) );
}

DeVAS_RGB_image *
DeVAS_RGB_image_from_file_png ( FILE *input )
/*
 * Read a PNG file and return pixel values as a DeVAS_RGB_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as RGB color values.
 *
 * input:	open file descriptor for an existing PNG file.
 */
{
    png_image	    png_output;
    DeVAS_RGB_image  *image;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );
    png_output.version = PNG_IMAGE_VERSION;

    if ( png_image_begin_read_from_stdio ( &png_output, input ) == 0 ) {
	fprintf ( stderr, "DeVAS_RGB_image_from_file_png: error reading file!" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    png_output.format = PNG_FORMAT_RGB;

    image = DeVAS_RGB_image_new ( png_output.height, png_output.width );

    if ( png_image_finish_read ( &png_output, NULL/*background*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL/*colormap*/) == 0 ) {
	fprintf ( stderr, "DeVAS_RGB_image_from_file_png: error reading file!" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    return ( image );
}

DeVAS_gray_image *
DeVAS_gray_image_from_filename_png ( char *filename )
/*
 * Read a PNG file and return pixel values as a DeVAS_gray_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as grayscale values.
 *
 * filename:	name of an existing PNG file.
 */
{
    FILE    *file;

    file = fopen ( filename, "rb" );
    if ( file == NULL ) {
	perror ( filename );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    return ( DeVAS_gray_image_from_file_png ( file ) );
}

DeVAS_gray_image *
DeVAS_gray_image_from_file_png ( FILE *input )
/*
 * Read a PNG file and return pixel values as a DeVAS_gray_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as grayscale values.
 *
 * input:	open file descriptor for an existing PNG file.
 */
{
    png_image	    png_output;
    DeVAS_gray_image  *image;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );
    png_output.version = PNG_IMAGE_VERSION;

    if ( png_image_begin_read_from_stdio ( &png_output, input ) == 0 ) {
	fprintf ( stderr,
		"DeVAS_gray_image_from_file_png: error reading file!" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    png_output.format = PNG_FORMAT_GRAY;

    image = DeVAS_gray_image_new ( png_output.height, png_output.width );

    if ( png_image_finish_read ( &png_output, NULL/*background*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL/*colormap*/) == 0 ) {
	fprintf ( stderr,
		"DeVAS_gray_image_from_file_png: error reading file!" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    return ( image );
}

/*
 * Read a PNG file and return pixel values as a DeVAS_gray_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as grayscale values.
 *
 * input:	open file descriptor for an existing PNG file.
 */

void
DeVAS_RGB_image_to_filename_png ( char *filename, DeVAS_RGB_image *image )
/*
 * Write the pixel values from a DeVAS_RGB_image to a RGB color PNG file.
 *
 * filename:	name of new PNG file.
 */
{
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    DeVAS_RGB_image_to_file_png ( file, image );

    fclose ( file );
}

void
DeVAS_RGB_image_to_file_png ( FILE *output, DeVAS_RGB_image *image )
/*
 * Write the pixel values from a DeVAS_RGB_image to a RGB color PNG file.
 *
 * output:	open file descriptor for new PNG file.
 */
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
	fprintf ( stderr, "DeVAS_RGB_image_to_file_png: error writing file!" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }
}

void
DeVAS_gray_image_to_filename_png ( char *filename, DeVAS_gray_image *image )
{
/*
 * Write the pixel values from a DeVAS_gray_image to a grayscale PNG file.
 *
 * filename:	name of new PNG file.
 */
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    DeVAS_gray_image_to_file_png ( file, image );

    fclose ( file );
}

void
DeVAS_gray_image_to_file_png ( FILE *output, DeVAS_gray_image *image )
/*
 * Write the pixel values from a DeVAS_gray_image to a grayscale PNG file.
 *
 * output:	open file descriptor for new PNG file.
 */
{
    png_image	png_output;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );

    png_output.opaque = NULL;
    png_output.version = PNG_IMAGE_VERSION;
    png_output.width = DeVAS_image_n_cols ( image );
    png_output.height = DeVAS_image_n_rows ( image );
    png_output.format = PNG_FORMAT_GRAY;
    png_output.flags =
    png_output.colormap_entries = 0;
    png_output.warning_or_error = 0;
    png_output.message[0] = '\0';

    png_image_write_to_stdio ( &png_output, output, 0 /*convert_to_8bit*/,
	    (void *) ( &DeVAS_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/);
}

