#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "devas-image.h"
#include "radianceIO.h"
#include "radiance-header.h"
#include "radiance/color.h"
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "devas-license.h"	/* DeVAS open source license */

DeVAS_float_image *
DeVAS_brightness_image_from_radfilename ( char *filename  )
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
    DeVAS_float_image	*brightness;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    brightness = DeVAS_brightness_image_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( brightness );
}

DeVAS_float_image *
DeVAS_brightness_image_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory brightness image.  Units of returned image are as for
 * rgbe-format Radiance files (watts/steradian/sq.meter over the visible
 * spectrum).
 */
{
    DeVAS_float_image	*brightness;
    COLOR		*radiance_scanline;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DeVAS_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr,
		"DeVAS_brightness_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    brightness = DeVAS_float_image_new ( n_rows, n_cols );
    DeVAS_image_view ( brightness ) = view;
    DeVAS_image_description ( brightness ) = description;
    DeVAS_image_exposure_set ( brightness ) = exposure_set;
    DeVAS_image_exposure ( brightness ) = exposure;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
	  "DeVAS_brightness_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( brightness, row, col ) =
		    bright ( radiance_scanline[col] );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( brightness, row, col ) =
		    colval ( radiance_scanline[col], CIEY ) / DeVAS_WHTEFFICACY;
	    }
	} else {
	    fprintf ( stderr,
		    "DeVAS_brightness_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( brightness );
}

void
DeVAS_brightness_image_to_radfilename ( char *filename,
	DeVAS_float_image *brightness )
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

    DeVAS_brightness_image_to_radfile ( radiance_fp, brightness );

    fclose ( radiance_fp );
}

void
DeVAS_brightness_image_to_radfile ( FILE *radiance_fp,
	DeVAS_float_image *brightness )
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
    int			exposure_set;
    double		exposure;
    char		*description;
    COLOR		*radiance_scanline;

    n_rows = DeVAS_image_n_rows ( brightness );
    n_cols = DeVAS_image_n_cols ( brightness );

    view = DeVAS_image_view ( brightness );

    exposure_set = DeVAS_image_exposure_set ( brightness );
    exposure = DeVAS_image_exposure ( brightness );

    description = DeVAS_image_description ( brightness );

    color_format = radcolor_rgbe;

    DeVAS_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, exposure_set, exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr,
		"DeVAS_brightness_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],	/* output is grayscale */
		    DeVAS_image_data ( brightness, row, col ),
		    DeVAS_image_data ( brightness, row, col ),
		    DeVAS_image_data ( brightness, row, col ) );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
	  "DeVAS_brightness_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DeVAS_float_image *
DeVAS_luminance_image_from_radfilename ( char *filename  )
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
    DeVAS_float_image	*luminance;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    luminance = DeVAS_luminance_image_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( luminance );
}

DeVAS_float_image *
DeVAS_luminance_image_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory luminance image.  Units of returned image are
 * (candela/m^2).  Note that this is not the unit used in rgbe-format
 * Radiance files.
 */
{
    DeVAS_float_image	*luminance;	/* note name confilict with RADIANCE */
    					/* file color.h */
    COLOR		*radiance_scanline;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DeVAS_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr,
		"DeVAS_luminance_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    luminance = DeVAS_float_image_new ( n_rows, n_cols );
    DeVAS_image_view ( luminance ) = view;
    DeVAS_image_description ( luminance ) = description;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
	  "DeVAS_luminance_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( luminance, row, col ) = exposure *
			luminance ( radiance_scanline[col] );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( luminance, row, col ) = exposure *
		    	colval ( radiance_scanline[col], CIEY );
	    }
	} else {
	    fprintf ( stderr,
		    "DeVAS_luminance_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( luminance );
}

void
DeVAS_luminance_image_to_radfilename ( char *filename,
	DeVAS_float_image *luminance )
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

    DeVAS_luminance_image_to_radfile ( radiance_fp, luminance );

    fclose ( radiance_fp );
}

