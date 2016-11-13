#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "deva-image.h"
#include "deva-utils.h"
#include "radianceIO.h"
#include "radiance/color.h"
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "deva-license.h"	/* DEVA open source license */

/* need to be global because of the way Radiance reads header lines */
static char		    *old_header = NULL;
static VIEW		    view = NULLVIEW;	/* in case no VIEW records */
						/* in header */
static VIEW		    nullview = NULLVIEW;/* so it can be assigned */
static RadianceColorFormat  color_format;
static int  headline ( char *s, void *p );

DEVA_float_image *
DEVA_brightness_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory brightness image.  Units of returned image are as for
 * rgbe-format Radiance files (watts/steradian/sq.meter over the visible
 * spectrum).
 *
 * A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DEVA_float_image	*brightness;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    brightness = DEVA_brightness_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( brightness );
}

DEVA_float_image *
DEVA_brightness_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory brightness image.  Units of returned image are as for
 * rgbe-format Radiance files (watts/steradian/sq.meter over the visible
 * spectrum).
 */
{
    DEVA_float_image	*brightness;
    COLOR		*radiance_scanline;
    int			row, col;
    int			n_rows, n_cols;
    int			scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    if ( old_header != NULL ) {
	free ( old_header );
    }
    old_header = NULL;
    view = nullview;	/* in case no VIEW records */

    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_brightness_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( ( scanline_ordering = fgetresolu ( &n_cols, &n_rows, radiance_fp ) )
	    < 0 ) {
	fprintf ( stderr,
		"DEVA_brightness_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( scanline_ordering != PIXSTANDARD ) {
	fprintf ( stderr,
	    "DEVA_brightness_from_radfile: non-standard scanline ordering!\n" );
	exit ( EXIT_FAILURE );
    }

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_brightness_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    brightness = DEVA_float_image_new ( n_rows, n_cols );
    DEVA_image_view ( brightness ) = view;	/* view set in headline ( ) */
    DEVA_image_description ( brightness ) = old_header;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_brightness_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( brightness, row, col ) =
					bright ( radiance_scanline[col] );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( brightness, row, col ) =
		    colval ( radiance_scanline[col], CIEY ) / DEVA_WHTEFFICACY;
	    }
	} else {
	    fprintf ( stderr,
		    "DEVA_brightness_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( brightness );
}

void
DEVA_brightness_to_radfilename ( char *filename, DEVA_float_image *brightness )
/*
 * Writes an in-memory brightness image to a Radiance rgbe image file
 * specified by pathname.  Units of the in-memory brightness image are
 * as for rgbe-format Radiance files (watts/steradian/sq.meter over the
 * visible spectrum).
 *
 * A pathname of "-" specifies standard output.
 */
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    DEVA_brightness_to_radfile ( radiance_fp, brightness );

    fclose ( radiance_fp );
}

void
DEVA_brightness_to_radfile ( FILE *radiance_fp, DEVA_float_image *brightness )
/*
 * Writes an in-memory brightness image to a Radiance rgbe image file
 * specified by an open file descriptor.  Units of the in-memory brightness
 * image are as for rgbe-format Radiance files (watts/steradian/sq.meter over
 * the visible spectrum).
 */
{
    int	   n_rows, n_cols;
    int	   row, col;
    COLOR  *radiance_scanline;

    n_rows = DEVA_image_n_rows ( brightness );
    n_cols = DEVA_image_n_cols ( brightness );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    newheader ( "RADIANCE", radiance_fp );

    if ( ( DEVA_image_description ( brightness ) != NULL ) &&
	    ( strlen ( DEVA_image_description ( brightness ) ) >= 1 ) ) {
	/* output history */
	fputs ( DEVA_image_description ( brightness ), radiance_fp );

	/* make sure history ends with a newline */
	if ( DEVA_image_description ( brightness )
	    [strlen ( DEVA_image_description ( brightness ) ) - 1] != '\n' ) {
	    fputc ('\n', radiance_fp );
	}
    }

    if ( DEVA_image_view ( brightness ) . type != '\0' ) {
	/* output view string if non-null */
	fputs ( VIEWSTR, radiance_fp );
	fprintview ( &DEVA_image_view ( brightness ), radiance_fp );
	putc ( '\n', radiance_fp );
    }

    /* only know how to write rgbe */
    fputformat ( COLRFMT, radiance_fp );

    fputc ('\n', radiance_fp );		/* ended of main header */

    fprtresolu ( n_cols, n_rows, radiance_fp );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_brightness_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],	/* output is grayscale */
		    DEVA_image_data ( brightness, row, col ),
		    DEVA_image_data ( brightness, row, col ),
		    DEVA_image_data ( brightness, row, col ) );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_brightness_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DEVA_float_image *
