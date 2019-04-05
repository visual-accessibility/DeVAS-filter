#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <libgen.h>
#include <strings.h>
#include <string.h>
#include "dilate.h"
#include "devas-visibility.h"
#include "visualize-hazards.h"
#include "devas-image.h"
#ifdef DeVAS_USE_CAIRO
#include "devas-add-text.h"
#endif	/* DeVAS_USE_CAIRO */
#include "devas-png.h"
#include "read-geometry.h"

#define	HAZARD_NO_EDGE	(-1.0)
#define	TEXT_DEFAULT_FONT_SIZE	24.0

#define	SQ(x)	((x) * (x))

char	*Usage =
"devas-compare-boundaries [--red-green|--red-gray]"
	"\n\t[--Gaussian=<sigma>|--reciprocal=<scale>|--linear=<max>]"
#ifdef DeVAS_USE_CAIRO
	"\n\t[--quantscore] [--fontsize=<n>]"
#endif	/* DeVAS_USE_CAIRO */
	"\n\t[--mask=<mask-filename>]"
	"\n\tstandard.png comparison.png coord visualization.png";
int	args_needed = 4;

int		    imax ( int a, int b );
DeVAS_float_image    *compute_hazards ( DeVAS_gray_image *standard,
			DeVAS_float_image *comparison_edge_distance,
			double degrees_per_pixel );
void		    make_visible ( DeVAS_gray_image *boundaries );

#ifdef DeVAS_USE_CAIRO
void		    add_quantscore ( char *standard_name,
			char *comparison_name, DeVAS_float_image *hazards,
       			DeVAS_RGB_image *hazards_visualization,
			double text_font_size,
			Measurement_type measurement_type,
			double scale_parameter,
			Visualization_type visualization_type,
			int log_output_flag, char *log_output_filename );
#endif	/* DeVAS_USE_CAIRO */