void
DeVAS_luminance_image_to_radfile ( FILE *radiance_fp,
	DeVAS_float_image *luminance )
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
    int			exposure_set;
    double		exposure;
    char		*description;
    COLOR		*radiance_scanline;

    n_rows = DeVAS_image_n_rows ( luminance );
    n_cols = DeVAS_image_n_cols ( luminance );

    exposure_set = DeVAS_image_exposure_set ( luminance );
    exposure = DeVAS_image_exposure ( luminance );

    view = DeVAS_image_view ( luminance );

    description = DeVAS_image_description ( luminance );

    color_format = radcolor_rgbe;

    DeVAS_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, exposure_set, exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_luminance_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],	/* output is grayscale */
		DeVAS_image_data ( luminance, row, col ) / DeVAS_WHTEFFICACY,
		DeVAS_image_data ( luminance, row, col ) / DeVAS_WHTEFFICACY,
		DeVAS_image_data ( luminance, row, col ) / DeVAS_WHTEFFICACY );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
	  "DeVAS_luminance_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DeVAS_RGBf_image *
DeVAS_RGBf_image_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory RGBf image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DeVAS_RGBf_image	*RGBf;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    RGBf = DeVAS_RGBf_image_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( RGBf );
}

DeVAS_RGBf_image *
DeVAS_RGBf_image_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory RGBf image.
 */
{
    DeVAS_RGBf_image	*RGBf;
    COLOR		*radiance_scanline;
    COLOR		RGBf_rad_pixel;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DeVAS_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_RGBf_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    RGBf = DeVAS_RGBf_image_new ( n_rows, n_cols );
    DeVAS_image_view ( RGBf ) = view;
    DeVAS_image_description ( RGBf ) = description;
    DeVAS_image_exposure_set ( RGBf ) = exposure_set;
    DeVAS_image_exposure ( RGBf ) = exposure;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DeVAS_RGBf_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( RGBf, row, col ) . red =
		    		colval ( radiance_scanline[col], RED );
		DeVAS_image_data ( RGBf, row, col ) . green =
		    		colval ( radiance_scanline[col], GRN );
		DeVAS_image_data ( RGBf, row, col ) . blue =
		    		colval ( radiance_scanline[col], BLU );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( RGBf_rad_pixel, xyz2rgbmat,
			radiance_scanline[col] );
		DeVAS_image_data ( RGBf, row, col ) . red =
		    colval ( RGBf_rad_pixel, RED ) / DeVAS_WHTEFFICACY;
		DeVAS_image_data ( RGBf, row, col ) . green =
		    colval ( RGBf_rad_pixel, GRN ) / DeVAS_WHTEFFICACY;
		DeVAS_image_data ( RGBf, row, col ) . blue =
		    colval ( RGBf_rad_pixel, BLU ) / DeVAS_WHTEFFICACY;
	    }
	} else {
	    fprintf ( stderr,
		    "DeVAS_RGBf_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( RGBf );
}

void
DeVAS_RGBf_image_to_radfilename ( char *filename, DeVAS_RGBf_image *RGBf )
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

    DeVAS_RGBf_image_to_radfile ( radiance_fp, RGBf );

    fclose ( radiance_fp );
}

void
DeVAS_RGBf_image_to_radfile ( FILE *radiance_fp, DeVAS_RGBf_image *RGBf )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    int			exposure_set;
    double		exposure;
    char		*description;
    COLOR		*radiance_scanline;

    n_rows = DeVAS_image_n_rows ( RGBf );
    n_cols = DeVAS_image_n_cols ( RGBf );

    view = DeVAS_image_view ( RGBf );

    exposure_set = DeVAS_image_exposure_set ( RGBf );
    exposure = DeVAS_image_exposure ( RGBf );

    description = DeVAS_image_description ( RGBf );

    color_format = radcolor_rgbe;

    DeVAS_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, exposure_set, exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_RGBf_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    setcolor ( radiance_scanline[col],
		    DeVAS_image_data ( RGBf, row, col ) . red,
		    DeVAS_image_data ( RGBf, row, col ) . green,
		    DeVAS_image_data ( RGBf, row, col ) . blue );
	}
	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DeVAS_RGBf_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DeVAS_XYZ_image *
DeVAS_XYZ_image_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory XYZ image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DeVAS_XYZ_image	*XYZ;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    XYZ = DeVAS_XYZ_image_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( XYZ );
}

