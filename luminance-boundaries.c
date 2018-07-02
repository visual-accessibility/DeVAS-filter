#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "deva-image.h"
#include "deva-canny.h"
#include "radianceIO.h"
#include "DEVA-png.h"
#include "deva-license.h"	/* DEVA open source license */

#define	CANNY_ST_DEV	M_SQRT2
#define EPSILON		0.1

char	*progname;	/* one radiance routine requires this as a global */

char	*Usage = "luminance-boundaries input.hdr output.png";
int	args_needed = 2;

int
main ( int argc, char *argv[] )
{
    DEVA_float_image	*input_image = NULL;	/* Y values in cd/m^2 */
    DEVA_xyY_image	*input_image_xyY;	/* Y values in cd/m^2 */
    int			row, col;
    DEVA_gray_image	*output_image = NULL;
    int			argpt = 1;

    /* scan and collect option flags */
    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( ( strcasecmp ( argv[argpt], "--nonargument" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-nonargument" ) == 0 ) ) {
	    exit ( EXIT_FAILURE );
	    argpt++;
	} else {
	    fprintf ( stderr, "%s: invalid flag (%s)!\n", progname,
		    argv[argpt] );
	    return ( EXIT_FAILURE );    /* error exit */
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );    /* error exit */
    }

    input_image_xyY = DEVA_xyY_image_from_radfilename ( argv[argpt++] );
    input_image = DEVA_float_image_new ( DEVA_image_n_rows ( input_image_xyY ),
	    DEVA_image_n_cols ( input_image_xyY ) );

    for ( row = 0; row < DEVA_image_n_rows ( input_image_xyY ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( input_image_xyY ); col++ ) {
	    DEVA_image_data ( input_image, row, col ) =
		DEVA_image_data ( input_image_xyY, row, col ).Y;
	}
    }

    output_image = deva_canny_autothresh ( input_image, CANNY_ST_DEV,
	    NULL, NULL );

    for ( row = 0; row < DEVA_image_n_rows ( output_image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( output_image ); col++ ) {
	    if ( DEVA_image_data ( output_image, row, col ) != 0 ) {
		DEVA_image_data ( output_image, row, col ) = 255;
	    }
	}
    }

    DEVA_gray_image_to_filename_png ( argv[argpt++], output_image );

    DEVA_float_image_delete ( input_image );
    DEVA_xyY_image_delete ( input_image_xyY );
    DEVA_gray_image_delete ( output_image );

    return ( EXIT_SUCCESS );	/* normal exit */
}
