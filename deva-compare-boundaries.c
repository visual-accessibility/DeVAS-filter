#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <libgen.h>
#include <strings.h>
#include <string.h>
#include "dilate.h"
#include "visualize-hazards.h"
#include "deva-image.h"
#ifdef DEVA_USE_CAIRO
#include "deva-add-text.h"
#endif	/* DEVA_USE_CAIRO */
#include "DEVA-png.h"
#include "read-geometry.h"

#define	HAZARD_NO_EDGE	(-1.0)
#define	TEXT_DEFAULT_FONT_SIZE	24.0

#define	SQ(x)	((x) * (x))

char	*Usage =
"deva-compare-boundaries [--red-gray|--red-green]"
	"\n\t[--Gaussian=<sigma>|--reciprocal=<scale>|--linear=<max>]"
#ifdef DEVA_USE_CAIRO
	"\n\t[--quantscore] [--fontsize=<n>]"
#endif	/* DEVA_USE_CAIRO */
	"\n\tstandard.png comparison.png coord visualization.png";
int	args_needed = 4;

int		    imax ( int a, int b );
DEVA_float_image    *compute_hazards ( DEVA_gray_image *standard,
			DEVA_float_image *comparison_edge_distance,
			double degrees_per_pixel );
void		    make_visible ( DEVA_gray_image *boundaries );

#ifdef DEVA_USE_CAIRO
void		    add_quantscore ( char *standard_name,
			char *comparison_name, DEVA_float_image *hazards,
       			DEVA_RGB_image *hazards_visualization,
			double text_font_size,
			Measurement_type measurement_type,
			double scale_parameter,
			Visualization_type visualization_type,
			int log_output_flag, char *log_output_filename );
#endif	/* DEVA_USE_CAIRO */

int
main ( int argc, char *argv[] )
{
#ifdef DEVA_USE_CAIRO
    int			log_output_flag = FALSE;
    char		*log_output_filename = NULL;
#endif	/* DEVA_USE_CAIRO */
    Visualization_type	visualization_type = red_gray_type;	/* default */
    Measurement_type	measurement_type = Gaussian_measure;	/* default */
    double		max_hazard = 2.0;			/* default */
    double		reciprocal_scale = 1.0;			/* default */
    double		Gaussian_sigma = 0.75;			/* default */
    double		scale_parameter;

#ifdef DEVA_USE_CAIRO
    int			quantscore = FALSE;
    double		text_font_size = TEXT_DEFAULT_FONT_SIZE;
#endif	/* DEVA_USE_CAIRO */

    DEVA_gray_image	*standard;
    char		*standard_name;
    DEVA_gray_image	*comparison;
    char		*comparison_name;
    DEVA_coordinates	*coordinates;
    double		degrees_per_pixel;
    DEVA_float_image	*comparison_edge_distance;
    DEVA_float_image	*hazards;
    DEVA_RGB_image	*hazards_visualization;
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
#ifdef DEVA_USE_CAIRO
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
#endif	/* DEVA_USE_CAIRO */
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

	/* hidden options */
#ifdef DEVA_USE_CAIRO
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
#endif	/* DEVA_USE_CAIRO */
	} else {
	    fprintf ( stderr, "deva-compare-boundaries: invalid flag (%s)!\n",
		    argv[argpt] );
	    return ( EXIT_FAILURE );	/* error return */
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	fprintf ( stderr, "%s\n", Usage );
	return ( EXIT_FAILURE );        /* error return */
    }

    standard_name = argv[argpt++];
    standard = DEVA_gray_image_from_filename_png ( standard_name );
    comparison_name = argv[argpt++];
    comparison = DEVA_gray_image_from_filename_png ( comparison_name );

    if ( !DEVA_image_samesize ( standard, comparison ) ) {
	fprintf ( stderr, "%s and %s not same size!\n", standard_name,
		comparison_name );
	return ( EXIT_FAILURE );        /* error return */
    }

    coordinates = DEVA_coordinates_from_filename ( argv[argpt++] );

    degrees_per_pixel = fmax ( coordinates->view.vert,
	    coordinates->view.horiz ) /
	((double) imax ( DEVA_image_n_rows ( standard ),
	    DEVA_image_n_cols ( standard ) ) );

    /* squared-distance transform */
    comparison_edge_distance = dt_euclid_sq ( comparison );

    /*
     * Compute distance, represented as a visual angle, from each standard
     * boundary pixel to nearest comparison boundary pixel.
     */

    hazards = compute_hazards ( standard, comparison_edge_distance,
	    degrees_per_pixel );

    /* clean up */
    DEVA_gray_image_delete ( standard );
    DEVA_gray_image_delete ( comparison );
    DEVA_float_image_delete ( comparison_edge_distance );

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
	    exit ( EXIT_FAILURE );
    }

    hazards_visualization = visualize_hazards ( hazards, measurement_type,
	    scale_parameter, visualization_type );
#ifdef DEVA_USE_CAIRO
    if ( quantscore ) {
	add_quantscore ( standard_name, comparison_name, hazards,
		hazards_visualization, text_font_size,
		measurement_type, scale_parameter,
		visualization_type,
		log_output_flag, log_output_filename );
    }
#endif	/* DEVA_USE_CAIRO */

    DEVA_RGB_image_to_filename_png ( argv[argpt++], hazards_visualization );

    /* clean up */
    DEVA_float_image_delete ( hazards );
    DEVA_RGB_image_delete ( hazards_visualization );

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

