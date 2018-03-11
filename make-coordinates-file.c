/*
 * Make a coordinates file specifing units of distance, coordinate system
 * orientation, and viewpoint for a set of DEVA geometry files.
 *
 * Units can be meters, centimeters, feet, or inches.  If desired, the
 * standard_units function can be used to converts these to a canonical
 * form (centimeters).  Note that the units apply to xyz and dst files,
 * but *not* to nor files with specify unit vectors.
 *
 * The coordinates file consists of two lines of text.  The first line
 * specifies the units.  The second line is a text representation of the
 * VIEW record taken from the cooresponding Radiance HDR file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "radiance-header.h"
#include "read-geometry.h"
#include "deva-license.h"	/* DEVA open source license */

char	*Usage =
	    "make-coordinates-file units radiance-file.hdr coordinates-file";
int	args_needed = 3;

int
main ( int argc, char *argv[] )
{
    char		*unit;
    char		*radiance_filename;
    FILE		*radiance_fp;
    char		*coordinates_filename;
    FILE		*coordinates_fp;
    VIEW		view;
    int			argpt = 1;

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );	/* error exit */
    }

    unit = argv[argpt++];

    radiance_filename = argv[argpt++];

    radiance_fp = fopen ( radiance_filename, "r" );
    if ( radiance_fp == NULL ) {
	perror ( radiance_filename );
	exit ( EXIT_FAILURE );
    }

    coordinates_filename = argv[argpt++];

    coordinates_fp = fopen ( coordinates_filename, "w" );
    if ( coordinates_fp == NULL ) {
	perror ( coordinates_filename );
	exit ( EXIT_FAILURE );
    }

    if ( ( strcmp ( unit, "centimeters" ) != 0 ) &&
	    ( strcmp ( unit, "meters" ) != 0 ) &&
	    ( strcmp ( unit, "inches" ) != 0 ) &&
	    ( strcmp ( unit, "feet" ) != 0 ) ) {
	fprintf ( stderr, "invalid unit (%s)!\n", unit );
	exit ( EXIT_FAILURE );
    }

    /* get VIEW record from Radiance file header */
    DEVA_read_radiance_header ( radiance_fp, NULL, NULL, NULL, &view,
	    NULL, NULL, NULL );

    if ( view.type == 0 ) {
	fprintf ( stderr, "invalid or missing VIEW record!\n" );
	exit ( EXIT_FAILURE );
    }

    /* write two line coordinates file */
    fprintf ( coordinates_fp, "distance-units=%s\n", unit );

    fputs ( VIEWSTR, coordinates_fp );
    fprintview ( &view, coordinates_fp );
    putc ( '\n', coordinates_fp );

    return ( EXIT_SUCCESS );
}