int
main ( int argc, char *argv[] )
{
#ifdef DeVAS_USE_CAIRO
    int			log_output_flag = FALSE;
    char		*log_output_filename = NULL;
#endif	/* DeVAS_USE_CAIRO */
    Visualization_type	visualization_type = red_gray_type;	/* default */
    Measurement_type	measurement_type = Gaussian_measure;	/* default */
    double		max_hazard = 2.0;			/* default */
    double		reciprocal_scale = 1.0;			/* default */
    double		Gaussian_sigma = 0.75;			/* default */
    double		scale_parameter;

#ifdef DeVAS_USE_CAIRO
    int			quantscore = FALSE;
    double		text_font_size = TEXT_DEFAULT_FONT_SIZE;
#endif	/* DeVAS_USE_CAIRO */

    DeVAS_gray_image	*standard;
    char		*standard_name;
    DeVAS_gray_image	*comparison;
    char		*comparison_name;
    DeVAS_coordinates	*coordinates;
    double		degrees_per_pixel;
    DeVAS_float_image	*comparison_edge_distance;
    DeVAS_float_image	*hazards;
    DeVAS_RGB_image	*hazards_visualization;
    char		*mask_file_name = NULL;
    DeVAS_gray_image	*mask = NULL;
    int			argpt = 1;

    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( ( strcasecmp ( argv[argpt], "--red-green" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-red-green" ) == 0 ) ) {
	    visualization_type = red_green_type;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--red-gray" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-red-gray" ) == 0 ) ) {
	    visualization_type = red_gray_type;
	    argpt++;
#ifdef DeVAS_USE_CAIRO
	} else if ( ( strcasecmp ( argv[argpt], "--quantscore" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-quantscore" ) == 0 ) ) {
	    quantscore = TRUE;
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "--fontsize=",
		    strlen ( "--fontsize=" ) ) == 0 ) {
	    text_font_size = atof ( argv[argpt] +
		    strlen ( "--fontsize=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-fontsize=",
		    strlen ( "-fontsize=" ) ) == 0 ) {
	    text_font_size = atof ( argv[argpt] +
		    strlen ( "-fontsize=" ) );
	    argpt++;
#endif	/* DeVAS_USE_CAIRO */
	} else if ( strncasecmp ( argv[argpt], "--reciprocal=",
		    strlen ( "--reciprocal=" ) ) == 0 ) {
	    measurement_type = reciprocal_measure;
	    reciprocal_scale = atof ( argv[argpt] +
		    strlen ( "--reciprocal=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-reciprocal=",
		    strlen ( "-reciprocal=" ) ) == 0 ) {
	    measurement_type = reciprocal_measure;
	    reciprocal_scale = atof ( argv[argpt] +
		    strlen ( "-reciprocal=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "--linear=",
		    strlen ( "--linear=" ) ) == 0 ) {
	    measurement_type = linear_measure;
	    max_hazard = atof ( argv[argpt] + strlen ( "--linear=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-linear=",
		    strlen ( "-linear=" ) ) == 0 ) {
	    measurement_type = linear_measure;
	    max_hazard = atof ( argv[argpt] + strlen ( "-linear=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "--Gaussian=",
		    strlen ( "--Gaussian=" ) ) == 0 ) {
	    measurement_type = Gaussian_measure;
	    Gaussian_sigma = atof ( argv[argpt] + strlen ( "--Gaussian=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-Gaussian=",
		    strlen ( "-Gaussian=" ) ) == 0 ) {
	    measurement_type = Gaussian_measure;
	    Gaussian_sigma = atof ( argv[argpt] + strlen ( "-Gaussian=" ) );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "--mask=",
		    strlen ( "--mask=" ) ) == 0 ) {
	    mask_file_name = argv[argpt] + strlen ( "--mask=" );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "-mask=",
		    strlen ( "-mask=" ) ) == 0 ) {
	    mask_file_name = argv[argpt] + strlen ( "-mask=" );
	    argpt++;

	/* hidden options */
#ifdef DeVAS_USE_CAIRO
	} else if ( strncasecmp ( argv[argpt], "--log=",
		    strlen ( "--log=" ) ) == 0 ) {
	    log_output_flag = TRUE;
	    log_output_filename = argv[argpt] + strlen ( "--log=" );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-log=",
		    strlen ( "-log=" ) ) == 0 ) {
	    log_output_flag = TRUE;
	    log_output_filename = argv[argpt] + strlen ( "-log=" );
	    argpt++;
#endif	/* DeVAS_USE_CAIRO */
	} else {
	    fprintf ( stderr, "devas-compare-boundaries: invalid flag (%s)!\n",
		    argv[argpt] );
	    return ( EXIT_FAILURE );	/* error return */
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );        /* error return */
    }

    standard_name = argv[argpt++];
    standard = DeVAS_gray_image_from_filename_png ( standard_name );
    comparison_name = argv[argpt++];
    comparison = DeVAS_gray_image_from_filename_png ( comparison_name );

    if ( !DeVAS_image_samesize ( standard, comparison ) ) {
	fprintf ( stderr, "%s and %s not same size!\n", standard_name,
		comparison_name );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	return ( EXIT_FAILURE );        /* error return */
    }

    coordinates = DeVAS_coordinates_from_filename ( argv[argpt++] );

    degrees_per_pixel = fmax ( coordinates->view.vert,
	    coordinates->view.horiz ) /
	((double) imax ( DeVAS_image_n_rows ( standard ),
	    DeVAS_image_n_cols ( standard ) ) );

    /* squared-distance transform */
    comparison_edge_distance = dt_euclid_sq ( comparison );

    /*
     * Compute distance, represented as a visual angle, from each standard
     * boundary pixel to nearest comparison boundary pixel.
     */

    hazards = compute_hazards ( standard, comparison_edge_distance,
	    degrees_per_pixel );

    /* clean up */
    DeVAS_gray_image_delete ( comparison );
    DeVAS_float_image_delete ( comparison_edge_distance );

    switch ( measurement_type ) {
	case reciprocal_measure:
	    scale_parameter = reciprocal_scale;
	    break;

	case linear_measure:
	    scale_parameter = max_hazard;
	    break;

	case Gaussian_measure:
	    scale_parameter = Gaussian_sigma;
	    break;

	default:
	    fprintf ( stderr, "invalid measurement_type!\n" );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
    }

    if ( mask_file_name != NULL ) {
	mask = DeVAS_gray_image_from_filename_png ( mask_file_name );
    }

    hazards_visualization = visualize_hazards ( hazards, measurement_type,
	    scale_parameter, visualization_type, mask, NULL, &standard, NULL );

    /* clean up */
    DeVAS_gray_image_delete ( standard );

#ifdef DeVAS_USE_CAIRO
    if ( quantscore ) {
	add_quantscore ( standard_name, comparison_name, hazards,
		hazards_visualization, text_font_size,
		measurement_type, scale_parameter,
		visualization_type,
		log_output_flag, log_output_filename );
    }
#endif	/* DeVAS_USE_CAIRO */

    DeVAS_RGB_image_to_filename_png ( argv[argpt++], hazards_visualization );

    /* clean up */
    DeVAS_float_image_delete ( hazards );
    DeVAS_RGB_image_delete ( hazards_visualization );
    DeVAS_coordinates_delete ( coordinates );
    if ( mask_file_name != NULL ) {
	DeVAS_gray_image_delete ( mask );
    }

    return ( EXIT_SUCCESS );	/* normal exit */
}

int
imax ( int a, int b )
{
    if ( a > b ) {
	return ( a );
    } else {
	return ( b );
    }
}

DeVAS_float_image *
compute_hazards ( DeVAS_gray_image *standard,
	DeVAS_float_image *comparison_edge_distance, double degrees_per_pixel )
/*
 * Compute distance, represented as a visual angle, from each geometric
 * boundary pixel to nearest luminance boundary pixel.
 *
 * standard:	TRUE is pixel is a geometric edge.
 *
 * comparison_edge_distance:	Squared distance to nearest luminance edge.
 *
 * degrees_per_pixel:	Angle in degrees between two horizontally or vertically
 * 			adjacent pixel locations.
 *
 * returned value:	DeVAS_float_image image object. Non-negative pixel
 * 			values give distance from geometry boundary at
 * 			pixel location to nearest luminance boundary element.
 * 			Negative pixel values indicate that there is no
 * 			geometry boundary at that location.
 */
{
    int			row, col;
    DeVAS_float_image	*hazards;

    if ( ! DeVAS_image_samesize ( standard, comparison_edge_distance ) ) {
	fprintf ( stderr, "compute_hazards: argument size mismatch!\n" );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    if ( degrees_per_pixel <= 0.0 ) {
	fprintf ( stderr, "compute_hazards: invalid degrees_per_pixel (%f)\n",
		degrees_per_pixel );
	DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	exit ( EXIT_FAILURE );
    }

    hazards = DeVAS_float_image_new ( DeVAS_image_n_rows ( standard ),
	    DeVAS_image_n_cols ( standard ) );

    for ( row = 0; row < DeVAS_image_n_rows ( standard ); row++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( standard );
		col++ ) {
	    if ( DeVAS_image_data ( standard, row, col ) ) {
		DeVAS_image_data ( hazards, row, col ) = degrees_per_pixel *
		    sqrt ( DeVAS_image_data ( comparison_edge_distance, row,
				col ) );
	    } else {
		DeVAS_image_data ( hazards, row, col ) = HAZARD_NO_EDGE;
	    }
	}
    }

    return ( hazards );
}

void
make_visible ( DeVAS_gray_image *boundaries )
/*
 * Change 0,1 values in a boolean file to 0,255.  This preserves TRUE/FALSE,
 * while making the file directly displayable.
 */
{
    int     row, col;

    for ( row = 0; row < DeVAS_image_n_rows ( boundaries ); row ++ ) {
	for ( col = 0; col < DeVAS_image_n_cols ( boundaries );
		col ++ ) {
	    if ( DeVAS_image_data ( boundaries, row,
			col ) ) {
		DeVAS_image_data ( boundaries, row, col ) = 255;
	    }
	}
    }
}

#define	BUFF_SIZE		200
#define	TEXT_ROW_1		30.0
#define	TEXT_ROW_2		60.0
#define	TEXT_ROW_3		90.0
#define	TEXT_COL		10.0
#define	TEXT_RED		1.0
#define	TEXT_GREEN		1.0
#define	TEXT_BLUE		1.0

#ifdef DeVAS_USE_CAIRO

void
add_quantscore ( char *standard_name, char *comparison_name,
	DeVAS_float_image *hazards, DeVAS_RGB_image *hazards_visualization,
	double text_font_size,
	Measurement_type measurement_type, double scale_parameter,
	Visualization_type visualization_type,
	int log_output_flag, char *log_output_filename )
{
    int		n_rows, n_cols;
    int		row, col;
    double	sum;
    int		count;
    char	text[BUFF_SIZE];
    char	*visualization_code;

    DeVAS_RGBf	text_color;
    cairo_surface_t *cairo_surface;
    FILE	*log_file_fd;

    n_rows = DeVAS_image_n_rows ( hazards );
    n_cols = DeVAS_image_n_cols ( hazards );

    sum = 0.0;
    count = 0;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( DeVAS_image_data ( hazards, row, col ) >= 0 ) {

		/*
		 * For matching measures, high values are good, low values
		 * are bad.  (This is opposite the conventions for hazard
		 * visualization.)
		 */

		switch ( measurement_type ) {

		    case reciprocal_measure:
			sum += scale_parameter /
			    ( DeVAS_image_data ( hazards, row, col ) +
			    				scale_parameter );
			break;

		    case linear_measure:
			sum += 1.0 - ( fmin ( DeVAS_image_data ( hazards, row,
				col ), scale_parameter ) / scale_parameter );
			break;

		    case Gaussian_measure:
			sum += exp ( -0.5 * ( SQ ( DeVAS_image_data ( hazards,
					    row, col ) / scale_parameter ) ) );
			break;

		    default:
			fprintf ( stderr,
				"devas-compare-boundaries: internal error!\n" );
			DeVAS_print_file_lineno ( __FILE__, __LINE__ );
			exit ( EXIT_FAILURE );	/* error return */
			break;
		}

		count++;
	    }
	}
    }

    visualization_code = NULL;

    switch ( visualization_type ) {

	case red_gray_type:
	    visualization_code = "red-gray";
	    break;

	case red_green_type:
	    visualization_code = "red-green";
	    break;

	default:
	    fprintf ( stderr, "devas-compare-boundaries: internal error!\n" );
	    break;
    }

    cairo_surface = DeVAS_RGB_cairo_open ( hazards_visualization );

    text_color.red = TEXT_RED;
    text_color.green = TEXT_GREEN;
    text_color.blue = TEXT_BLUE;

    if ( count == 0 ) {
	snprintf ( text, BUFF_SIZE,
		"%s / %s: no pixels in standard!",
		basename ( standard_name ), basename ( comparison_name ) );
	DeVAS_cairo_add_text ( cairo_surface, TEXT_ROW_1, TEXT_COL,
		text_font_size, text_color, text );
    } else {

	snprintf ( text, BUFF_SIZE, "%s / %s:",
		basename ( standard_name ), basename ( comparison_name ) );
	DeVAS_cairo_add_text ( cairo_surface, TEXT_ROW_1, TEXT_COL,
		text_font_size, text_color, text );

	switch ( measurement_type ) {
	    case reciprocal_measure:
		snprintf ( text, BUFF_SIZE,
			"reciprocal measure (scale = %.2f), %s",
			scale_parameter, visualization_code );
		break;

	    case linear_measure:
		snprintf ( text, BUFF_SIZE,
			"linear measure (max_hazard = %.2f), %s",
			scale_parameter, visualization_code );
		break;

	    case Gaussian_measure:
		snprintf ( text, BUFF_SIZE,
			"Gaussian measure (Gaussian_sigma = %.2f), %s",
			scale_parameter, visualization_code );
		break;

	    default:
		fprintf ( stderr,
			"devas-compare-boundaries: internal error!\n" );
		DeVAS_print_file_lineno ( __FILE__, __LINE__ );
		exit ( EXIT_FAILURE );	/* error return */
		break;
	}

	DeVAS_cairo_add_text ( cairo_surface, TEXT_ROW_2, TEXT_COL,
		text_font_size, text_color, text );

	snprintf ( text, BUFF_SIZE, "matching score = %.3f",
		sum / ((double) count ) );
	DeVAS_cairo_add_text ( cairo_surface, TEXT_ROW_3, TEXT_COL,
		text_font_size, text_color, text );
    }

    DeVAS_RGB_cairo_close_inplace ( cairo_surface, hazards_visualization );

    if ( log_output_flag ) {
	log_file_fd = fopen ( log_output_filename, "a" );
	if ( log_file_fd == NULL ) {
	    perror ( log_output_filename );
	    DeVAS_print_file_lineno ( __FILE__, __LINE__ );
	    exit ( EXIT_FAILURE );
	}

	if ( count > 0 ) {
	    fprintf ( log_file_fd, ",%.3f", sum / ((double) count ) );
	} else {
	    fprintf ( log_file_fd, "," );
	}

	fclose ( log_file_fd );
    }
}

#endif	/* DeVAS_USE_CAIRO */
