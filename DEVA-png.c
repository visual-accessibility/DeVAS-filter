/*
 * Read/write DEVA RGB/gray image object from/to PNG files.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include "deva-image.h"
#include "DEVA-png.h"
#include "deva-license.h"

DEVA_RGB_image *
DEVA_RGB_image_from_filename_png ( char *filename )
/*
 * Read a PNG file and return pixel values as a DEVA_RGB_image image object.
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
	exit ( EXIT_FAILURE );
    }

    return ( DEVA_RGB_image_from_file_png ( file ) );
}

DEVA_RGB_image *
DEVA_RGB_image_from_file_png ( FILE *input )
/*
 * Read a PNG file and return pixel values as a DEVA_RGB_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as RGB color values.
 *
 * input:	open file descriptor for an existing PNG file.
 */
{
    png_image	    png_output;
    DEVA_RGB_image  *image;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );
    png_output.version = PNG_IMAGE_VERSION;

    if ( png_image_begin_read_from_stdio ( &png_output, input ) == 0 ) {
	fprintf ( stderr, "DEVA_RGB_image_from_file_png: error reading file!" );
	exit ( EXIT_FAILURE );
    }

    png_output.format = PNG_FORMAT_RGB;

    image = DEVA_RGB_image_new ( png_output.height, png_output.width );

    if ( png_image_finish_read ( &png_output, NULL/*background*/,
	    (void *) ( &DEVA_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL/*colormap*/) == 0 ) {
	fprintf ( stderr, "DEVA_RGB_image_from_file_png: error reading file!" );
	exit ( EXIT_FAILURE );
    }

    return ( image );
}

DEVA_gray_image *
DEVA_gray_image_from_filename_png ( char *filename )
/*
 * Read a PNG file and return pixel values as a DEVA_gray_image image object.
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
	exit ( EXIT_FAILURE );
    }

    return ( DEVA_gray_image_from_file_png ( file ) );
}

DEVA_gray_image *
DEVA_gray_image_from_file_png ( FILE *input )
/*
 * Read a PNG file and return pixel values as a DEVA_gray_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as grayscale values.
 *
 * input:	open file descriptor for an existing PNG file.
 */
{
    png_image	    png_output;
    DEVA_gray_image  *image;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );
    png_output.version = PNG_IMAGE_VERSION;

    if ( png_image_begin_read_from_stdio ( &png_output, input ) == 0 ) {
	fprintf ( stderr,
		"DEVA_gray_image_from_file_png: error reading file!" );
	exit ( EXIT_FAILURE );
    }

    png_output.format = PNG_FORMAT_GRAY;

    image = DEVA_gray_image_new ( png_output.height, png_output.width );

    if ( png_image_finish_read ( &png_output, NULL/*background*/,
	    (void *) ( &DEVA_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL/*colormap*/) == 0 ) {
	fprintf ( stderr,
		"DEVA_gray_image_from_file_png: error reading file!" );
	exit ( EXIT_FAILURE );
    }

    return ( image );
}

/*
 * Read a PNG file and return pixel values as a DEVA_gray_image image object.
 * Pixels in file can be either grayscale or color, but in both cases are
 * returned as grayscale values.
 *
 * input:	open file descriptor for an existing PNG file.
 */

void
DEVA_RGB_image_to_filename_png ( char *filename, DEVA_RGB_image *image )
/*
 * Write the pixel values from a DEVA_RGB_image to a RGB color PNG file.
 *
 * filename:	name of new PNG file.
 */
{
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    DEVA_RGB_image_to_file_png ( file, image );

    fclose ( file );
}

void
DEVA_RGB_image_to_file_png ( FILE *output, DEVA_RGB_image *image )
/*
 * Write the pixel values from a DEVA_RGB_image to a RGB color PNG file.
 *
 * output:	open file descriptor for new PNG file.
 */
{
    png_image	png_output;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );

    png_output.opaque = NULL;
    png_output.version = PNG_IMAGE_VERSION;
    png_output.width = DEVA_image_n_cols ( image );
    png_output.height = DEVA_image_n_rows ( image );
    png_output.format = PNG_FORMAT_RGB;
    png_output.flags = 0;
    png_output.colormap_entries = 0;
    png_output.warning_or_error = 0;
    png_output.message[0] = '\0';

    if ( png_image_write_to_stdio ( &png_output, output, 0 /*convert_to_8bit*/,
	    (void *) ( &DEVA_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/) == 0 ) {
	fprintf ( stderr, "DEVA_RGB_image_to_file_png: error writing file!" );
	exit ( EXIT_FAILURE );
    }
}

void
DEVA_gray_image_to_filename_png ( char *filename, DEVA_gray_image *image )
{
/*
 * Write the pixel values from a DEVA_gray_image to a grayscale PNG file.
 *
 * filename:	name of new PNG file.
 */
    FILE    *file;

    file = fopen ( filename, "wb" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    DEVA_gray_image_to_file_png ( file, image );

    fclose ( file );
}

void
DEVA_gray_image_to_file_png ( FILE *output, DEVA_gray_image *image )
/*
 * Write the pixel values from a DEVA_gray_image to a grayscale PNG file.
 *
 * output:	open file descriptor for new PNG file.
 */
{
    png_image	png_output;

    /* Initialize the 'png_image' structure. */
    memset ( &png_output, 0, sizeof ( png_output ) );

    png_output.opaque = NULL;
    png_output.version = PNG_IMAGE_VERSION;
    png_output.width = DEVA_image_n_cols ( image );
    png_output.height = DEVA_image_n_rows ( image );
    png_output.format = PNG_FORMAT_GRAY;
    png_output.flags =
    png_output.colormap_entries = 0;
    png_output.warning_or_error = 0;
    png_output.message[0] = '\0';

    png_image_write_to_stdio ( &png_output, output, 0 /*convert_to_8bit*/,
	    (void *) ( &DEVA_image_data ( image, 0, 0 ) ), 0 /*row_stride*/,
	    NULL /*colormap*/);
}