DEVA_luminance_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory luminance image.  Units of returned image are
 * (candela/m^2).  Note that this is not the unit used in rgbe-format
 * Radiance files.
 *
 * A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DEVA_float_image	*luminance;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    luminance = DEVA_luminance_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( luminance );
}

DEVA_float_image *
DEVA_luminance_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory luminance image.  Units of returned image are
 * (candela/m^2).  Note that this is not the unit used in rgbe-format
 * Radiance files.
 */
{
    DEVA_float_image	*luminance;	/* note name confilict with RADIANCE */
    					/* file color.h */
    COLOR		*radiance_scanline;
    int			row, col;
    int			n_rows, n_cols;
    int			scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    if ( old_header != NULL ) {
	free ( old_header );
    }
    old_header = NULL;
    view = nullview;	/* in case no VIEW records */

    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_luminance_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( ( scanline_ordering = fgetresolu ( &n_cols, &n_rows, radiance_fp ) )
	    < 0 ) {
	fprintf ( stderr,
		"DEVA_luminance_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( scanline_ordering != PIXSTANDARD ) {
	fprintf ( stderr,
	    "DEVA_luminance_from_radfile: non-standard scanline ordering!\n" );
	exit ( EXIT_FAILURE );
    }

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_luminance_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    luminance = DEVA_float_image_new ( n_rows, n_cols );
    DEVA_image_view ( luminance ) = view;	/* view set in headline ( ) */
    DEVA_image_description ( luminance ) = old_header;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_luminance_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( luminance, row, col ) =
			luminance ( radiance_scanline[col] );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( luminance, row, col ) =
		    	colval ( radiance_scanline[col], CIEY );
	    }
	} else {
	    fprintf ( stderr,
		    "DEVA_luminance_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( luminance );
}

void
DEVA_luminance_to_radfilename ( char *filename, DEVA_float_image *luminance )
/*
 * Writes an in-memory luminance image to a Radiance rgbe image file
 * specified by pathname.  Units of the in-memory luminance image are
 * assumed to be candela/m^2, with values converted to the standard
 * units of rgbe-format Radiance files (watts/steradian/sq.meter over
 * the visible spectrum).
 *
 * A pathname of "-" specifies standard output.
 */
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    DEVA_luminance_to_radfile ( radiance_fp, luminance );

    fclose ( radiance_fp );
}

