/*
 * Read DEVA geometry files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "read-geometry.h"
#include "radiance-header.h"
#include "deva-image.h"
#include "radiance/copyright.h" /* Radiance open source license */
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/color.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "deva-license.h"	/* DEVA open source license */

#define	CENTIMETERS_TO_CENTIMETERS	1.0
#define	METERS_TO_CENTIMETERS		100.0
#define	INCHES_TO_CENTIMETERS		2.54
#define	FEET_TO_CENTIMETERS		30.48

#ifndef TRUE
#define TRUE            1
#endif  /* TRUE */
#ifndef FALSE
#define FALSE           0
#endif  /* FALSE */

#define	HEADER_MAXLINE		2048	/* same as for getheader */

char	*progname;			/* Radiance requires this be global */

VIEW	DEVA_null_view;			/* from radiance-header.c */

/*
 * properties extracted from header (need to be globals because of how
 * radiance decodes headers)
 */

VIEW
deva_get_VIEW_from_filename ( char *filename, int *n_rows_p, int *n_cols_p )
{
    FILE		*radiance_fp;
    VIEW		view;
    int			n_rows, n_cols;

    radiance_fp = fopen ( filename, "r" );
    if ( radiance_fp == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    NULL, &view, NULL, NULL, NULL );

    fclose ( radiance_fp );

    if ( n_rows_p != NULL ) {
	*n_rows_p = n_rows;
    } 
    if ( n_cols_p != NULL ) {
	*n_cols_p = n_cols;
    }

    return ( view );
}

void
deva_print_VIEW ( VIEW view )
{
    switch ( view.type ) {
	case VT_PER:
	    printf ( "view type = perspective\n" );
	    break;

	case VT_PAR:
	    printf ( "view type = parallel\n" );
	    break;

	case VT_ANG:
	    printf ( "view type = angular fisheye\n" );
	    break;

	case VT_HEM:
	    printf ( "view type = hemispherical fisheye\n" );
	    break;

	case VT_PLS:
	    printf ( "view type = planispheric fisheye\n" );
	    break;

	case VT_CYL:
	    printf ( "view type = cylindrical panorama\n" );
	    break;

	default:
	    printf ( "unknown view type!\n" );
	    break;
    }

    printf ( "view origin = (%f, %f, %f)\n", view.vp[0], view.vp[1],
	    view.vp[2] );
    printf ( "view direction = (%f, %f, %f)\n", view.vdir[0], view.vdir[1],
	    view.vdir[2] );
    printf ( "view up = (%f, %f, %f)\n", view.vup[0], view.vup[1],
	    view.vup[2] );
    printf ( "view distance = %f\n", view.vdist );
    printf ( "hFOV = %f, vFOV = %f\n", view.horiz, view.vert );
    printf ( "horizontal image vector = (%f, %f, %f)\n", view.hvec[0],
	    view.hvec[1], view.hvec[2] );
    printf ( "vertical image vector = (%f, %f, %f)\n", view.vvec[0],
	    view.vvec[1], view.vvec[2] );
}

