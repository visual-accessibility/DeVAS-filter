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
#include <ctype.h>
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
static int		    header_line_number;
static RadianceColorFormat  color_format;
static VIEW		    view;
static char		    *header_text;
static int		    exposure_set;	/* TRUE if exposure set */
						/* in file (needed because */
						/* missing EXPOSURE value */
						/* may have different */
						/* implications than */
						/* EXPOSURE = 1.0) */
static double		    exposure;		/* actual exposure value */
						/* if set in file or 1.0 */
						/* otherwise */
static int		    view_set;		/* help in dealing with */
static int		    indented_view_set;	/* pcomp generated files */
static VIEW		    indented_view;	/* which can have multiple */
						/* VIEW records, some or all */
						/* of which are indented */

static void	initialize_headline ( void );
static int	headline ( char *s, void *p );
static char	*strcat_safe ( char *dest, char *src );
#ifdef VIEW_COMP
static int	view_comp ( VIEW *v1,  VIEW *v2 );
#endif	/* VIEW_COMP */

void
DEVA_read_radiance_header ( FILE *radiance_fp, int *n_rows_p, int *n_cols_p,
	RadianceColorFormat *color_format_p, VIEW *view_p, int *exposure_set_p,
	double *exposure_p, char **header_text_p )
/*
 * Read Radiance image file header and return potentially relevant information.
 * Leaves file at the start of the first scan line.
 *
 * radiance_fp:		Open file descriptor for Radiance file.
 *
 * [All of the following are returned only if the arguement point is not NULL.]
 *
 * n_rows_p, n_cols_p:	Size of image data in file.
 *
 * color_format_p:	radcolor_unknown, radcolor_missing, radcolor_rgbe, or
 *			radcolor_xyze.
 *
 * view_p:		VIEW record.
 *
 * exposure_set_p:	TRUE if the file header had one or more EXPOSURE records
 *
 * exposure_p:		The combined exposure value.  Only valid if
 *			exposure_set_p is TRUE.
 *
 * header_text_p:	Header text of original file, except for EXPOSURE
 * 			and VIEW records.
 *
 * All detected errors are fatal.
 */
{
    int	    n_rows, n_cols;
    int	    scanline_ordering;

    SET_FILE_BINARY ( radiance_fp );	/* only affects Windows systems */

    /*
     * Initialize global veriables used to collect information from header.
     */
    initialize_headline ( );

    /*
     * getheader is a Radiance routine that calls headline for each line of
     * the header of the Radiance file.
     */
    if ( getheader ( radiance_fp, headline, NULL ) < 0 ) {
	fprintf ( stderr,
		"DEVA_read_radiance_header: invalid file header!\n" );
	exit ( EXIT_FAILURE );
    }

    /* get size and pixel ordering */
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

    /* return requested information */

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

	/*
	 * Deal with older versions of pcomb, which might have one or more
	 * indented VIEW records, but no non-indented VIEW records.
	 */
	if ( indented_view_set && !view_set ) {
	    printf ( "using indented VIEW record.\n" );
	    view = indented_view;
	}
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
    view_set = FALSE;
    indented_view = DEVA_null_view;
    indented_view_set = FALSE;
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
    char	*q;

    if ( header_line_number == 0 ) {
	if ( strncmp ( s, "#?RADIANCE", strlen ( "#?RADIANCE" ) ) != 0 ) {
	    return ( -1 );
	} else {
	    header_line_number++;
	    return ( 1 );
	}
    }

    if ( formatval ( fmt, s) ) {
	/* get pixel type (rgbe or xyze) */
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

    /*
     * Need to special case VIEW record processing for images produced using
     * pcomb, since these images only have the VIEW records for the source
     * images given to pcomb, and those are indented (with a leading '\t')
     * and so not recognized by isview() or sscanview().
     */
    q = s;
    while ( ( *q != '\0' ) && isblank ( *q ) ) {
	q++;	/* skip over leading blanks and tabs */
    }

    if ( strncmp ( s, "VIEW=", strlen ( "VIEW=" ) ) == 0 ) {
    		/* check for un-indented VIEW records */
		/* use only explicit VIEW records */
	    sscanview ( &view, s );
	    view_set = TRUE;
    } else if ( strncmp ( q, "VIEW=", strlen ( "VIEW=" ) ) == 0 ) {
    		/* check for indented VIEW records */
	if ( !indented_view_set ) {	/* use first if more than one */
	    sscanview ( &indented_view, q );
	    indented_view_set = TRUE;
	}
    } else if ( isexpos ( s ) ) {
		/* check for exposure records */
	exposure_set = TRUE;
	exposure *= exposval ( s );  /* allow for multiple exposure records */
    } else {
	/* save anything else for possible return to calling program */
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
/*
 * radiance_fp:		Open file descriptor for Radiance file.
 *
 * n_rows, n_cols:	Size of image data to be written.
 *
 * color_format:	radcolor_rgbe, or radcolor_xyze.
 *
 * view:		VIEW record to be written.
 *
 * exposure_set:	TRUE if the file header should have an EXPOSURE record.
 *
 * exposure:		The exposure value.  Only valid if exposure_set is TRUE.
 *
 * other_info:		Other header text to be written.
 */
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

#ifdef VIEW_COMP
static int
view_comp ( VIEW *v1,  VIEW *v2 )
/*
 * Check if two VIEW records are the same.  May fail due to floating point
 * roundoff effects!
 */
{
    return (
	    ( v1->type == v2->type ) &&

	    ( v1->vp[0] == v2->vp[0] ) &&
	    ( v1->vp[1] == v2->vp[1] ) &&
	    ( v1->vp[2] == v2->vp[2] ) &&

	    ( v1->vdir[0] == v2->vdir[0] ) &&
	    ( v1->vdir[1] == v2->vdir[1] ) &&
	    ( v1->vdir[2] == v2->vdir[2] ) &&

	    ( v1->vup[0] == v2->vup[0] ) &&
	    ( v1->vup[1] == v2->vup[1] ) &&
	    ( v1->vup[2] == v2->vup[2] ) &&

	    ( v1->vdist == v2->vdist ) &&

	    ( v1->horiz == v2->horiz ) &&

	    ( v1->vert == v2->vert ) &&

	    ( v1->hoff == v2->hoff ) &&

	    ( v1->voff == v2->voff ) &&

	    ( v1->vfore == v2->vfore ) &&

	    ( v1->vaft == v2->vaft ) &&

	    ( v1->hvec[0] == v2->hvec[0] ) &&
	    ( v1->hvec[1] == v2->hvec[1] ) &&
	    ( v1->hvec[2] == v2->hvec[2] ) &&

	    ( v1->vvec[0] == v2->vvec[0] ) &&
	    ( v1->vvec[1] == v2->vvec[1] ) &&
	    ( v1->vvec[2] == v2->vvec[2] ) &&

	    ( v1->hn2 == v2->hn2 ) &&

	    ( v1->vn2 == v2->vn2 )
	   );
}
#endif	/* VIEW_COMP */
