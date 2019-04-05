/*
 * Hodge-podge of potentially useful routines.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "devas-utils.h"
#include "devas-image.h"
#include "devas-license.h"	/* DeVAS open source license */

char *
strcat_safe ( char *dest, char *src )
/*
 *  Appends the src string to the dest string, reallocating the dest string
 *  to make sure that it is big enought.
 *
 *  The dest string *must* have started out as NULL or a malloc'ed value!
 *  If value of dest is NULL, form of call should be:
 *
 *  	dest = strcat_safe ( dest, src);
 *
 *  (This can be used whether or not the value of dest is NULL.)
 */
{
    char    *returned_string;

    if ( dest == NULL ) {
	returned_string = strdup ( src );
    } else {
	returned_string = (char *) realloc ( dest, sizeof ( char ) *
		( strlen ( dest ) + strlen ( src ) + 1 ) );
			/* size of two strings plus trailing '\0' */
	if ( returned_string != NULL ) {
	    strcat ( returned_string, src );
	} else {
	    fprintf ( stderr, "DeVAS:strcat_safe: malloc failed!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}
    }

    return ( returned_string );
}

DeVAS_float_image *
DeVAS_float_image_dup ( DeVAS_float_image *original_image )
/*
 * Make a copy of a DeVAS_float_image
 * [Currently unused.]
 */
{
    DeVAS_float_image	*duplicate_image;
    int			row, col;

    duplicate_image =
	DeVAS_float_image_new ( DeVAS_image_n_rows ( original_image ),
		DeVAS_image_n_cols ( original_image ) );
    DeVAS_image_view ( duplicate_image ) = DeVAS_image_view ( original_image );
    if ( DeVAS_image_description ( original_image ) == NULL ) {
	DeVAS_image_description ( duplicate_image ) = NULL;
    } else {
	DeVAS_image_description ( duplicate_image ) =
	    strdup ( DeVAS_image_description ( original_image ) );
	if ( DeVAS_image_description ( duplicate_image ) == NULL ) {
	    fprintf ( stderr, "DeVAS_float_image_dup: malloc failed!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}
    }

    for ( row = 0; row < DeVAS_image_n_rows ( original_image ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( original_image ); col++ ) {
	    DeVAS_image_data ( duplicate_image, row, col ) =
		DeVAS_image_data ( original_image, row, col );
	}
    }

    return ( duplicate_image );
}

void
DeVAS_float_image_addto ( DeVAS_float_image *i1, DeVAS_float_image *i2 )
/*
 * Add values of i2 to values of i1
 */
{
    int     row, col;

    if ( ! DeVAS_float_image_samesize ( i1, i2 ) ) {
	fprintf ( stderr,
		"DeVAS_float_image_addto: array sizes don't match!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < DeVAS_image_n_rows ( i1 ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( i1 ); col++ ) {
	    DeVAS_image_data ( i1, row, col ) +=
		DeVAS_image_data ( i2, row, col );
	}
    }
}

void
DeVAS_float_image_scalarmult ( DeVAS_float_image *i, float m )
/*
 * Multiply values of i by m;
 * [Currently unused.]
 */
{
    int     row, col;

    for ( row = 0; row < DeVAS_image_n_rows ( i ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( i ); col++ ) {
	    DeVAS_image_data ( i, row, col ) *= m;
	}
    }
}

double
degree2radian ( double degrees )
/*
 * [Currently unused.]
 */
{
    return ( degrees * ( ( 2.0 * M_PI ) / 360.0 ) );
}

double
radian2degree ( double radians )
{
    return ( radians * ( 360.0 / ( 2.0 * M_PI ) ) );
}

int
imax ( int x, int y )
{
    return ( ( x > y ) ? x : y );
}

int
imin ( int x, int y )
{
    return ( ( x < y ) ? x : y );
}