int
DEVA_geom_dim_from_radfilename ( char *filename )
/*
 * 1-D or 3-D data?
 */
{
    FILE    *radiance_fp;
    int	    dimensions;

    radiance_fp = fopen ( filename, "r" );
    if ( radiance_fp == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    dimensions = DEVA_geom_dim_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( dimensions );
}

int
DEVA_geom_dim_from_radfile ( FILE *radiance_fp )
/*
 * 1-D or 3-D data?
 */
{
    int	    n_rows, n_cols;
    char    header_line[HEADER_MAXLINE];
    float   v1, v2, v3, v4;
    int	    dimensions;

    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom_dim_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( strcmp ( header_line, "#?RADIANCE\n" ) != 0 ) {
	fprintf ( stderr, "DEVA_geom_dim_from_radfile: not RADIANCE file!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    /* get started on skipping through header */
    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom_dim_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( ( strlen ( header_line ) > 1 ) &&
	    ( header_line[strlen(header_line)-1] != '\n' ) ) {
	fprintf ( stderr, "DEVA_geom_dim_from_radfile: line too long!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    /* main section of header ends with a blank line */
    while ( header_line[0] != '\n' ) {

	if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	    perror ( "DEVA_geom_dim_from_radfile" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
	if ( ( strlen ( header_line ) > 1 ) &&
		( header_line[strlen(header_line)-1] != '\n' ) ) {
	    fprintf ( stderr, "DEVA_geom_dim_from_radfile: line too long!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
    }

    /* get dimensions (note need to skip trailing newline!) */
    if ( fscanf ( radiance_fp, "-Y %d +X %d\n", &n_rows, &n_cols ) != 2 ) {
	fprintf ( stderr,
		"DEVA_geom_dim_from_radfile: invalid RADIANCE file!\n" );
	exit ( EXIT_FAILURE );        /* error return */
    }

    /*
     * The header does not indicate whether values are 1D or 3D.  To figure
     * this out, assume that 1D values are written one to a line and 3D values
     * are written three to a line.
     */

    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom_dim_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( ( strlen ( header_line ) > 1 ) &&
	    ( header_line[strlen(header_line)-1] != '\n' ) ) {
	fprintf ( stderr, "DEVA_geom_dim_from_radfile: line too long!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    dimensions = sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 );
    
    if ( ( dimensions == 1 ) || ( dimensions == 3 ) ) {
	return ( dimensions );
    } else {
	fprintf ( stderr,
		"DEVA_geom_dim_from_radfile: not 1-D or 3-D data!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }
}

DEVA_XYZ_image *
DEVA_geom3d_from_radfilename ( char *filename )
{
    DEVA_XYZ_image  *deva_image;	/* reuse CIE XYZ for geometery xyz */
    FILE	    *radiance_fp;

    radiance_fp = fopen ( filename, "r" );
    if ( radiance_fp == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    deva_image = DEVA_geom3d_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( deva_image );
}

DEVA_XYZ_image *
DEVA_geom3d_from_radfile ( FILE *radiance_fp )
{
    DEVA_XYZ_image  *deva_image;
    int		    n_rows, n_cols;
    int		    row, col;
    char	    header_line[HEADER_MAXLINE];
    float	    v1, v2, v3, v4;
    int		    dimensions;

    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom3d_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( strcmp ( header_line, "#?RADIANCE\n" ) != 0 ) {
	fprintf ( stderr, "DEVA_geom3d_from_radfile: not RADIANCE file!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    /* get started on skipping through header */
    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom3d_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( ( strlen ( header_line ) > 1 ) &&
	    ( header_line[strlen(header_line)-1] != '\n' ) ) {
	fprintf ( stderr, "DEVA_geom3d_from_radfile: line too long!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    /* main section of header ends with a blank line */
    while ( header_line[0] != '\n' ) {

	if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	    perror ( "DEVA_geom3d_from_radfile" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
	if ( ( strlen ( header_line ) > 1 ) &&
		( header_line[strlen(header_line)-1] != '\n' ) ) {
	    fprintf ( stderr, "DEVA_geom3d_from_radfile: line too long!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
    }

    /* get dimensions (note need to skip trailing newline!) */
    if ( fscanf ( radiance_fp, "-Y %d +X %d\n", &n_rows, &n_cols ) != 2 ) {
	fprintf ( stderr,
		"DEVA_geom3d_from_radfile: invalid RADIANCE file!\n" );
	exit ( EXIT_FAILURE );        /* error return */
    }

    /*
     * The header does not indicate whether values are 1D or 3D.  To figure
     * this out, assume that 1D values are written one to a line and 3D values
     * are written three to a line.
     */

    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom3d_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( ( strlen ( header_line ) > 1 ) &&
	    ( header_line[strlen(header_line)-1] != '\n' ) ) {
	fprintf ( stderr, "DEVA_geom3d_from_radfile: line too long!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    dimensions = sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 );

    if ( dimensions != 3 ) {
	fprintf ( stderr, "DEVA_geom3d_from_radfile: not 3-D data!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    } else {
	/* first line of input file */
	deva_image = DEVA_XYZ_image_new ( n_rows, n_cols );

	DEVA_image_data ( deva_image, 0, 0 ) . X = v1;
	DEVA_image_data ( deva_image, 0, 0 ) . Y = v2;
	DEVA_image_data ( deva_image, 0, 0 ) . Z = v3;
    }

    /* remainder of first row */
    for ( col = 1; col < n_cols; col++ ) {
	if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	    perror ( "DEVA_geom3d_from_radfile" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
	if ( ( strlen ( header_line ) > 1 ) &&
		( header_line[strlen(header_line)-1] != '\n' ) ) {
	    fprintf ( stderr, "DEVA_geom3d_from_radfile: line too long!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}

	if ( sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 ) != 3 ) {
	    fprintf ( stderr, "DEVA_geom3d_from_radfile: not 3-D data!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}

	DEVA_image_data ( deva_image, 0, col ) . X = v1;
	DEVA_image_data ( deva_image, 0, col ) . Y = v2;
	DEVA_image_data ( deva_image, 0, col ) . Z = v3;
    }

    /* remaining rows */
    for ( row = 1; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
		perror ( "DEVA_geom3d_from_radfile" );
		exit ( EXIT_FAILURE );	/* error return */
	    }
	    if ( ( strlen ( header_line ) > 1 ) &&
		    ( header_line[strlen(header_line)-1] != '\n' ) ) {
		fprintf ( stderr,
			"DEVA_geom3d_from_radfile: line too long!\n" );
		exit ( EXIT_FAILURE );	/* error return */
	    }


	    if ( sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 )
		    != 3 ) {
		fprintf ( stderr, "DEVA_geom3d_from_radfile: not 3-D data!\n" );
		exit ( EXIT_FAILURE );	/* error return */
	    }

	    DEVA_image_data ( deva_image, row, col ) . X = v1;
	    DEVA_image_data ( deva_image, row, col ) . Y = v2;
	    DEVA_image_data ( deva_image, row, col ) . Z = v3;
	}
    }

    return ( deva_image );
}

DEVA_float_image *
DEVA_geom1d_from_radfilename ( char *filename )
{
    DEVA_float_image	*deva_image;
    FILE		*radiance_fp;

    radiance_fp = fopen ( filename, "r" );
    if ( radiance_fp == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    deva_image = DEVA_geom1d_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( deva_image );
}

DEVA_float_image *
DEVA_geom1d_from_radfile ( FILE *radiance_fp )
{
    DEVA_float_image	*deva_image;
    int			n_rows, n_cols;
    int			row, col;
    char		header_line[HEADER_MAXLINE];
    float		v1, v2, v3, v4;
    int			dimensions;

    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom1d_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( strcmp ( header_line, "#?RADIANCE\n" ) != 0 ) {
	fprintf ( stderr, "DEVA_geom1d_from_radfile: not RADIANCE file!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    /* get started on skipping through header */
    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom1d_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( ( strlen ( header_line ) > 1 ) &&
	    ( header_line[strlen(header_line)-1] != '\n' ) ) {
	fprintf ( stderr, "DEVA_geom1d_from_radfile: line too long!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    /* main section of header ends with a blank line */
    while ( header_line[0] != '\n' ) {

	if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	    perror ( "DEVA_geom1d_from_radfile" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
	if ( ( strlen ( header_line ) > 1 ) &&
		( header_line[strlen(header_line)-1] != '\n' ) ) {
	    fprintf ( stderr, "DEVA_geom1d_from_radfile: line too long!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
    }

    /* get dimensions (note need to skip trailing newline!) */
    if ( fscanf ( radiance_fp, "-Y %d +X %d\n", &n_rows, &n_cols ) != 2 ) {
	fprintf ( stderr,
		"DEVA_geom1d_from_radfile: invalid RADIANCE file!\n" );
	exit ( EXIT_FAILURE );        /* error return */
    }

    /*
     * The header does not indicate whether values are 1D or 3D.  To figure
     * this out, assume that 1D values are written one to a line and 3D values
     * are written three to a line.
     */

    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	perror ( "DEVA_geom1d_from_radfile" );
	exit ( EXIT_FAILURE );	/* error return */
    }
    if ( ( strlen ( header_line ) > 1 ) &&
	    ( header_line[strlen(header_line)-1] != '\n' ) ) {
	fprintf ( stderr, "DEVA_geom1d_from_radfile: line too long!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    }

    dimensions = sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 );

    if ( dimensions != 1 ) {
	fprintf ( stderr, "DEVA_geom1d_from_radfile: not 1-D data!\n" );
	exit ( EXIT_FAILURE );	/* error return */
    } else {
	/* first line of input file */
	deva_image = DEVA_float_image_new ( n_rows, n_cols );

	DEVA_image_data ( deva_image, 0, 0 ) = v1;
    }

    /* remainder of first row */
    for ( col = 1; col < n_cols; col++ ) {
	if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
	    perror ( "DEVA_geom3d_from_radfile" );
	    exit ( EXIT_FAILURE );	/* error return */
	}
	if ( ( strlen ( header_line ) > 1 ) &&
		( header_line[strlen(header_line)-1] != '\n' ) ) {
	    fprintf ( stderr, "DEVA_geom3d_from_radfile: line too long!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}

	if ( sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 ) != 1 ) {
	    fprintf ( stderr, "DEVA_geom1d_from_radfile: not 1-D data!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}

	DEVA_image_data ( deva_image, 0, col ) = v1;
    }

    /* remaining rows */
    for ( row = 1; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( fgets ( header_line, HEADER_MAXLINE, radiance_fp ) == NULL ) {
		perror ( "DEVA_geom3d_from_radfile" );
		exit ( EXIT_FAILURE );	/* error return */
	    }
	    if ( ( strlen ( header_line ) > 1 ) &&
		    ( header_line[strlen(header_line)-1] != '\n' ) ) {
		fprintf ( stderr,
			"DEVA_geom3d_from_radfile: line too long!\n" );
		exit ( EXIT_FAILURE );	/* error return */
	    }

	    if ( sscanf ( header_line, "%f %f %f %f", &v1, &v2, &v3, &v4 )
		    != 1 ) {
		fprintf ( stderr, "DEVA_geom1d_from_radfile: not 1-D data!\n" );
		exit ( EXIT_FAILURE );	/* error return */
	    }

	    DEVA_image_data ( deva_image, row, col ) = v1;
	}
    }

    return ( deva_image );
}

DEVA_coordinates *
DEVA_coordinates_from_filename ( char *filename )
{
    DEVA_coordinates	*coordinates;
    FILE		*file;

    file = fopen ( filename, "r" );
    if ( file == NULL ) {
	perror ( filename );
	exit ( EXIT_FAILURE );
    }

    coordinates = DEVA_coordinates_from_file ( file );

    fclose ( file );

    return ( coordinates );
}

DEVA_coordinates *
DEVA_coordinates_from_file ( FILE *file )
{
    DEVA_coordinates	*coordinates;
    char		header_string[500];
    char		units_string[100];

    fgets ( header_string, 500, file );
    if ( ( strlen ( header_string ) > 0 ) && ( header_string[strlen (
		    header_string ) - 1] != '\n' ) ) {
	fprintf ( stderr, "invalid UNITS value!\n" );
	exit ( EXIT_FAILURE ); }

    if ( sscanf ( header_string, "distance-units=%100s", units_string ) != 1 ) {
	fprintf ( stderr, "invalid UNITS value!\n" );
	exit ( EXIT_FAILURE );
    }

    coordinates = DEVA_coordinates_new ( );

    coordinates->view = DEVA_null_view;

    fgets ( header_string, 500, file );

    if ( strlen ( header_string ) > 0 ) {

	if ( header_string[strlen ( header_string ) - 1] != '\n' ) {
	    fprintf ( stderr, "invalid UNITS value!\n" );
	    exit ( EXIT_FAILURE );

	} else if ( strncmp ( header_string, "VIEW=", strlen ( "VIEW=" ) )
								== 0 ) {
	    sscanview ( &( coordinates->view ),
		    header_string + strlen ( "VIEW=" ) );

	} else {
	    fprintf ( stderr, "invalid UNITS value!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    if ( strcmp ( units_string, "centimeters" ) == 0 ) {
	coordinates->units = centimeters;
	coordinates->convert_to_centimeters = CENTIMETERS_TO_CENTIMETERS;
    } else if ( strcmp ( units_string, "meters" ) == 0 ) {
	coordinates->units = meters;
	coordinates->convert_to_centimeters = METERS_TO_CENTIMETERS;
    } else if ( strcmp ( units_string, "inches" ) == 0 ) {
	coordinates->units = inches;
	coordinates->convert_to_centimeters = INCHES_TO_CENTIMETERS;
    } else if ( strcmp ( units_string, "feet" ) == 0 ) {
	coordinates->units = feet;
	coordinates->convert_to_centimeters = FEET_TO_CENTIMETERS;
    } else {
	fprintf ( stderr, "invalid UNITS value!\n" );
	exit ( EXIT_FAILURE );
    }

    return ( coordinates );
}

void
DEVA_print_coordinates ( DEVA_coordinates *coordinates )
{
    switch ( coordinates->units ) {
	case unknown_unit:
	    fprintf ( stderr, "DEVA_print_coordinates: invalid units!\n" );
	    exit ( EXIT_FAILURE );
	    break;

	case centimeters:
	    printf ( "distance-units=centimeters\n" );
	    break;

	case meters:
	    printf ( "distance-units=meters\n" );
	    break;

	case inches:
	    printf ( "distance-units=inches\n" );
	    break;

	case feet:
	    printf ( "distance-units=feet\n" );
	    break;

	default:
	    fprintf ( stderr, "DEVA_print_coordinates: invalid units!\n" );
	    exit ( EXIT_FAILURE );
    }

    printf ( "VIEW=" );
    fprintview ( &(coordinates->view), stdout );
    printf ( "\n" );
}

DEVA_coordinates *
DEVA_coordinates_new ( void )
{
    DEVA_coordinates	*coordinates;

    coordinates = (DEVA_coordinates *) malloc ( sizeof ( DEVA_coordinates ) );
    if ( coordinates == NULL ) {
	fprintf ( stderr, "DEVA_coordinates_new: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    return ( coordinates );
}

void
DEVA_coordinates_delete ( DEVA_coordinates *coordinates )
{
    free ( coordinates );
}

void
standard_units_3D ( DEVA_XYZ_image *threeDgeom, DEVA_coordinates *coordinates )
/*
 * Convert values in threeDgeom to standard units (e.g., centemeters),
 * based on the original units specified in coordinates.
 */
{
    int     row, col;
    double  conversion;

    conversion = coordinates->convert_to_centimeters;

    for ( row = 0; row < DEVA_image_n_rows ( threeDgeom ); row++ )
    {
	for ( col = 0; col < DEVA_image_n_cols ( threeDgeom ); col++ ) {
	    DEVA_image_data ( threeDgeom, row, col ) . X *= conversion;
	    DEVA_image_data ( threeDgeom, row, col ) . Y *= conversion;
	    DEVA_image_data ( threeDgeom, row, col) .  Z *= conversion;
	}
    }
}

void
standard_units_1D ( DEVA_float_image *oneDgeom, DEVA_coordinates *coordinates )
/*
 * Convert values in threeDgeom to standard units (e.g., centemeters),
 * based on the original units specified in coordinates.
 */
{
    int     row, col;
    double  conversion;

    conversion = coordinates->convert_to_centimeters;

    for ( row = 0; row < DEVA_image_n_rows ( oneDgeom ); row++ )
    {
	for ( col = 0; col < DEVA_image_n_cols ( oneDgeom ); col++ ) {
	    DEVA_image_data ( oneDgeom, row, col ) *= conversion;
	}
    }
}