void
DEVA_luminance_to_radfile ( FILE *radiance_fp, DEVA_float_image *luminance )
/*
 * Writes an in-memory luminance image to a Radiance rgbe image file specified
 * by an open file descriptor.  Units of the in-memory luminance image are
 * assumed to be candela/m^2, with values converted to the standard units of
 * rgbe-format Radiance files (watts/steradian/sq.meter over the visible
 * spectrum).
 */
{
    int	   n_rows, n_cols;
    int	   row, col;
    COLOR  *radiance_scanline;

    n_rows = DEVA_image_n_rows ( luminance );
    n_cols = DEVA_image_n_cols ( luminance );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    newheader ( "RADIANCE", radiance_fp );

    if ( ( DEVA_image_description ( luminance ) != NULL ) &&
	    ( strlen ( DEVA_image_description ( luminance ) ) >= 1 ) ) {
	/* output history */
	fputs ( DEVA_image_description ( luminance ), radiance_fp );

	/* make sure history ends with a newline */
	if ( DEVA_image_description ( luminance )
		[strlen ( DEVA_image_description ( luminance ) ) - 1] !=
				'\n' ) {
	    fputc ('\n', radiance_fp );
	}
    }

    if ( DEVA_image_view ( luminance ) . type != '\0' ) {
	/* output view string if non-null */
	fputs ( VIEWSTR, radiance_fp );
	fprintview ( &DEVA_image_view ( luminance ), radiance_fp );
	putc ( '\n', radiance_fp );
    }

    /* only know how to write rgbe */
    fputformat ( COLRFMT, radiance_fp );

    fputc ('\n', radiance_fp );

    fprtresolu ( n_cols, n_rows, radiance_fp );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_luminance_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],	/* output is grayscale */
		  DEVA_image_data ( luminance, row, col ) / DEVA_WHTEFFICACY,
		  DEVA_image_data ( luminance, row, col ) / DEVA_WHTEFFICACY,
		  DEVA_image_data ( luminance, row, col ) / DEVA_WHTEFFICACY );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_luminance_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DEVA_RGBf_image *
DEVA_RGBf_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory RGBf image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DEVA_RGBf_image	*RGBf;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    RGBf = DEVA_RGBf_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( RGBf );
}

DEVA_RGBf_image *
DEVA_RGBf_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory RGBf image.
 */
{
    DEVA_RGBf_image	*RGBf;
    COLOR		*radiance_scanline;
    COLOR		RGBf_rad_pixel;
    int			row, col;
    int			n_rows, n_cols;
    int			scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    if ( old_header != NULL ) {
	free ( old_header );
    }
    old_header = NULL;
    view = nullview;	/* in case no VIEW records */

    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_RGBf_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( ( scanline_ordering = fgetresolu ( &n_cols, &n_rows, radiance_fp ) )
	    < 0 ) {
	fprintf ( stderr,
		"DEVA_RGBf_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( scanline_ordering != PIXSTANDARD ) {
	fprintf ( stderr,
	    "DEVA_RGBf_from_radfile: non-standard scanline ordering!\n" );
	exit ( EXIT_FAILURE );
    }

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_RGBf_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    RGBf = DEVA_RGBf_image_new ( n_rows, n_cols );
    DEVA_image_view ( RGBf ) = view;	/* view set in headline ( ) */
    DEVA_image_description ( RGBf ) = old_header;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_RGBf_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( RGBf, row, col ) . red =
		    		colval ( radiance_scanline[col], RED );
		DEVA_image_data ( RGBf, row, col ) . green =
		    		colval ( radiance_scanline[col], GRN );
		DEVA_image_data ( RGBf, row, col ) . blue =
		    		colval ( radiance_scanline[col], BLU );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( RGBf_rad_pixel, xyz2rgbmat,
			radiance_scanline[col] );
		DEVA_image_data ( RGBf, row, col ) . red =
		    colval ( RGBf_rad_pixel, RED ) / DEVA_WHTEFFICACY;
		DEVA_image_data ( RGBf, row, col ) . green =
		    colval ( RGBf_rad_pixel, GRN ) / DEVA_WHTEFFICACY;
		DEVA_image_data ( RGBf, row, col ) . blue =
		    colval ( RGBf_rad_pixel, BLU ) / DEVA_WHTEFFICACY;
	    }
	} else {
	    fprintf ( stderr,
		    "DEVA_RGBf_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( RGBf );
}

void
DEVA_RGBf_to_radfilename ( char *filename, DEVA_RGBf_image *RGBf )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    DEVA_RGBf_to_radfile ( radiance_fp, RGBf );

    fclose ( radiance_fp );
}

