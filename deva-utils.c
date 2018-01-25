/*
 * Hodge-podge of potentially useful routines.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "deva-utils.h"
#include "deva-image.h"
#include "deva-license.h"	/* DEVA open source license */

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
	    fprintf ( stderr, "DEVA:strcat_safe: malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    return ( returned_string );
}

DEVA_float_image *
DEVA_float_image_dup ( DEVA_float_image *original_image )
/*
 * Make a copy of a DEVA_float_image
 */
{
    DEVA_float_image	*duplicate_image;
    int			row, col;

    duplicate_image =
	DEVA_float_image_new ( DEVA_image_n_rows ( original_image ),
		DEVA_image_n_cols ( original_image ) );
    DEVA_image_view ( duplicate_image ) = DEVA_image_view ( original_image );
    if ( DEVA_image_description ( original_image ) == NULL ) {
	DEVA_image_description ( duplicate_image ) = NULL;
    } else {
	DEVA_image_description ( duplicate_image ) =
	    strdup ( DEVA_image_description ( original_image ) );
	if ( DEVA_image_description ( duplicate_image ) == NULL ) {
	    fprintf ( stderr, "DEVA_float_image_dup: malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    for ( row = 0; row < DEVA_image_n_rows ( original_image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( original_image ); col++ ) {
	    DEVA_image_data ( duplicate_image, row, col ) =
		DEVA_image_data ( original_image, row, col );
	}
    }

    return ( duplicate_image );
}

void
DEVA_float_image_addto ( DEVA_float_image *i1, DEVA_float_image *i2 )
/*
 * Add values of i2 to values of i1
 */
{
    int     row, col;

    if ( ! DEVA_float_image_samesize ( i1, i2 ) ) {
	fprintf ( stderr,
		"DEVA_float_image_addto: array sizes don't match!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < DEVA_image_n_rows ( i1 ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( i1 ); col++ ) {
	    DEVA_image_data ( i1, row, col ) +=
		DEVA_image_data ( i2, row, col );
	}
    }
}

void
DEVA_float_image_scalarmult ( DEVA_float_image *i, float m )
/*
 * Multiply values of i by m;
 */
{
    int     row, col;

    for ( row = 0; row < DEVA_image_n_rows ( i ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( i ); col++ ) {
	    DEVA_image_data ( i, row, col ) *= m;
	}
    }
}

double
degree2radian ( double degrees )
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
