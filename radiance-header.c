/*
 * Routines for reading and writing RADIANCE file headers.
 *
 * Requires the following RADIANCE routines:
 * color.c, header.c, fputword.c, resolu.c, image.c, fvect.c, badarg.c,
 * words.c, spec_rgb.c.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "radiance-header.h"
#include "radiance/color.h"
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "deva-license.h"	/* DEVA open source license */

#define NULLVIEW	{'\0',{0.,0.,0.},{0.,0.,0.},{0.,0.,0.}, \
				0.,0.,0.,0.,0.,0.,0., \
				{0.,0.,0.},{0.,0.,0.},0.,0.}

VIEW DEVA_null_view = NULLVIEW;	/* available to calling programs */

/* need to be global because of the way Radiance reads header lines */
char	    		    *progname = "radianceIO";
/* radiance/image.c needs this */
static int		    header_line_number;
static RadianceColorFormat  color_format;
static VIEW		    view;
static char		    *header_text;
static int		    exposure_set;	/* TRUE is exposure set */
						/* in file (needed because */
						/* missing EXPOSURE value */
						/* may have different */
						/* implications than */
						/* EXPOSURE = 1.0 */
static double		    exposure;		/* actual exposure value */
						/* if set in file or 1.0 */
						/* otherwise */

static void	initialize_headline ( void );
static int	headline ( char *s, void *p );
static char	*strcat_safe ( char *dest, char *src );

void
DEVA_read_radiance_header ( FILE *radiance_fp, int *n_rows_p, int *n_cols_p,
	RadianceColorFormat *color_format_p, VIEW *view_p, int *exposure_set_p,
	double *exposure_p, char **header_text_p )
/*
 * Read Radiance image file header and return potentially relevant information.
 * Leaves file at the start of the first scan line.
 *
 * All detected errors are fatal.
 */
{
    int	    n_rows, n_cols;
    int	    scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    initialize_headline ( );
    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_read_radiance_header: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( ( scanline_ordering =
		fgetresolu ( &n_cols, &n_rows, radiance_fp ) ) < 0 ) {
	fprintf ( stderr,
		"DEVA_read_radiance_header: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( scanline_ordering != PIXSTANDARD ) {
	fprintf ( stderr,
	    "DEVA_read_radiance_header: non-standard scanline ordering!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( n_rows_p != NULL ) {
	*n_rows_p = n_rows;
    }
    if ( n_cols_p != NULL ) {
	*n_cols_p = n_cols;
    }

    if ( color_format_p != NULL ) {
	*color_format_p = color_format;
    }

    if ( view_p != NULL ) {
	*view_p = view;
    }

    if ( exposure_set_p != NULL ) {
	*exposure_set_p = exposure_set;
    }

    if ( exposure_p != NULL ) {
	*exposure_p = exposure;
    }

    if ( header_text_p != NULL ) {
	*header_text_p = header_text;
    }
}

static void
initialize_headline ( void )
/*
 * Headline returns all relevant information through global variables,
 * some of which have values that depend on multiple header lines.  This
 * routine initializes all of the globals before the start of reading the
 * header.
 *
 * 	header_line_number
 * 	color_format
 * 	view
 * 	header_text
 * 	exposure_set
 * 	exposure
 */
{
    header_line_number = 0;
    color_format = radcolor_unknown;
    view = DEVA_null_view;
    if ( header_text != NULL ) {
	free ( header_text );
    }
    header_text = NULL;
    exposure_set = FALSE;
    exposure = 1.0;
}

static int
headline ( char *s, void *p )
/*
 * Called for each line of the Radiance header.
 * All relevant information retrieved from the header is returned via
 * global variables, all of which have to be correctly initialized before
 * the first call!
 *
 * 	header_line_number
 * 	color_format
 * 	view
 * 	header_text
 * 	exposure
 */
{
    char	fmt[LPICFMT+1];

    if ( header_line_number == 0 ) {
	if ( strncmp ( s, "#?RADIANCE", strlen ( "#?RADIANCE" ) ) != 0 ) {
	    return ( -1 );
	} else {
	    header_line_number++;
	    return ( 1 );
	}
    }

    if ( formatval ( fmt, s) ) {
	if ( color_format != radcolor_unknown ) {
	    fprintf ( stderr,
		    "DEVA_read_radiance_header: multiple format records!\n" );
	    exit ( EXIT_FAILURE );
	} else if ( strcmp ( fmt, COLRFMT) == 0 ) {
	    color_format = radcolor_rgbe;
	} else if ( strcmp( fmt, CIEFMT ) == 0 ) {
	    color_format = radcolor_xyze;
	} else {
	    fprintf ( stderr,
		    "DEVA_read_radiance_header: unrecognized format!\n" );
	    exit ( EXIT_FAILURE );
	}

	/* regenerate FORMAT for output, so don't save here */
	header_line_number++;
	return ( 1 );
    }

    /* if ( isview ( s ) )   use *only* VIEW= lines */
    if ( strncmp ( s, "VIEW=", strlen ( "VIEW=" ) ) == 0 ) {
	sscanview ( &view, s );
    } else if ( isexpos ( s ) ) {
	exposure_set = TRUE;
	exposure *= exposval ( s );
    } else {
	header_text = strcat_safe ( header_text, s );
    }

    header_line_number++;

    return ( 1 );
}

static char *
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
	    fprintf ( stderr, "strcat_safe: malloc failed!\n" );
	    exit ( EXIT_FAILURE );
	}
    }

    return ( returned_string );
}

void
DEVA_write_radiance_header ( FILE *radiance_fp, int n_rows, int n_cols,
	RadianceColorFormat color_format, VIEW view, int exposure_set,
	double exposure, char *other_info )
{
    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    newheader ( "RADIANCE", radiance_fp );

    if ( other_info != NULL ) {
	fputs ( other_info, radiance_fp );
	if ( ( strlen ( other_info ) >= 1 ) &&
		( other_info[strlen(other_info)-1] != '\n' ) ) {
	    /* make sure header string ends with * a newline */
	    fputc ('\n', radiance_fp );
	}
    }

    if ( exposure_set ) {
	fprintf ( radiance_fp, "%s%f\n", EXPOSSTR, exposure );
    }

    if ( view.type != 0 ) {
	fputs ( VIEWSTR, radiance_fp );
	fprintview ( &view, radiance_fp );
	putc ( '\n', radiance_fp );
    }

    switch ( color_format ) {
	case radcolor_rgbe:
	    fputformat ( COLRFMT, radiance_fp );
	    break;

	case radcolor_xyze:
	    fputformat ( CIEFMT, radiance_fp );
	    break;

	default:
	    fprintf ( stderr,
		    "DEVA_write_radiance_header: unknown format code!\n" );
	    break;
    }

    fputc ( '\n', radiance_fp );
    fprtresolu ( n_cols, n_rows, radiance_fp );
}