DEVA_float_image *
compute_hazards ( DEVA_gray_image *standard,
	DEVA_float_image *comparison_edge_distance, double degrees_per_pixel )
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
 * returned value:	DEVA_float_image image object. Non-negative pixel
 * 			values give distance from geometry boundary at
 * 			pixel location to nearest luminance boundary element.
 * 			Negative pixel values indicate that there is no
 * 			geometry boundary at that location.
 */
{
    int			row, col;
    DEVA_float_image	*hazards;

    if ( ! DEVA_image_samesize ( standard, comparison_edge_distance ) ) {
	fprintf ( stderr, "compute_hazards: argument size mismatch!\n" );
	exit ( EXIT_FAILURE );
    }

    if ( degrees_per_pixel <= 0.0 ) {
	fprintf ( stderr, "compute_hazards: invalid degrees_per_pixel (%f)\n",
		degrees_per_pixel );
	exit ( EXIT_FAILURE );
    }

    hazards = DEVA_float_image_new ( DEVA_image_n_rows ( standard ),
	    DEVA_image_n_cols ( standard ) );

    for ( row = 0; row < DEVA_image_n_rows ( standard ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( standard );
		col++ ) {
	    if ( DEVA_image_data ( standard, row, col ) ) {
		DEVA_image_data ( hazards, row, col ) = degrees_per_pixel *
		    sqrt ( DEVA_image_data ( comparison_edge_distance, row,
				col ) );
	    } else {
		DEVA_image_data ( hazards, row, col ) = HAZARD_NO_EDGE;
	    }
	}
    }

    return ( hazards );
}

void
make_visible ( DEVA_gray_image *boundaries )
/*
 * Change 0,1 values in a boolean file to 0,255.  This preserves TRUE/FALSE,
 * while making the file directly displayable.
 */
{
    int     row, col;

    for ( row = 0; row < DEVA_image_n_rows ( boundaries ); row ++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( boundaries );
		col ++ ) {
	    if ( DEVA_image_data ( boundaries, row,
			col ) ) {
		DEVA_image_data ( boundaries, row, col ) = 255;
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

#ifdef DEVA_USE_CAIRO

void
add_quantscore ( char *standard_name, char *comparison_name,
	DEVA_float_image *hazards, DEVA_RGB_image *hazards_visualization,
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

    DEVA_RGBf	text_color;
    cairo_surface_t *cairo_surface;
    FILE	*log_file_fd;

    n_rows = DEVA_image_n_rows ( hazards );
    n_cols = DEVA_image_n_cols ( hazards );

    sum = 0.0;
    count = 0;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    if ( DEVA_image_data ( hazards, row, col ) >= 0 ) {

		/*
		 * For matching measures, hight values are good, low values
		 * are bad.  (This is opposite the convensions for hazard
		 * visualization.)
		 */

		switch ( measurement_type ) {

		    case reciprocal_measure:
			sum += scale_parameter /
			    ( DEVA_image_data ( hazards, row, col ) +
			    				scale_parameter );
			break;

		    case linear_measure:
			sum += 1.0 - ( fmin ( DEVA_image_data ( hazards, row,
				col ), scale_parameter ) / scale_parameter );
			break;

		    case Gaussian_measure:
			sum += exp ( -0.5 * ( SQ ( DEVA_image_data ( hazards,
					    row, col ) / scale_parameter ) ) );
			break;

		    default:
			fprintf ( stderr,
				"deva-compare-boundaries: internal error!\n" );
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
	    fprintf ( stderr, "deva-compare-boundaries: internal error!\n" );
	    break;
    }

    cairo_surface = DEVA_RGB_cairo_open ( hazards_visualization );

    text_color.red = TEXT_RED;
    text_color.green = TEXT_GREEN;
    text_color.blue = TEXT_BLUE;

    if ( count == 0 ) {
	snprintf ( text, BUFF_SIZE,
		"%s / %s: no pixels in standard!",
		basename ( standard_name ), basename ( comparison_name ) );
	DEVA_cairo_add_text ( cairo_surface, TEXT_ROW_1, TEXT_COL,
		text_font_size, text_color, text );
    } else {

	snprintf ( text, BUFF_SIZE, "%s / %s:",
		basename ( standard_name ), basename ( comparison_name ) );
	DEVA_cairo_add_text ( cairo_surface, TEXT_ROW_1, TEXT_COL,
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
			"deva-compare-boundaries: internal error!\n" );
		exit ( EXIT_FAILURE );	/* error return */
		break;
	}

	DEVA_cairo_add_text ( cairo_surface, TEXT_ROW_2, TEXT_COL,
		text_font_size, text_color, text );

	snprintf ( text, BUFF_SIZE, "matching score = %.3f",
		sum / ((double) count ) );
	DEVA_cairo_add_text ( cairo_surface, TEXT_ROW_3, TEXT_COL,
		text_font_size, text_color, text );
    }

    DEVA_RGB_cairo_close_inplace ( cairo_surface, hazards_visualization );

    if ( log_output_flag ) {
	log_file_fd = fopen ( log_output_filename, "a" );
	if ( log_file_fd == NULL ) {
	    perror ( log_output_filename );
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

#endif	/* DEVA_USE_CAIRO */
