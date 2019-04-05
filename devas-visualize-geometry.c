/*
 * Generate displayable version of DeVAS geometry file as a PNG file.
 * 3D values are displayed in RGB, with R=|x|, G=|y|, and B=|z|.  1D
 * values are displayed with positive values in green and negative
 * values in red.  Black indicates a zero value.  The --3dx1=dim flag
 * displays a single dimension of at 3D file using the same coloring
 * scheme as for 1D data.  <a> is a single characters specifying the
 * dimension ('x', 'y', or 'z').  For example, the following could be
 * used for displaying the Y dimension of an xyz.txt file:
 *
 * 	devas-visualize-geometry --3dx1=y input_xyz.txt output.png
 *
 * Display values are scaled by the extremal value of the input.  For
 * 1d data, effectively infinite values (holes in distance data) are
 * displayed in orange.  This is not needed for xyz or nor data, since
 * in those data files hole result in zero values.
 */

/* #define	DeVAS_CHECK_BOUNDS */

#define	MAX_DST		(1e+8)	/* any distance larger than this is infinite */
				/* and thus a hole */
#define	WARN_RED	255
#define	WARN_GREEN	127
#define	WARN_BLUE	0

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include "devas-image.h"
#include "read-geometry.h"
#include "devas-sRGB.h"
#include "devas-png.h"
#include "devas-filter-version.h"
#include "devas-license.h"

char	*Usage =
	    "devas-visualize-geometry [--3dx1=<a>] [--print] [--outline]"
	    "\n\t[print-row-start print-col-start print-row-end print-col-end]"
	    "\n\tgeom.txt vis.png";
int	args_needed = 2;

double  rescale ( double old_value, double old_min, double old_max,
	    double new_min, double new_max );
int	print_coordinates_in_image ( int print_row_start, int print_col_start,
	    int print_row_end, int print_col_end, int n_rows, int n_cols );