DeVAS_XYZ_image *
DeVAS_XYZ_image_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory XYZ image.
 */
{
    DeVAS_XYZ_image	*XYZ;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DeVAS_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_XYZ_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    XYZ = DeVAS_XYZ_image_new ( n_rows, n_cols );
    DeVAS_image_view ( XYZ ) = view;
    DeVAS_image_description ( XYZ ) = description;
    DeVAS_image_exposure_set ( XYZ ) = exposure_set;
    DeVAS_image_exposure ( XYZ ) = exposure;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DeVAS_XYZ_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( XYZ_rad_pixel, rgb2xyzmat,
			radiance_scanline[col] );
		DeVAS_image_data ( XYZ, row, col ) . X =
		    colval ( XYZ_rad_pixel, CIEX ) * DeVAS_WHTEFFICACY;
		DeVAS_image_data ( XYZ, row, col ) . Y =
		    colval ( XYZ_rad_pixel, CIEY ) * DeVAS_WHTEFFICACY;
		DeVAS_image_data ( XYZ, row, col ) . Z =
		    colval ( XYZ_rad_pixel, CIEZ ) * DeVAS_WHTEFFICACY;
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		DeVAS_image_data ( XYZ, row, col ) . X =
		    colval ( radiance_scanline[col], CIEX );
		DeVAS_image_data ( XYZ, row, col ) . Y =
		    colval ( radiance_scanline[col], CIEY );
		DeVAS_image_data ( XYZ, row, col ) . Z =
		    colval ( radiance_scanline[col], CIEZ );
	    }
	} else {
	    fprintf ( stderr,
		    "DeVAS_XYZ_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( XYZ );
}

void
DeVAS_XYZ_image_to_radfilename ( char *filename, DeVAS_XYZ_image *XYZ )
/*
 * For now, only write rgbe format files.
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

    DeVAS_XYZ_image_to_radfile ( radiance_fp, XYZ );

    fclose ( radiance_fp );
}

void
DeVAS_XYZ_image_to_radfile ( FILE *radiance_fp, DeVAS_XYZ_image *XYZ )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    int			exposure_set;
    double		exposure;
    char		*description;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;

    n_rows = DeVAS_image_n_rows ( XYZ );
    n_cols = DeVAS_image_n_cols ( XYZ );

    view = DeVAS_image_view ( XYZ );

    exposure_set = DeVAS_image_exposure_set ( XYZ );
    exposure = DeVAS_image_exposure ( XYZ );

    description = DeVAS_image_description ( XYZ );

    color_format = radcolor_rgbe;

    DeVAS_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, exposure_set, exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_XYZ_image_to_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    colval ( XYZ_rad_pixel, CIEX ) =
		DeVAS_image_data ( XYZ, row, col ) . X / DeVAS_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEY ) =
		DeVAS_image_data ( XYZ, row, col ) . Y / DeVAS_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEZ ) =
		DeVAS_image_data ( XYZ, row, col ) . Z / DeVAS_WHTEFFICACY;

	    colortrans ( radiance_scanline[col], xyz2rgbmat, XYZ_rad_pixel );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DeVAS_XYZ_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}

DeVAS_xyY_image *
DeVAS_xyY_image_from_radfilename ( char *filename  )
/*
 * Reads Radiance rgbe or xyze file specified by pathname and returns
 * an in-memory xyY image.  A pathname of "-" specifies standard input.
 */
{
    FILE		*radiance_fp;
    DeVAS_xyY_image	*xyY;

    if ( strcmp ( filename, "-" ) == 0 ) {
	radiance_fp = stdin;
    } else {
	radiance_fp = fopen ( filename, "r" );
	if ( radiance_fp == NULL ) {
	    perror ( filename );
	    exit ( EXIT_FAILURE );
	}
    }

    xyY = DeVAS_xyY_image_from_radfile ( radiance_fp );
    fclose ( radiance_fp );

    return ( xyY );
}

DeVAS_xyY_image *
DeVAS_xyY_image_from_radfile ( FILE *radiance_fp )
/*
 * Reads Radiance rgbe or xyze file from an open file descriptor and returns
 * an in-memory xyY image.
 */
{
    DeVAS_xyY_image	*xyY;
    COLOR		*radiance_scanline;
    COLOR		XYZ_rad_pixel;
    DeVAS_XYZ		XYZ_DeVAS_pixel;
    RadianceColorFormat	color_format;
    VIEW		view;
    int			exposure_set;
    double		exposure;
    int			row, col;
    int			n_rows, n_cols;
    char		*description;

    DeVAS_read_radiance_header ( radiance_fp, &n_rows, &n_cols,
	    &color_format, &view, &exposure_set, &exposure, &description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_xyY_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    xyY = DeVAS_xyY_image_new ( n_rows, n_cols );
    DeVAS_image_view ( xyY ) = view;
    DeVAS_image_description ( xyY ) = description;
    DeVAS_image_exposure_set ( xyY ) = exposure_set;
    DeVAS_image_exposure ( xyY ) = exposure;

    for ( row = 0; row < n_rows; row++ ) {
	if ( freadscan( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DeVAS_xyY_image_from_radfile: error reading Radiance file!" );
	    exit ( EXIT_FAILURE );
	}
	if ( color_format == radcolor_rgbe ) {
	    for ( col = 0; col < n_cols; col++ ) {
		colortrans ( XYZ_rad_pixel, rgb2xyzmat,
			radiance_scanline[col] );

		XYZ_DeVAS_pixel.X =
		    colval ( XYZ_rad_pixel, CIEX ) * DeVAS_WHTEFFICACY;
		XYZ_DeVAS_pixel.Y =
		    colval ( XYZ_rad_pixel, CIEY ) * DeVAS_WHTEFFICACY;
		XYZ_DeVAS_pixel.Z =
		    colval ( XYZ_rad_pixel, CIEZ ) * DeVAS_WHTEFFICACY;

		DeVAS_image_data ( xyY, row, col ) =
		    DeVAS_XYZ2xyY ( XYZ_DeVAS_pixel );
	    }
	} else if ( color_format == radcolor_xyze ) {
	    for ( col = 0; col < n_cols; col++ ) {
		XYZ_DeVAS_pixel.X =
		    colval ( radiance_scanline[col], CIEX );
		XYZ_DeVAS_pixel.Y =
		    colval ( radiance_scanline[col], CIEY );
		XYZ_DeVAS_pixel.Z =
		    colval ( radiance_scanline[col], CIEZ );

		DeVAS_image_data ( xyY, row, col ) =
		    DeVAS_XYZ2xyY ( XYZ_DeVAS_pixel );
	    }
	} else {
	    fprintf ( stderr,
		    "DeVAS_XYZ_image_from_radfile: internal error!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );

    return ( xyY );
}

void
DeVAS_xyY_image_to_radfilename ( char *filename, DeVAS_xyY_image *xyY )
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

    DeVAS_xyY_image_to_radfile ( radiance_fp, xyY );

    fclose ( radiance_fp );
}

void
DeVAS_xyY_image_to_radfile ( FILE *radiance_fp, DeVAS_xyY_image *xyY )
/*
 * For now, only write rgbe format files.
 */
{
    int			n_rows, n_cols;
    int			row, col;
    VIEW		view;
    RadianceColorFormat	color_format;
    int			exposure_set;
    double		exposure;
    char		*description;
    DeVAS_XYZ		XYZ_DeVAS_pixel;
    COLOR		XYZ_rad_pixel;
    COLOR		*radiance_scanline;

    n_rows = DeVAS_image_n_rows ( xyY );
    n_cols = DeVAS_image_n_cols ( xyY );

    view = DeVAS_image_view ( xyY );

    exposure_set = DeVAS_image_exposure_set ( xyY );
    exposure = DeVAS_image_exposure ( xyY );

    description = DeVAS_image_description ( xyY );

    color_format = radcolor_rgbe;

    DeVAS_write_radiance_header ( radiance_fp, n_rows, n_cols, color_format,
	    view, exposure_set, exposure, description );

    radiance_scanline = (COLOR *) malloc ( n_cols * sizeof ( COLOR ) );
    if ( radiance_scanline == NULL ) {
	fprintf ( stderr, "DeVAS_xyY_image_from_radfile: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    XYZ_DeVAS_pixel = DeVAS_xyY2XYZ ( DeVAS_image_data ( xyY, row,col ) );
	    colval ( XYZ_rad_pixel, CIEX ) =
		XYZ_DeVAS_pixel.X / DeVAS_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEY ) =
		XYZ_DeVAS_pixel.Y / DeVAS_WHTEFFICACY;
	    colval ( XYZ_rad_pixel, CIEZ ) =
		XYZ_DeVAS_pixel.Z / DeVAS_WHTEFFICACY;

	    colortrans ( radiance_scanline[col], xyz2rgbmat, XYZ_rad_pixel );
	}

	if ( fwritescan ( radiance_scanline, n_cols, radiance_fp ) < 0 ) {
	    fprintf ( stderr,
		"DeVAS_xyY_image_to_radfile: error writing radiance file!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    free ( radiance_scanline );
}
