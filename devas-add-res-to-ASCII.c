/*
 * DeVAS geometry files are constructed from a Radiance .hdr image and the oct
 * tree from which is was generated.  On Linux/unix and MacOS systems, the
 * following command line will produce a geometry file for one of the possible
 * geometry types:
 *
 *   vwrays -fd scene.hdr | rtrace -fda `vwrays -d scene.hdr` -o? \
 *           scene.oct > geom.txt
 *
 * where scene.hdr is the Radiance file containing the image of the scene,
 * scene.oct is the oct file for the scene, and -o? is
 *
 *   -op     xyz position
 *   -on     surface normal
 *   -oL     distance
 *
 * The standard command window for Windows does not support command
 * substitution, and pipes in Windows are sometimes flaky for binary data.  As
 * a result, the following should be used with Windows:
 *
 *   vwrays -fa scene.hdr | rtrace -fa -ld- -oL scene.oct | ^
 *           devas-add-res-to-ASCII - scene.hdr > geom.txt
 *
 * Without the `vwrays -d scene.hdr` command substitution, rtrace produces an
 * output file that does not contain the Radiance resolution record.
 * devas-add-res-to-ASCII (this program) inserts the resolution record in the
 * appropriate place in the stream.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "radiance-header.h"
#include "read-geometry.h"
#include "devas-license.h"	/* DeVAS open source license */

#define BUFSIZE		2000

char	*Usage = "devas-add-res-to-ASCII in.hdr res.hdr";
int	args_needed = 2;

int	check_radiance_header ( FILE *in_fp, char *res_filename );

int
main ( int argc, char *argv[] )
{
    FILE	*in_fp = NULL;
    char	*res_filename = NULL;
    char	buffer[BUFSIZE];
    int		argpt = 1;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( strcmp ( argv[argpt], "-" ) == 0 ) {
	    break;	/* read from stdin? */
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );	/* error exit */
    }

    if ( strcmp ( argv[argpt], "-" ) == 0 ) {
	in_fp = stdin;
    } else {
	in_fp = fopen ( argv[argpt], "r" );
    }
    argpt++;

    if ( in_fp == NULL ) {
	perror ( "error opening input file" );
	exit ( EXIT_FAILURE );
    }

    res_filename = argv[argpt++];

    check_radiance_header ( in_fp, res_filename );

    while ( fgets ( buffer, BUFSIZE, in_fp ) != NULL ) {
	fputs ( buffer, stdout );
    }

    return ( EXIT_SUCCESS );
}

int
check_radiance_header ( FILE *in_fp, char *res_filename )
{
    int	    first_line;
    int	    last_line_empty;
    int	    cr_found = FALSE;
    char    buffer[BUFSIZE];
    char    *cpt;
    RESOLU  rs;
    FILE    *res_fp;
    int	    n_rows, n_cols;

    last_line_empty = FALSE;
    		/* indicates status of previously processed line */
    
    first_line = TRUE;	/* indicates that current line is first line */

    while ( fgets ( buffer, BUFSIZE, in_fp ) != NULL ) {
	/* first line is (or at least should be) special */

	if ( first_line && ( strcmp ( buffer, "#?RADIANCE\n" ) != 0 ) ) {
	    fprintf ( stderr, "not RADIANCE file!\n" );
	    exit ( EXIT_FAILURE );	/* error return */
	}

	if ( last_line_empty ) {
	    /* if the last line was empty, then the next line should be a */
	    /* resolution record */

	    if ( str2resolu ( &rs, buffer ) ) {/* check for resolution record */

		printf ( "%s", buffer );
				/* add existing resolution record to output */

		return ( TRUE );	/* done with a successful search for */
					/* an existing resolution record */

	    } else {

		/*
		 * Read a radiance image file to get its resolution information.
		 * (The rest of this file is ignored.) Note that the current
		 * contents of buffer is valid data that needs to be output
		 * *after* the new resolution record.
		 */
		res_fp = fopen ( res_filename, "r" );
		if ( res_fp == NULL ) {
		    perror ( "error opening resolution file" );
		    exit ( EXIT_FAILURE );
		}

		DeVAS_read_radiance_header ( res_fp, &n_rows, &n_cols, NULL,
			NULL, NULL, NULL, NULL );
				/* get dimensions of the just opened file */

		fclose ( res_fp );

		printf ( "-Y %d +X %d\n", n_rows, n_cols );
			/* write new resolution record */

		printf ( "%s", buffer );
			/* write previously read data record */

		return ( FALSE );
	    }
	}

	printf ( "%s", buffer );	/* still reading through and writing */
					/* out the header information */

	first_line = FALSE;

	cpt = buffer;
	cr_found = FALSE;
	while ( *cpt != '\0' ) {
	    if ( *cpt == '\n' ) {
		cr_found = TRUE;
		break;
	    }
	    cpt++;
	}
	if ( !cr_found ) {
	    fprintf ( stderr, "line too long or missing <return>!\n" );
	    exit ( EXIT_FAILURE );
	}

	if ( buffer[0] == '\n' ) {
	    last_line_empty = TRUE;
	}
    }

    fprintf ( stderr, "end-of-header not found!\n" );
    exit ( EXIT_FAILURE );
}