void
DEVA_RGBf_to_radfile ( FILE *radiance_fp, DEVA_RGBf_image *RGBf )
/*
 * For now, only write rgbe format files.
 */
{
    int	   n_rows, n_cols;
    int	   row, col;
    COLOR  *radiance_scanline;

    n_rows = DEVA_image_n_rows ( RGBf );
    n_cols = DEVA_image_n_cols ( RGBf );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    newheader ( "RADIANCE", radiance_fp );

    if ( ( DEVA_image_description ( RGBf ) != NULL ) &&
	    ( strlen ( DEVA_image_description ( RGBf ) ) >= 1 ) ) {
	/* output history */
	fputs ( DEVA_image_description ( RGBf ), radiance_fp );

	/* make sure history ends with a newline */
	if ( DEVA_image_description ( RGBf )
		[strlen ( DEVA_image_description ( RGBf ) ) - 1] != '\n' ) {
	    fputc ('\n', radiance_fp );
	}
    }

    if ( DEVA_image_view ( RGBf ) . type != '\0' ) {
	/* output view string if non-null */
	fputs ( VIEWSTR, radiance_fp );
	fprintview ( &DEVA_image_view ( RGBf ), radiance_fp );
	putc ( '\n', radiance_fp );
    }

    /* only know how to write rgbe */
    fputformat ( COLRFMT, radiance_fp );

    fputc ('\n', radiance_fp );

    fprtresolu ( n_cols, n_rows, radiance_fp );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_RGBf_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],
		    DEVA_image_data ( RGBf, row, col ) . red,
		    DEVA_image_data ( RGBf, row, col ) . green,
		    DEVA_image_data ( RGBf, row, col ) . blue );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_RGBf_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DEVA_XYZ_image *
DEVA_XYZ_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory XYZ image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DEVA_XYZ_image	*XYZ;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    XYZ = DEVA_XYZ_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( XYZ );
}

DEVA_XYZ_image *
DEVA_XYZ_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory XYZ image.
 */
{
    DEVA_XYZ_image	*XYZ;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    int			row, col;
    int			n_rows, n_cols;
    int			scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    if ( old_header != NULL ) {
	free ( old_header );
    }
    old_header = NULL;
    view = nullview;	/* in case no VIEW records */

    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_XYZ_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( ( scanline_ordering = fgetresolu ( &n_cols, &n_rows, radiance_fp ) )
	    < 0 ) {
	fprintf ( stderr,
		"DEVA_XYZ_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( scanline_ordering != PIXSTANDARD ) {
	fprintf ( stderr,
	    "DEVA_XYZ_from_radfile: non-standard scanline ordering!\n" );
	exit ( EXIT_FAILURE );
    }

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_XYZ_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    XYZ = DEVA_XYZ_image_new ( n_rows, n_cols );
    DEVA_image_view ( XYZ ) = view;
    DEVA_image_description ( XYZ ) = old_header;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_XYZ_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( XYZ_rad_pixel, rgb2xyzmat,
			radiance_scanline[col] );
		DEVA_image_data ( XYZ, row, col ) . X =
		    colval ( XYZ_rad_pixel, CIEX ) * DEVA_WHTEFFICACY;
		DEVA_image_data ( XYZ, row, col ) . Y =
		    colval ( XYZ_rad_pixel, CIEY ) * DEVA_WHTEFFICACY;
		DEVA_image_data ( XYZ, row, col ) . Z =
		    colval ( XYZ_rad_pixel, CIEZ ) * DEVA_WHTEFFICACY;
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( XYZ, row, col ) . X =
		    colval ( radiance_scanline[col], CIEX );
		DEVA_image_data ( XYZ, row, col ) . Y =
		    colval ( radiance_scanline[col], CIEY );
		DEVA_image_data ( XYZ, row, col ) . Z =
		    colval ( radiance_scanline[col], CIEZ );
	    }
	} else {
	    fprintf ( stderr,
		    "DEVA_XYZ_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( XYZ );
}

void
DEVA_XYZ_to_radfilename ( char *filename, DEVA_XYZ_image *XYZ )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    DEVA_XYZ_to_radfile ( radiance_fp, XYZ );

    fclose ( radiance_fp );
}