int
main ( int argc, char *argv[] )
{
    char		dim_to_show = 'n';	/* default is to show 3D */
    /* data with each dimension */
    /* in a different color */
    int			geo_dim;
    DeVAS_float_image	*data_1d;
    DeVAS_XYZ_image	*data_3d;
    DeVAS_RGB_image	*display;
    DeVAS_RGBf		display_value_float;
    int			row, col;
    float		max_X, max_Y, max_Z, max_1d;
    float		min_X, min_Y, min_Z, min_1d;
    float		max_all_dimensions, min_all_dimensions;
    float		norm_3d, norm_1d;
    int			verbose = FALSE;
    int			warn_only = FALSE;
    int			fullrange = FALSE;
    int			reverse = FALSE;
    int			found_valid_point;
    int			print_flag = FALSE;
    int			print_row_start = -1;
    int			print_col_start = -1;
    int			print_row_end = -1;
    int			print_col_end = -1;
    int			outline_flag = FALSE;
    DeVAS_RGB		cyan = { 0, 255, 255 };

    int			argpt = 1;

    /* scan for flags */
    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( strncasecmp ( argv[argpt], "--3dx1=",
		    strlen ( "--3dx1=" ) ) == 0 ) {
	    dim_to_show = tolower ( *( argv[argpt] + strlen ( "--3dx1=" ) ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-3dx1=",
		    strlen ( "-3dx1=" ) ) == 0 ) {
	    dim_to_show = *( argv[argpt] + strlen ( "-3dx1=" ) );
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--fullrange" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-fullrange" ) == 0 ) ) {
	    fullrange = TRUE;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--reverse" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-reverse" ) == 0 ) ) {
	    reverse = TRUE;
	    fullrange = TRUE;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--warnonly" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-warnonly" ) == 0 ) ) {
	    warn_only = TRUE;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--print" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-print" ) == 0 ) ) {
	    print_flag = TRUE;
	    args_needed += 4;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--outline" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-outline" ) == 0 ) ) {
	    outline_flag = TRUE;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--verbose" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-verbose" ) == 0 ) ) {
	    verbose = TRUE;
	    argpt++;
	} else {
	    fprintf ( stderr, "invalid flag (%s)!\n", argv[argpt] );
	    return ( EXIT_FAILURE );    /* error exit */
	}
    }

    /* check number of arguments */
    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );	/* error return */
    }

    /* other error checks */
    if ( ( dim_to_show != 'n' ) && ( dim_to_show != 'x' ) &&
	    ( dim_to_show != 'y' ) && ( dim_to_show != 'z' ) )  {
	fprintf ( stderr, "invalid dim_to_show (%d)!\n", dim_to_show );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	return ( EXIT_FAILURE );    /* error exit */
    }

    if ( outline_flag && !print_flag ) {
	fprintf ( stderr, "invalid dim_to_show (%d)!\n", dim_to_show );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	return ( EXIT_FAILURE );    /* error exit */
    }

    if ( print_flag ) {
	print_row_start = atoi ( argv[argpt++] );
	print_col_start = atoi ( argv[argpt++] );
	print_row_end = atoi ( argv[argpt++] );
	print_col_end = atoi ( argv[argpt++] );
    }

    geo_dim = DeVAS_geom_dim_from_radfilename ( argv[argpt] );
    /* need to reuse this argument below */

    /* argument compatibility check */
    if ( ( geo_dim == 1 ) && ( dim_to_show != 'n' ) ) {
	fprintf ( stderr, "can't specify --3dx1 for 1D data!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	return ( EXIT_FAILURE );    /* error exit */
    }

    if ( geo_dim == 1 ) {
	/* input is 1D */

	data_1d = DeVAS_geom1d_from_radfilename ( argv[argpt++] );

	if ( print_flag ) {
	    if ( print_coordinates_in_image ( print_row_start, print_col_start,
			print_row_end, print_col_end,
			DeVAS_image_n_rows ( data_1d ),
			DeVAS_image_n_cols ( data_1d ) ) ) {
		fprintf ( stderr,
			"print points outsize image or otherwise invalid!\n" );
		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
		exit ( EXIT_FAILURE );
	    }

	    for ( row = print_row_start; row <= print_row_end; row++ ) {
		for ( col = print_col_start; col <= print_col_end; col++ ) {
		    printf ( "%f ", DeVAS_image_data ( data_1d, row, col ) );
		}
		printf ( "\n" );
	    }
	    printf ( "\n" );
	}

	display = DeVAS_RGB_image_new ( DeVAS_image_n_rows ( data_1d ),
		DeVAS_image_n_cols ( data_1d ) );

	/* find extremal value in input */
	max_1d = min_1d = 0.0;	/* quiet -Wunused-variable */
	found_valid_point = FALSE;
	for ( row = 0; row < DeVAS_image_n_rows ( data_1d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_1d ); col++ ) {
		if ( DeVAS_image_data ( data_1d, row, col ) < MAX_DST ) {
		    max_1d = min_1d = DeVAS_image_data ( data_1d, row, col );
		    found_valid_point = TRUE;
		}
	    }
	}
	if ( !found_valid_point ) {
	    fprintf ( stderr, "no valid data!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}

	for ( row = 0; row < DeVAS_image_n_rows ( data_1d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_1d ); col++ ) {
		if ( DeVAS_image_data ( data_1d, row, col ) > MAX_DST ) {
		    continue;
		}
		if ( max_1d < DeVAS_image_data ( data_1d, row, col ) ) {
		    max_1d = DeVAS_image_data ( data_1d, row, col );
		} else if ( min_1d > DeVAS_image_data ( data_1d, row, col ) ) {
		    min_1d = DeVAS_image_data ( data_1d, row, col );
		}
	    }
	}

	if ( verbose ) {
	    printf ( "min_1d = %f, max_1d = %f\n", min_1d, max_1d );
	}

	if ( ( max_1d - min_1d ) == 0 ) {
	    fprintf ( stderr, "no dynamic range in geometry file!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    return ( EXIT_FAILURE );	/* error exit */
	}

	/* create visualization image, normalizing as appropriate */

	norm_1d = 1.0 / fmax ( max_1d, -min_1d );

	for ( row = 0; row < DeVAS_image_n_rows ( data_1d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_1d ); col++ ) {
		if ( DeVAS_image_data ( data_1d, row, col ) <= MAX_DST ) {
		    if ( warn_only ) {
			display_value_float.red = 0.0;
			display_value_float.green = 0.0;
			display_value_float.blue = 0.0;
		    } else {
			if ( fullrange && !reverse ) {
			    display_value_float.red =
				display_value_float.green =
				display_value_float.blue =
				rescale ( DeVAS_image_data ( data_1d, row,
					    col ), min_1d, max_1d,
					0.0, 1.0 );

			} else if ( fullrange && reverse ) {

			    display_value_float.red =
				display_value_float.green =
				display_value_float.blue = 1.0 -
				rescale ( DeVAS_image_data ( data_1d, row,
					    col ), min_1d, max_1d,
					0.0, 1.0 );
			} else if ( DeVAS_image_data ( data_1d, row, col )
				< 0.0 ) {
			    display_value_float.red = norm_1d *
				-DeVAS_image_data ( data_1d, row, col );
			    display_value_float.green = 0.0;
			    display_value_float.blue = 0.0;
			} else {
			    display_value_float.red = 0.0;
			    display_value_float.green = norm_1d *
				DeVAS_image_data ( data_1d, row, col );
			    display_value_float.blue = 0.0;
			}
		    }
		} else {
		    display_value_float.red = WARN_RED;
		    display_value_float.green = WARN_GREEN;
		    display_value_float.blue = WARN_BLUE;
		}

		DeVAS_image_data ( display, row, col ) =
		    /* RGBf_to_sRGB ( display_value_float ); */
		    RGBf_to_RGB ( display_value_float );
		/* perceptual encoding */
	    }
	}

	DeVAS_float_image_delete ( data_1d );

    } else if ( ( geo_dim == 3 ) && ( dim_to_show == 'n' ) ) {
	/*
	 * 3-D data, show absolute value of each dimension in a different
	 * color.
	 */

	if ( fullrange ) {
	    fprintf ( stderr, "--fullrange not valid for 3D data!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}

	if ( reverse ) {
	    fprintf ( stderr, "--reverse not valid for 3D data!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}

	data_3d = DeVAS_geom3d_from_radfilename ( argv[argpt++] );

	if ( print_flag ) {
	    if ( print_coordinates_in_image ( print_row_start, print_col_start,
			print_row_end, print_col_end,
			DeVAS_image_n_rows ( data_3d ),
			DeVAS_image_n_cols ( data_3d ) ) ) {
		fprintf ( stderr,
			"print points outsize image or otherwise invalid!\n" );
		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
		exit ( EXIT_FAILURE );
	    }

	    for ( row = print_row_start; row <= print_row_end; row++ ) {
		for ( col = print_col_start; col <= print_col_end; col++ ) {
		    printf ( "(%.2f,%.2f,%.2f) ",
			    DeVAS_image_data ( data_3d, row, col ) . X,
			    DeVAS_image_data ( data_3d, row, col ) . Y,
			    DeVAS_image_data ( data_3d, row, col ) . Z );
		}
		printf ( "\n" );
	    }
	    printf ( "\n" );
	}

	display = DeVAS_RGB_image_new ( DeVAS_image_n_rows ( data_3d ),
		DeVAS_image_n_cols ( data_3d ) );

	/* find extremal value in input */

	max_X = min_X = DeVAS_image_data ( data_3d, 0, 0 ) . X;
	max_Y = min_Y = DeVAS_image_data ( data_3d, 0, 0 ) . Y;
	max_Z = min_Z = DeVAS_image_data ( data_3d, 0, 0 ) . Z;

	for ( row = 0; row < DeVAS_image_n_rows ( data_3d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_3d ); col++ ) {

		if ( max_X <
			DeVAS_image_data ( data_3d, row, col ) . X ) {
		    max_X = DeVAS_image_data ( data_3d, row, col ) . X;
		} else if ( min_X >
			DeVAS_image_data ( data_3d, row, col ) . X ) {
		    min_X = DeVAS_image_data ( data_3d, row, col ) . X;
		}

		if ( max_Y <
			DeVAS_image_data ( data_3d, row, col ) . Y ) {
		    max_Y = DeVAS_image_data ( data_3d, row, col ) . Y;
		} else if ( min_Y >
			DeVAS_image_data ( data_3d, row, col ) . Y ) {
		    min_Y = DeVAS_image_data ( data_3d, row, col ) . Y;
		}

		if ( max_Z <
			DeVAS_image_data ( data_3d, row, col ) . Z ) {
		    max_Z = DeVAS_image_data ( data_3d, row, col ) . Z;
		} else if ( min_Z >
			DeVAS_image_data ( data_3d, row, col ) . Z ) {
		    min_Z = DeVAS_image_data ( data_3d, row, col ) . Z;
		}
	    }
	}

	if ( verbose ) {
	    printf ( "min_X = %f, max_X = %f\n", min_X, max_X );
	    printf ( "min_Y = %f, max_Y = %f\n", min_Y, max_Y );
	    printf ( "min_Z = %f, max_Z = %f\n", min_Z, max_Z );
	}

	max_all_dimensions = fmax ( fmax ( max_X, max_Y ), max_Z );
	min_all_dimensions = fmin ( fmin ( min_X, min_Y ), min_Z );

	if ( ( max_all_dimensions - min_all_dimensions ) == 0 ) {
	    fprintf ( stderr,
		    "no dynamic range in geometry file!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    return ( EXIT_FAILURE );	/* error exit */
	}

	/* create visualization image, normalizing as appropriate */

	norm_3d = 1.0 / fmax ( max_all_dimensions, -min_all_dimensions );

	for ( row = 0; row < DeVAS_image_n_rows ( data_3d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_3d ); col++ ) {
		display_value_float.red = fabs ( norm_3d *
			DeVAS_image_data ( data_3d, row, col ) . X );
		display_value_float.green = fabs ( norm_3d *
			DeVAS_image_data ( data_3d, row, col ) . Y );
		display_value_float.blue = fabs ( norm_3d *
			DeVAS_image_data ( data_3d, row, col ) . Z );

		DeVAS_image_data ( display, row, col ) =
		    /* RGBf_to_sRGB ( display_value_float ); */
		    RGBf_to_RGB ( display_value_float );
	    }
	}

	DeVAS_XYZ_image_delete ( data_3d );

    } else if ( ( geo_dim == 3 ) && ( dim_to_show != 'n' ) ) {
	/* 3-D data, show selected dimension as for 1-D data */

	data_3d = DeVAS_geom3d_from_radfilename ( argv[argpt++] );
	display = DeVAS_RGB_image_new ( DeVAS_image_n_rows ( data_3d ),
		DeVAS_image_n_cols ( data_3d ) );

	data_1d = DeVAS_float_image_new ( DeVAS_image_n_rows ( data_3d ),
		DeVAS_image_n_cols ( data_3d ) );

	/* copy selected dimension to 1-D image */
	switch ( dim_to_show ) {
	    case 'x':
		for ( row = 0; row < DeVAS_image_n_rows ( data_3d ); row++ ) {
		    for ( col = 0; col < DeVAS_image_n_cols ( data_3d );
			    col++ ) {
			DeVAS_image_data ( data_1d, row, col ) =
			    DeVAS_image_data ( data_3d, row, col ) . X;
		    }
		}
		break;

	    case 'y':
		for ( row = 0; row < DeVAS_image_n_rows ( data_3d ); row++ ) {
		    for ( col = 0; col < DeVAS_image_n_cols ( data_3d );
			    col++ ) {
			DeVAS_image_data ( data_1d, row, col ) =
			    DeVAS_image_data ( data_3d, row, col ) . Y;
		    }
		}
		break;

	    case 'z':
		for ( row = 0; row < DeVAS_image_n_rows ( data_3d ); row++ ) {
		    for ( col = 0; col < DeVAS_image_n_cols ( data_3d );
			    col++ ) {
			DeVAS_image_data ( data_1d, row, col ) =
			    DeVAS_image_data ( data_3d, row, col ) . Z;
		    }
		}
		break;

	    default:
		fprintf ( stderr, "internal error!\n" );
		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
		return ( EXIT_FAILURE );    /* error exit */
		break;
	}

	if ( print_flag ) {
	    if ( print_coordinates_in_image ( print_row_start, print_col_start,
			print_row_end, print_col_end,
			DeVAS_image_n_rows ( data_1d ),
			DeVAS_image_n_cols ( data_1d ) ) ) {
		fprintf ( stderr,
			"print points outsize image or otherwise invalid!\n" );
		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
		exit ( EXIT_FAILURE );
	    }

	    for ( row = print_row_start; row <= print_row_end; row++ ) {
		for ( col = print_col_end; col <= print_col_end; col++ ) {
		    printf ( "%f ", DeVAS_image_data ( data_1d, row, col ) );
		}
		printf ( "\n" );
	    }
	    printf ( "\n" );
	}

	/* find extremal value in input */

	max_1d = min_1d = DeVAS_image_data ( data_1d, 0, 0 );

	for ( row = 0; row < DeVAS_image_n_rows ( data_1d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_1d ); col++ ) {
		if ( max_1d < DeVAS_image_data ( data_1d, row, col ) ) {
		    max_1d = DeVAS_image_data ( data_1d, row, col );
		} else if ( min_1d > DeVAS_image_data ( data_1d, row, col ) ) {
		    min_1d = DeVAS_image_data ( data_1d, row, col );
		}
	    }
	}

	if ( verbose ) {
	    printf ( "min_%c = %f, max_%c = %f\n", dim_to_show, min_1d,
		    dim_to_show, max_1d );
	}

	if ( ( max_1d - min_1d ) == 0 ) {
	    fprintf ( stderr,
		    "no dynamic range in geometry file!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    return ( EXIT_FAILURE );	/* error exit */
	}

	/* create visualization image, normalizing as appropriate */

	norm_1d = 1.0 / fmax ( max_1d, -min_1d );

	for ( row = 0; row < DeVAS_image_n_rows ( data_1d ); row++ ) {
	    for ( col = 0; col < DeVAS_image_n_cols ( data_1d ); col++ ) {
		if ( DeVAS_image_data ( data_1d, row, col ) < 0.0 ) {
		    display_value_float.red = norm_1d *
			-DeVAS_image_data ( data_1d, row, col );
		    display_value_float.green = 0.0;
		    display_value_float.blue = 0.0;
		} else {
		    display_value_float.red = 0.0;
		    display_value_float.green = norm_1d *
			DeVAS_image_data ( data_1d, row, col );
		    display_value_float.blue = 0.0;
		}
		DeVAS_image_data ( display, row, col ) =
		    RGBf_to_sRGB ( display_value_float );
	    }
	}

	DeVAS_float_image_delete ( data_1d );
	DeVAS_XYZ_image_delete ( data_3d );

    } else {
	fprintf ( stderr, "internal error!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	return ( EXIT_FAILURE );    /* error exit */
    }

    if ( outline_flag ) {
	for ( row = print_row_start; row <= print_row_end; row ++ ) {
	    DeVAS_image_data ( display, row, print_col_start ) =
		DeVAS_image_data ( display, row, print_col_end ) = cyan;
	}

	for ( col = print_col_start; col <= print_col_end; col ++ ) {
	    DeVAS_image_data ( display, print_row_start, col ) =
		DeVAS_image_data ( display, print_row_end, col ) = cyan;
	}
    }

    DeVAS_RGB_image_to_filename_png ( argv[argpt++], display );

    DeVAS_RGB_image_delete ( display );

    return ( EXIT_SUCCESS );	/* normal exit */
}

double
rescale ( double old_value, double old_min, double old_max, double new_min,
	        double new_max )
/*
 * Rescale old_value, asserted to be in range [old_min - old_max] to new
 * range [new_min - new_max].
 */
{
    double new_value;

    new_value = ( ( ( old_value - old_min ) *
		    ( new_max - new_min ) ) / ( old_max - old_min ) ) +
			new_min;

    return ( new_value );
}

int
print_coordinates_in_image ( int print_row_start, int print_col_start,
	int print_row_end, int print_col_end, int n_rows, int n_cols )
{
    return ( !( ( print_row_start >= 0 ) && ( print_row_start < n_rows ) &&
	    ( print_row_end >= 0 ) && ( print_row_end < n_rows ) &&
        ( print_col_start >= 0 ) && ( print_col_start < n_cols ) &&
	    ( print_col_end >= 0 ) && ( print_col_end < n_cols ) &&
	( print_row_start <= print_row_end ) &&
	    ( print_col_start <= print_col_end ) ) );
}
