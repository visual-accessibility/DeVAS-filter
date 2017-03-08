#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "deva-image.h"
#include "deva-utils.h"
#include "radianceIO.h"
#include "radiance-header.h"
#include "radiance/color.h"
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "deva-license.h"	/* DEVA open source license */

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
    RadianceColorFormat	color_format;
    VIEW		view;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	            &color_format, &view, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_brightness_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    brightness = DEVA_float_image_new ( n_rows, n_cols );
    DEVA_image_view ( brightness ) = view;
    DEVA_image_description ( brightness ) = description;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_brightness_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( brightness, row, col ) = exposure *
					bright ( radiance_scanline[col] );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( brightness, row, col ) = exposure *
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
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;

    n_rows = DEVA_image_n_rows ( brightness );
    n_cols = DEVA_image_n_cols ( brightness );

    view = DEVA_image_view ( brightness );

    description = DEVA_image_description ( brightness );

    color_format = rgbe;

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, -1.0, description );

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
    RadianceColorFormat	color_format;
    VIEW		view;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_luminance_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    luminance = DEVA_float_image_new ( n_rows, n_cols );
    DEVA_image_view ( luminance ) = view;
    DEVA_image_description ( luminance ) = description;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_luminance_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( luminance, row, col ) = exposure *
			luminance ( radiance_scanline[col] );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( luminance, row, col ) = exposure *
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
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat color_format;
    char		*description;
    COLOR		*radiance_scanline;

    n_rows = DEVA_image_n_rows ( luminance );
    n_cols = DEVA_image_n_cols ( luminance );

    view = DEVA_image_view ( luminance );

    description = DEVA_image_description ( luminance );

    color_format = rgbe;

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, -1.0, description );

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
    RadianceColorFormat	color_format;
    VIEW		view;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_RGBf_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    RGBf = DEVA_RGBf_image_new ( n_rows, n_cols );
    DEVA_image_view ( RGBf ) = view;
    DEVA_image_description ( RGBf ) = description;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DEVA_RGBf_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( RGBf, row, col ) . red = exposure *
		    		colval ( radiance_scanline[col], RED );
		DEVA_image_data ( RGBf, row, col ) . green = exposure *
		    		colval ( radiance_scanline[col], GRN );
		DEVA_image_data ( RGBf, row, col ) . blue = exposure *
		    		colval ( radiance_scanline[col], BLU );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( RGBf_rad_pixel, xyz2rgbmat,
			radiance_scanline[col] );
		DEVA_image_data ( RGBf, row, col ) . red = exposure *
		    colval ( RGBf_rad_pixel, RED ) / DEVA_WHTEFFICACY;
		DEVA_image_data ( RGBf, row, col ) . green = exposure *
		    colval ( RGBf_rad_pixel, GRN ) / DEVA_WHTEFFICACY;
		DEVA_image_data ( RGBf, row, col ) . blue = exposure *
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
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;

    n_rows = DEVA_image_n_rows ( RGBf );
    n_cols = DEVA_image_n_cols ( RGBf );

    view = DEVA_image_view ( RGBf );

    description = DEVA_image_description ( RGBf );

    color_format = rgbe;

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, -1.0, description );

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
    RadianceColorFormat	color_format;
    VIEW		view;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure, &description );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_XYZ_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    XYZ = DEVA_XYZ_image_new ( n_rows, n_cols );
    DEVA_image_view ( XYZ ) = view;
    DEVA_image_description ( XYZ ) = description;

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
		DEVA_image_data ( XYZ, row, col ) . X = exposure *
		    colval ( XYZ_rad_pixel, CIEX ) * DEVA_WHTEFFICACY;
		DEVA_image_data ( XYZ, row, col ) . Y = exposure *
		    colval ( XYZ_rad_pixel, CIEY ) * DEVA_WHTEFFICACY;
		DEVA_image_data ( XYZ, row, col ) . Z = exposure *
		    colval ( XYZ_rad_pixel, CIEZ ) * DEVA_WHTEFFICACY;
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DEVA_image_data ( XYZ, row, col ) . X = exposure *
		    colval ( radiance_scanline[col], CIEX );
		DEVA_image_data ( XYZ, row, col ) . Y = exposure *
		    colval ( radiance_scanline[col], CIEY );
		DEVA_image_data ( XYZ, row, col ) . Z = exposure *
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
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    char		*description;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;

    n_rows = DEVA_image_n_rows ( XYZ );
    n_cols = DEVA_image_n_cols ( XYZ );

    view = DEVA_image_view ( XYZ );

    description = DEVA_image_description ( XYZ );

    color_format = rgbe;

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, -1.0, description );

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
    RadianceColorFormat	color_format;
    VIEW		view;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DEVA_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DEVA_xyY_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    xyY = DEVA_xyY_image_new ( n_rows, n_cols );
    DEVA_image_view ( xyY ) = view;
    DEVA_image_description ( xyY ) = description;

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

		XYZ_DEVA_pixel.X = exposure *
		    colval ( XYZ_rad_pixel, CIEX ) * DEVA_WHTEFFICACY;
		XYZ_DEVA_pixel.Y = exposure *
		    colval ( XYZ_rad_pixel, CIEY ) * DEVA_WHTEFFICACY;
		XYZ_DEVA_pixel.Z = exposure *
		    colval ( XYZ_rad_pixel, CIEZ ) * DEVA_WHTEFFICACY;

		DEVA_image_data ( xyY, row, col ) =
		    DEVA_XYZ2xyY ( XYZ_DEVA_pixel );
	    }
	} else if ( color_format == xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		XYZ_DEVA_pixel.X = exposure *
		    colval ( radiance_scanline[col], CIEX );
		XYZ_DEVA_pixel.Y = exposure *
		    colval ( radiance_scanline[col], CIEY );
		XYZ_DEVA_pixel.Z = exposure *
		    colval ( radiance_scanline[col], CIEZ );

		DEVA_image_data ( xyY, row, col ) =
		    DEVA_XYZ2xyY ( XYZ_DEVA_pixel );
	    }
	} else {
	    fprintf ( stderr,
		    "DEVA_XYZ_from_radfile: internal error!\n" );
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
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    char		*description;
    DEVA_XYZ		XYZ_DEVA_pixel;
    COLOR		XYZ_rad_pixel;
    COLOR		*radiance_scanline;

    n_rows = DEVA_image_n_rows ( xyY );
    n_cols = DEVA_image_n_cols ( xyY );

    view = DEVA_image_view ( xyY );

    description = DEVA_image_description ( xyY );

    color_format = rgbe;

    DEVA_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, -1.0, description );

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