void
DEVA_XYZ_to_radfile ( FILE *radiance_fp, DEVA_XYZ_image *XYZ )
/*
 * For now, only write rgbe format files.
 */
{
    int	   n_rows, n_cols;
    int	   row, col;
    COLOR  *radiance_scanline;
    COLOR  XYZ_rad_pixel;

    n_rows = DEVA_image_n_rows ( XYZ );
    n_cols = DEVA_image_n_cols ( XYZ );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    newheader ( "RADIANCE", radiance_fp );

    if ( ( DEVA_image_description ( XYZ ) != NULL ) &&
	    ( strlen ( DEVA_image_description ( XYZ ) ) >= 1 ) ) {
	/* output history */
	fputs ( DEVA_image_description ( XYZ ), radiance_fp );

	/* make sure history ends with a newline */
	if ( DEVA_image_description ( XYZ )
		[strlen ( DEVA_image_description ( XYZ ) ) - 1] != '\n' ) {
	    fputc ('\n', radiance_fp );
	}
    }

    if ( DEVA_image_view ( XYZ ) . type != '\0' ) {
	/* output view string if non-null */
	fputs ( VIEWSTR, radiance_fp );
	fprintview ( &DEVA_image_view ( XYZ ), radiance_fp );
	putc ( '\n', radiance_fp );
    }

    /* only know how to write rgbe */
    fputformat ( COLRFMT, radiance_fp );

    fputc ('\n', radiance_fp );

    fprtresolu ( n_cols, n_rows, radiance_fp );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_XYZ_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    colval ( XYZ_rad_pixel, CIEX ) =
		DEVA_image_data ( XYZ, row, col ) . X / DEVA_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEY ) =
		DEVA_image_data ( XYZ, row, col ) . Y / DEVA_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEZ ) =
		DEVA_image_data ( XYZ, row, col ) . Z / DEVA_WHTEFFICACY;

	    colortrans ( radiance_scanline[col], xyz2rgbmat, XYZ_rad_pixel );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_XYZ_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DEVA_xyY_image *
DEVA_xyY_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory xyY image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DEVA_xyY_image	*xyY;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    xyY = DEVA_xyY_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( xyY );
}

DEVA_xyY_image *
DEVA_xyY_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory xyY image.
 */
{
    DEVA_xyY_image	*xyY;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    DEVA_XYZ		XYZ_DEVA_pixel;
    int			row, col;
    int			n_rows, n_cols;
    int			scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    if ( old_header != NULL ) {
	free ( old_header );
    }
    old_header = NULL;
    view = nullview;	/* in case no VIEW records */

    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_xyY_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( ( scanline_ordering = fgetresolu ( &n_cols, &n_rows, radiance_fp ) )
	    < 0 ) {
	fprintf ( stderr,
		"DEVA_xyY_from_radfile: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( scanline_ordering != PIXSTANDARD ) {
	fprintf ( stderr,
	    "DEVA_xyY_from_radfile: non-standard scanline ordering!\n" );
	exit ( EXIT_FAILURE );
    }

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_xyY_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    xyY = DEVA_xyY_image_new ( n_rows, n_cols );
    DEVA_image_view ( xyY ) = view;
    DEVA_image_description ( xyY ) = old_header;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_xyY_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( XYZ_rad_pixel, rgb2xyzmat,
			radiance_scanline[col] );
		XYZ_DEVA_pixel.X =
		    colval ( XYZ_rad_pixel, CIEX ) * DEVA_WHTEFFICACY;
		XYZ_DEVA_pixel.Y =
		    colval ( XYZ_rad_pixel, CIEY ) * DEVA_WHTEFFICACY;
		XYZ_DEVA_pixel.Z =
		    colval ( XYZ_rad_pixel, CIEZ ) * DEVA_WHTEFFICACY;

		DEVA_image_data ( xyY, row, col ) =
				DEVA_XYZ2xyY ( XYZ_DEVA_pixel );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		XYZ_DEVA_pixel.X = colval ( radiance_scanline[col], CIEX );
		XYZ_DEVA_pixel.Y = colval ( radiance_scanline[col], CIEY );
		XYZ_DEVA_pixel.Z = colval ( radiance_scanline[col], CIEZ );

		DEVA_image_data ( xyY, row, col ) =
		    		DEVA_XYZ2xyY ( XYZ_DEVA_pixel );
	    }
	} else {
	    fprintf ( stderr,
		    "DEVA_xyY_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( xyY );
}

void
DEVA_xyY_to_radfilename ( char *filename, DEVA_xyY_image *xyY )
{
    FILE    *radiance_fp;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdout;
    } else {
	radiance_fp = fopen ( filename, "w" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    DEVA_xyY_to_radfile ( radiance_fp, xyY );

    fclose ( radiance_fp );
}

void
DEVA_xyY_to_radfile ( FILE *radiance_fp, DEVA_xyY_image *xyY )
/*
 * For now, only write rgbe format files.
 */
{
    int		n_rows, n_cols;
    int		row, col;
    DEVA_XYZ	XYZ_DEVA_pixel;
    COLOR	XYZ_rad_pixel;
    COLOR	*radiance_scanline;

    n_rows = DEVA_image_n_rows ( xyY );
    n_cols = DEVA_image_n_cols ( xyY );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    newheader ( "RADIANCE", radiance_fp );

    if ( ( DEVA_image_description ( xyY ) != NULL ) &&
	    ( strlen ( DEVA_image_description ( xyY ) ) >= 1 ) ) {
	/* output history */
	fputs ( DEVA_image_description ( xyY ), radiance_fp );

	/* make sure history ends with a newline */
	if ( DEVA_image_description ( xyY )
		[strlen ( DEVA_image_description ( xyY ) ) - 1] != '\n' ) {
	    fputc ('\n', radiance_fp );
	}
    }

    if ( DEVA_image_view ( xyY ) . type != '\0' ) {
	/* output view string if non-null */
	fputs ( VIEWSTR, radiance_fp );
	fprintview ( &DEVA_image_view ( xyY ), radiance_fp );
	putc ( '\n', radiance_fp );
    }

    /* only know how to write rgbe */
    fputformat ( COLRFMT, radiance_fp );

    fputc ('\n', radiance_fp );

    fprtresolu ( n_cols, n_rows, radiance_fp );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_xyY_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    XYZ_DEVA_pixel = DEVA_xyY2XYZ ( DEVA_image_data ( xyY, row,col ) );
	    colval ( XYZ_rad_pixel, CIEX ) =
		XYZ_DEVA_pixel.X / DEVA_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEY ) =
		XYZ_DEVA_pixel.Y / DEVA_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEZ ) =
		XYZ_DEVA_pixel.Z / DEVA_WHTEFFICACY;

	    colortrans ( radiance_scanline[col], xyz2rgbmat, XYZ_rad_pixel );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_xyY_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

static int
headline ( char *s, void *p )
{
    char	fmt[LPICFMT+1];
    static int	header_line_number = 0;

    if ( header_line_number == 0 ) {
	if ( strncmp ( s, "#?RADIANCE", strlen ( "#?RADIANCE" ) ) != 0 ) {
	    return ( -1 );
	} else {
	    header_line_number++;
	    return ( 1 );
	}
    }

    if ( formatval ( fmt, s) ) {
	if ( color_format != unknown ) {
	    fprintf ( stderr,
		  "DEVA_brightness_from_radfile: multiple format records!\n" );
	    exit ( EXIT_FAILURE );
	} else if ( strcmp ( fmt, COLRFMT) == 0 ) {
	    color_format = rgbe;
	} else if ( strcmp( fmt, CIEFMT ) == 0 ) {
	    color_format = xyze;
	} else {
	    fprintf ( stderr,
		    "DEVA_brightness_from_radfile: unrecognized format!\n" );
	    exit ( EXIT_FAILURE );
	}

	/* regenerate FORMAT for output, so don't save here */
	header_line_number++;
	return ( 1 );
    }

    /* if ( isview ( s ) ) {  use *only* VIEW= lines */
    if ( strncmp ( s, "VIEW=", strlen ( "VIEW=" ) ) == 0 ) {
	sscanview ( &view, s );
    } else {	/* don't include old VIEW record in header */
	old_header = strcat_safe ( old_header, s );
    }

    header_line_number++;
    return ( 1 );
}
