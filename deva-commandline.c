/*
 * Command line interface for two related programs, the deva-filter simulation
 * of reduced acuity and contrast sensitivity and the deva-visibility
 * estimation of visibility hazards.
 */

/*
 * Define DEVA_VISIBILITY to generate command line version of deva-visibility.  
 * Leave undefined for command line version of deva-filter.
 *
 * This is currently handled in CMakeLists.txt.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>		/* for strcmp, strlen */
#include <strings.h>		/* for strcasecmp, strncasecmp */
#include <libgen.h>		/* for basename */
#include "deva-image.h"
#include "deva-filter.h"
#include "deva-utils.h"
#include "deva-margin.h"
#include "radianceIO.h"
#include "acuity-conversion.h"
#include "ChungLeggeCSF.h"
#ifdef DEVA_VISIBILITY
#include "read-geometry.h"
#include "deva-visibility.h"
#include "visualize-hazards.h"
#include "DEVA-png.h"
#endif	/* DEVA_VISIBILITY */
#include "deva-license.h"	/* DEVA open source license */

/* #define	UNIFORM_MARGINS */	/* make top/bottom and lef/right */
					/* margins all the same size */

#ifdef DEVA_VISIBILITY

/* The following need tuning!!! */

#define	POSITION_PATCH_SIZE		3
#define	ORIENTATION_PATCH_SIZE		3
#define	POSITION_THRESHOLD		2 /* cm */
#define	ORIENTATION_THRESHOLD		20 /* degrees */

#endif	/* DEVA_VISIBILITY */

char	*progname;	/* one radiance routine requires this as a global */

#ifndef DEVA_VISIBILITY

char	*Usage = /* deva-filter */
	    "--mild|--moderate|--severe|--profound [--margin=<value>]"
	    "\n\tinput.hdr output.hdr";
char	*Usage2 = "[--snellen|--logMAR] [--sensitivity-ratio|--pelli-robson]"
    "\n\t[--autoclip|--clip=<level>] [--color|--grayscale|saturation=<value>]"
    "\n\t[--margin=<value>] [--verbose] [--version] [--presets]"
	    "\n\t\tacuity contrast input.hdr output.hdr";
int	args_needed = 4;

/*
 * Options:
 *
 *   --mild | --moderate | --significant | --severe
 *
 *   		Specifies use of one of four predefined levels of acuity and
 *   		contrast deficits, along with a corresponding loss in color
 *   		sensitivity.  When used, the *acuity* and *contrast*
 *   		arguments are left off the command line.  Only one of these
 *   		flags can be specified.  When used, no additional flags other
 *   		than --margin=<value> or --verbose are allowed.
 *
 *   --Snellen | --logMAR
 *
 *   		Specifies the format of the *acuity* argument.  --Snellen is
 *   		the default.  Snellen ratios can be given as a single decimal
 *   		value or in the common "n/m" (in the U.S., usually "20/m")
 *   		fractional form.  logMAR values are given in the conventional
 *   		manner as a single number.
 *
 *   --cutoff | --peak
 *
 *   		Indicates that *acuity* is specified with respect to cutoff or
 *   		peak contrast sensitivity.  --cutoff is the default, and
 *   		corresponds to the values usually reported in clinical eye
 *   		examinations.
 *
 *   --sensitivity-ratio | --pelli-robson
 *
 *   		Indicates that *contrast* is specified as a ratio of desired
 *   		Michaelson contrast sensitivity to normal vision contrast
 *   		sensitivity (--sensitivity-ratio) or as a Pelli-Robson Chart
 *   		score (--pelli-robson).  --sensitivity-ratio is the default.

 *   --autoclip | --clip=<level>
 *
 *   		Large magnitude input luminance values are reduced to a lower
 *   		level.  This is most useful in reducing filtering artifacts due
 *   		to small, bright light sources.  For --autoclip, the level at
 *   		which values are clipped is chosen automatically.  For --clip,
 *   		values larger than <level> are reduced to <level>, with <level>
 *   		specified in cd/m^2.  Default is no clipping.
 *
 *   --color | --grayscale
 *
 *		Indicates that output image is either in color or grayscale.
 *		--color is the default.
 *
 *   --margin=<value>
 *
 *		Add a margin around the input file to reduce FFT artifacts due
 *		to top-bottom and left-right wraparound. <value> is a number
 *		in the range (0.0 -- 1.0].  It specifies the size of the
 *		horizontal and vertical padding as a fraction of the original
 *		horizontal and vertical size.
 *
 *   --version	Print version number and then exit.  No other flages or
 *		arguments are required.
 *
 *   --presets	Print out acuity, contrast sensitivity, and color saturation
 *		parameters associated with the presets --mild, --moderate,
 *		--severe, and --profound.  No other flags or arguments are
 *		required.
 *
 *   --verbose
 *		Print possibly informative information about a particular run.
 *
 * Arguments:
 *
 *   acuity	Acuity, in format as specified by --Snellen or --logMAR flags.
 *
 *   contrast	Contrast, in format as specified by --sensitivity-ratio or
 *   		--pelli-robson.
 *
 *   input.hdr	Radiance visible image file.  Must contain a VIEW record.
 *
 *   output.hdr	Low vision simulation.
 */

#else	/* DEVA_VISIBILITY */

char	*Usage = /* deva-visibility */
	    "--mild|--moderate|--severe|--profound [--margin=<value>]"
	    "\n\t[--luminanceboundaries=<filename.png>]"
	    "\n\t[--geometryboundaries=<filename.png>]"
	    "\n\tinput.hdr coordinates xyz.txt dist.txt nor.txt"
	    "\n\tsimulated-view.hdr hazards.png";
char	*Usage2 = "[--snellen|--logMAR] [--sensitivity-ratio|--pelli-robson]"
    "\n\t[--autoclip|--clip=<level>] [--color|--grayscale|saturation=<value>]"
    "\n\t[--margin=<value>] [--verbose] [--version] [--presets]"
    "\n\t[--luminanceboundaries=<filename.png>]"
    "\n\t[--geometryboundaries=<filename.png>]"
	    "\n\t\tacuity contrast input.hdr coordinates xyz.txt dist.txt"
	    "\n\t\tnor.txt simulated-view.hdr hazards.png";
int	args_needed = 9;

/*
 * Options:
 *
 *   All of the deva-filter options, plus:
 *
 *   --luminanceboundaries==<filename>.png
 *
 *		Write a grayscale PNG image file indicating the location of
 *		detected luminance boundaries.
 *
 *   --luminanceboundaries==<filename>.png
 *
 *		Write a grayscale PNG image file indicating the location of
 *		detected luminance boundaries.
 *
 * Arguments:
 *
 *   input.hdr	Original Radiance image of area in design model to be evaluated
 *		for low-vision visibility hazards, as for deva-filter.
 *
 *   coordinates
 *
 *		A two line text file.  The first line specifies the units for
 *		the xyz.txt and dist.txt files. The second line is the same as
 *		the VIEW record in input.hdr.  See make-coordinates-file for
 *		information on how to create this file.
 *
 *   xyz.txt	A Radiance ASCII format file specifying the xyz model
 *		coordinates for each surface point in the model corresponding
 *		to the line of sight associated with each pixel in input.hdr.
 *
 *   dist.txt	A Radiance ASCII format file specifying the distance from the
 *		viewpoint to each surface point in the model corresponding to
 *		the line of sight associated with each pixel in input.hdr.
 *
 *   nor.txt	A Radiance ASCII format file specifying the surface normal in
 *		model coordinates for each surface point in the model
 *		corresponding to the line of sight associated with each pixel
 *		in input.hdr.  Note that the numeric values are unitless since
 *		they specify a unit normal.
 *
 *   simulated-view.hdr
 *
 *		A Radiance image simulating the reduced visibility associated
 *		with loss of visual acuity and contrast sensitivity.
 *
 *   hazards.png
 *
 *		An output PNG image indicating likely potential visibility
 *		hazards.
 */

#endif	/* DEVA_VISIBILITY */

/* option flags */
typedef enum { no_preset, mild, moderate, severe, profound } PresetType;
typedef	enum { undefined_acuity_format, Snellen, logMAR } AcuityFormat;
typedef enum { undefined_sensitivity, sensitivity_ratio, pelli_robson }
							SensitivityType;
typedef enum { undefined_color, color, grayscale, saturation_value } ColorType;
typedef enum { undefined_clip, auto_clip, value_clip } ClipType;
/* hidden option flags */
typedef	enum { undefined_acuity_type, peak_sensitivity, cutoff }
							AcuityType;
typedef enum { undefined_smoothing, no_smoothing, smoothing } SmoothingType;

/* defaults: */
#define	DEFAULT_ACUITY_FORMAT		Snellen
#define	DEFAULT_ACUITY_FORMAT_STRING	"Snellen"
#define	DEFAULT_SENSITIVITY_TYPE	sensitivity_ratio
#define	DEFAULT_SENSITIVITY_TYPE_STRING	"sensitivity_ratio"
#define	DEFAULT_COLOR_TYPE		color
#define	DEFAULT_COLOR_TYPE_STRING	"color"
#define	DEFAULT_CLIP_TYPE		auto_clip
#define	DEFAULT_CLIP_TYPE_STRING	"auto_clip"
#define	DEFAULT_ACUITY_TYPE		cutoff
#define	DEFAULT_ACUITY_TYPE_STRING	"cutoff"
#define	DEFAULT_SMOOTHING_TYPE		smoothing
#define	DEFAULT_SMOOTHING_TYPE_STRING	"smoothing"
#define	DEFAULT_FILTER_TYPE		use_DEVA_filter
#define	DEFAULT_FILTER_TYPE_STRING	"use_DEVA_filter"

#define	DEFAULT_MEASUREMENT_TYPE	Gaussian_measure
#define	DEFAULT_SCALE_PARAMETER		0.5
#define	DEFAULT_VISUALIZATION_TYPE	red_gray_type;

/* preset values */

#ifndef DEVA_USE_OLD_PRESETS

#define	PRESET_MILD			"mild"
#define	MILD_SNELLEN			(20.0/45.0)
#define	MILD_LOGMAR			0.35
#define	MILD_PELLI_ROBSON		1.6
#define	MILD_SATURATION			.75

#define	PRESET_MODERATE			"moderate"
#define	MODERATE_SNELLEN		(20.0/115.0)
#define	MODERATE_LOGMAR			0.75
#define	MODERATE_PELLI_ROBSON		1.0
#define	MODERATE_SATURATION		.4

#define	PRESET_SEVERE			"severe"
#define	SEVERE_SNELLEN			(20.0/285.0)
#define	SEVERE_LOGMAR			1.15
#define	SEVERE_PELLI_ROBSON		0.75
#define	SEVERE_SATURATION		.25

#define	PRESET_PROFOUND			"profound"
#define	PROFOUND_SNELLEN		(20.0/710.0)
#define	PROFOUND_LOGMAR			1.55
#define	PROFOUND_PELLI_ROBSON		0.5
#define	PROFOUND_SATURATION		0.0

#else	/* DEVA_USE_OLD_PRESETS */

#define	MILD_SNELLEN			(20.0/80.0)	/* logMAR 0.6 */
#define	MILD_PELLI_ROBSON		1.5
#define	MILD_SATURATION			.75

#define	MODERATE_SNELLEN		(20.0/250.0)	/* logMAR 1.1 */
#define	MODERATE_PELLI_ROBSON		1.0
#define	MODERATE_SATURATION		.4

#define	SIGNIFICANT_SNELLEN		(20.0/450.0)	/* logMAR 1.35 */
#define	SIGNIFICANT_PELLI_ROBSON	0.75
#define	SIGNIFICANT_SATURATION		.25

#define	SEVERE_SNELLEN			(20.0/800.0)	/* logMAR 1.6 */
#define	SEVERE_PELLI_ROBSON		0.5
#define	SEVERE_SATURATION		0.0

#endif	/* DEVA_USE_OLD_PRESETS */

#define	LOGMAR_MAX		2.3
#define	LOGMAR_MIN		(-0.65)
#define	SNELLEN_MAX		4.0
#define	SNELLEN_MIN		0.005

#define	CONTRAST_RATIO_MIN	0.01
#define	CONTRAST_RATIO_MAX	2.5

#define	PELLI_ROBSON_MIN	0.0
#define	PELLI_ROBSON_MAX	2.4
#define PELLI_ROBSON_NORMAL	2.0	/* normal vision score on chart */

#define	AUTO_CLIP_MEDIAN		/* based auto_clip value on median, */
					/* not average */
#define	CUTOFF_RATIO_MEAN	7.0	/* RADIANCE identifies glare sources */
					/* as being brighter than 7 times the */
					/* average luminance level. */
#define	CUTOFF_RATIO_MEDIAN	12.0	/* need higher value for median */
#define	NO_CLIP_LEVEL		-1.0	/* don't clip values */

#define	exp10(x)	pow ( 10.0, (x) ) /* can't count on availablility
					     of exp10 */

static void	add_description_arguments ( DEVA_xyY_image *image, int argc,
		    char *argv[] );
static void	print_usage ( void );
static void	internal_error ( void );
static double	PelliRobson2contrastratio ( double PelliRobson_score );
static double	contrastratio2PelliRobson ( double contrast_ratio );
#ifndef	AUTO_CLIP_MEDIAN
static double	auto_clip_level ( DEVA_xyY_image *image );
#else
static double	auto_clip_level_median ( DEVA_xyY_image *image );
#endif	/* AUTO_CLIP_MEDIAN */
static void	clip_max_value ( DEVA_xyY_image *image, double clip_value );
static void	deva_filter_print_defaults ( void );
static void	deva_filter_print_presets ( void );

int
main ( int argc, char *argv[] )
{
    /* option flags */
    PresetType		preset_type = no_preset;
    AcuityFormat	acuity_format = undefined_acuity_format;
    SensitivityType	sensitivity_type = undefined_sensitivity;
    ColorType		color_type = undefined_color;
    ClipType		clip_type = undefined_clip;

    /* hidden option flags */
    AcuityType		acuity_type = undefined_acuity_type;
    SmoothingType	smoothing_type = undefined_smoothing;

    /* initialized next four variables to quiet uninitialized warning */
    double		saturation = -1.0;	/* Saturation adjustment: */
    						/* 0 => full desaturation */
						/* (0-1) => partial desat */
    						/* 1 => leave sat as is */
    double		clip_value = -1.0;
    double		acuity = -1.0;		/* ratio to normal */
    double		contrast_ratio = -1.0;	/* ratio to normal */
    int			smoothing_flag;		/* reduce banding artifacts */
    						/* due to thresholding */
    double		acuity_adjustment;	/* CSF peak adjustment */
    double		logMAR_arg;		/* used for sanity check */
    double		pelli_robson_score;	/* log contrast */
    double		margin = -1.0;		/* width of margin to add */
						/* to mitigate FFT */
						/* wraparound artificats */
    int			v_margin, h_margin;	/* in pixels */
    char		*input_file_name;

    DEVA_xyY_image	*input_image;		/* Y values in cd/m^2 */
    DEVA_xyY_image	*margin_image;		/* to deal with wrap-around */
    DEVA_xyY_image	*margin_filtered_image;	/* artifacts */
    char		*filtered_image_file_name;
    DEVA_xyY_image	*filtered_image;	/* Y values in cd/m^2 */

#ifdef DEVA_VISIBILITY
    char		*coordinates_file_name;
    char		*xyz_file_name;
    char		*dist_file_name;
    char		*nor_file_name;
    char		*hazards_file_name;
    char		*luminance_boundaries_file_name = NULL;
    char		*geometry_boundaries_file_name = NULL;
    DEVA_coordinates	*coordinates;
    DEVA_XYZ_image	*xyz;
    DEVA_float_image	*dist;
    DEVA_XYZ_image	*nor;
    /* hidden options */
    Measurement_type	measurement_type = DEFAULT_MEASUREMENT_TYPE;
    double		scale_parameter = DEFAULT_SCALE_PARAMETER;
    Visualization_type	visualization_type = DEFAULT_VISUALIZATION_TYPE;

    /* Hardwired parameters for detection of geometry boundaries. */
    int			position_patch_size = POSITION_PATCH_SIZE;
    int			orientation_patch_size = ORIENTATION_PATCH_SIZE;
    int			position_threshold = POSITION_THRESHOLD;
    int			orientation_threshold = ORIENTATION_THRESHOLD;

    DEVA_float_image	*hazards;	/* Visual angle between geometry */
    					/* boundaries and nearest low vision */
    					/* luminance boundaries. */
    DEVA_RGB_image	*hazards_visualization;
    					/* Displayable image indicating */
    					/* potentially invisible geometry */
    					/* boundaries in red.  Optionally, */
    					/* geometry boundaries predicted to */
    					/* be visible can be marked in green. */
#endif	/* DEVA_VISIBILITY */

    int			argpt = 1;

    progname = basename ( argv[0] );

    /* scan and collect option flags */
    while ( ( ( argc - argpt ) >= 1 ) && ( argv[argpt][0] == '-' ) ) {
	if ( strcmp ( argv[argpt], "-" ) == 0 ) {
	    break;	/* read/write from/to stdin/stdout? */
	}

	if ( ( strcasecmp ( argv[argpt], "--mild" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-mild" ) == 0 ) ) {
	    /* preset for mild loss of acuity and contrast */
	    if ( ! ( ( preset_type == no_preset ) ||
			( preset_type == mild ) ) ) {
		fprintf ( stderr, "conflicting preset values!\n" );
		exit ( EXIT_FAILURE );
	    }
	    preset_type = mild;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--moderate" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-moderate" ) == 0 ) ) {
	    /* preset for moderate loss of acuity and contrast */
	    if ( ! ( ( preset_type == no_preset ) ||
			( preset_type == moderate ) ) ) {
		fprintf ( stderr, "conflicting preset values!\n" );
		exit ( EXIT_FAILURE );
	    }
	    preset_type = moderate;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--severe" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-severe" ) == 0 ) ) {
	    /* preset for severe loss of acuity and contrast */
	    if ( ! ( ( preset_type == no_preset ) ||
			( preset_type == severe ) ) ) {
		fprintf ( stderr, "conflicting preset values!\n" );
		exit ( EXIT_FAILURE );
	    }
	    preset_type = severe;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--profound" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-profound" ) == 0 ) ) {
	    /* preset for profound loss of acuity and contrast */
	    if ( ! ( ( preset_type == no_preset ) ||
			( preset_type == profound ) ) ) {
		fprintf ( stderr, "conflicting preset values!\n" );
		exit ( EXIT_FAILURE );
	    }
	    preset_type = profound;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--Snellen" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-Snellen" ) == 0 ) ) {
	    /* acuity specified as Snellen fraction or Snellen decimal */
	    if ( acuity_format == logMAR ) {
		fprintf ( stderr, "conflicting --Snellen/--logMAR!\n" );
		exit ( EXIT_FAILURE );
	    }
	    acuity_format = Snellen;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--logMAR" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-logMAR" ) == 0 ) ) {
	    /* acuity specified as logMAR value */
	    if ( acuity_format == Snellen ) {
		fprintf ( stderr, "conflicting --Snellen/--logMAR!\n" );
		exit ( EXIT_FAILURE );
	    }
	    acuity_format = logMAR;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--sensitivity-ratio" ) == 0 )
		||
		( strcasecmp ( argv[argpt], "-sensitivity-ratio" ) == 0 ) ) {
	    /* contrast sensitivity specifed as ratio to normal */
	    if ( sensitivity_type == pelli_robson ) {
		fprintf ( stderr,
			"conflicting --sensitivity-ratio/--pelli_robson!\n" );
		exit ( EXIT_FAILURE );
	    }
	    sensitivity_type = sensitivity_ratio;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--pelli-robson" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-pelli-robson" ) == 0 ) ) {
	    /* contrast sensitivity specifed as Pelli-Robson score */
	    if ( sensitivity_type == sensitivity_ratio ) {
		fprintf ( stderr,
			"conflicting --sensitivity-ratio/--pelli_robson!\n" );
		exit ( EXIT_FAILURE );
	    }
	    sensitivity_type = pelli_robson;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--color" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-color" ) == 0 ) ) {
	    /* output as filtered color image */
	    if ( ! ( ( color_type == undefined_color ) ||
			( color_type == color ) ) ) {
		fprintf ( stderr,
			"can't mix --color, --grayscale, and --saturation!\n" );
		exit ( EXIT_FAILURE );
	    }
	    color_type = color;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--grayscale" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-grayscale" ) == 0 ) ) {
	    /* output as filtered grayscale image */
	    if ( ! ( ( color_type == undefined_color ) ||
			( color_type == grayscale ) ) ) {
		fprintf ( stderr,
			"can't mix --color, --grayscale, and --saturation!\n" );
		exit ( EXIT_FAILURE );
	    }
	    color_type = grayscale;
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "--saturation=",
		    strlen ( "--saturation=" ) ) == 0 ) {
	    /* scale output saturation by specified value */
	    if ( ! ( ( color_type == undefined_color ) ||
			( color_type == saturation_value ) ) ) {
		fprintf ( stderr,
			"can't mix --color, --grayscale, and --saturation!\n" );
		exit ( EXIT_FAILURE );
	    }
	    if ( color_type == saturation_value ) {
		fprintf ( stderr, "multiple --saturation=<value> flags!\n" );
		exit ( EXIT_FAILURE );
	    }
	    color_type = saturation_value;
	    saturation = atof ( argv[argpt] + strlen ( "--saturation=" ) );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "-saturation=",
		    strlen ( "-saturation=" ) ) == 0 ) {
	    /* scale output saturation by specified value */
	    if ( ! ( ( color_type == undefined_color ) ||
			( color_type == saturation_value ) ) ) {
		fprintf ( stderr,
			"can't mix --color, --grayscale, and --saturation!\n" );
		exit ( EXIT_FAILURE );
	    }
	    if ( color_type == saturation_value ) {
		fprintf ( stderr, "multiple --saturation=<value> flags!\n" );
		exit ( EXIT_FAILURE );
	    }
	    color_type = saturation_value;
	    saturation = atof ( argv[argpt] + strlen ( "-saturation=" ) );
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--autoclip" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-autoclip" ) == 0 ) ) {
	    /* pick a clip value automatically */
	    if ( clip_type == value_clip ) {
		fprintf ( stderr,
			"can't mix --clip=<value> and --autoclip!\n" );
		exit ( EXIT_FAILURE );
	    }
	    clip_type = auto_clip;
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "--clip=",
		    strlen ( "--clip=" ) ) == 0 ) {
	    /* clip input to specified value (cd/m^2) */
	    if ( clip_type == auto_clip ) {
		fprintf ( stderr,
			"can't mix --clip=<value> and --autoclip!\n" );
		exit ( EXIT_FAILURE );
	    }
	    if ( clip_type == value_clip ) {
		fprintf ( stderr, "multiple --clip=<value> flags!\n" );
		exit ( EXIT_FAILURE );
	    }
	    clip_type = value_clip;
	    clip_value = atof ( argv[argpt] + strlen ( "--clip=" ) );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "-clip=",
		    strlen ( "-clip=" ) ) == 0 ) {
	    /* clip input to specified value (cd/m^2) */
	    if ( clip_type == auto_clip ) {
		fprintf ( stderr,
			"can't mix -clip=<value> and -autoclip!\n" );
		exit ( EXIT_FAILURE );
	    }
	    if ( clip_type == value_clip ) {
		fprintf ( stderr, "multiple --clip=<value> flags!\n" );
		exit ( EXIT_FAILURE );
	    }
	    clip_type = value_clip;
	    clip_value = atof ( argv[argpt] + strlen ( "-clip=" ) );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "--margin=",
		    strlen ( "--margin=" ) ) == 0 ) {
	    margin = atof ( argv[argpt] + strlen ( "--margin=" ) );
	    if ( ( margin < 0.0 ) || ( margin > 1.0 ) ) {
		printf ( "margin (%f) must be in range [0.0 -- 1.0]!\n",
			margin );
		exit ( EXIT_FAILURE );
	    }
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "-margin=",
		    strlen ( "-margin=" ) ) == 0 ) {
	    margin = atof ( argv[argpt] + strlen ( "-margin=" ) );
	    if ( ( margin < 0.0 ) || ( margin > 1.0 ) ) {
		printf ( "margin (%f) must be in range [0.0 -- 1.0]!\n",
			margin );
		exit ( EXIT_FAILURE );
	    }
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--version" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-version" ) == 0 ) ) {
	    /* print version number then exit */
	    deva_filter_print_version ( );
	    /* argpt++; */
	    exit ( EXIT_FAILURE );

	} else if ( ( strcasecmp ( argv[argpt], "--presets" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-presets" ) == 0 ) ) {
	    /* print presets number then exit */
	    deva_filter_print_presets ( );
	    /* argpt++; */
	    exit ( EXIT_FAILURE );

	} else if ( ( strcasecmp ( argv[argpt], "--defaults" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-defaults" ) == 0 ) ) {
	    /* print default values then exit */
	    deva_filter_print_defaults ( );
	    /* argpt++; */
	    exit ( EXIT_FAILURE );

	} else if ( ( strcasecmp ( argv[argpt], "--verbose" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-verbose" ) == 0 ) ) {
	    /* print out information potentially useful to users */
	    DEVA_verbose = TRUE;
	    argpt++;

	    /* "hidden" options: */
	} else if ( ( strcasecmp ( argv[argpt], "--veryverbose" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-veryverbose" ) == 0 ) ) {
	    /* print out debugging information */
	    DEVA_verbose = TRUE;
	    DEVA_veryverbose = TRUE;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--peak" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-peak" ) == 0 ) ) {
	    /* adjust acuity by shifting peak of CSF */
	    if ( acuity_type == cutoff ) {
		fprintf ( stderr, "conflicting --peak/--cutoff!\n" );
		exit ( EXIT_FAILURE );
	    }
	    acuity_type = peak_sensitivity;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--cutoff" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-cutoff" ) == 0 ) ) {
	    /* adjust acuity by shifting high-frequency cutoff of CSF */
	    if ( acuity_type == peak_sensitivity ) {
		fprintf ( stderr, "conflicting --peak/--cutoff!\n" );
		exit ( EXIT_FAILURE );
	    }
	    acuity_type = cutoff;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--smooth" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-smooth" ) == 0 ) ) {
	    /* smooth thresholded contrast bands */
	    if ( smoothing_type == no_smoothing ) {
		fprintf ( stderr, "conflicting --smooth/-nosmooth!\n" );
		exit ( EXIT_FAILURE );
	    }
	    smoothing_type = smoothing;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--nosmooth" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-nosmooth" ) == 0 ) ) {
	    /* don't smooth thresholded contrast bands */
	    if ( smoothing_type == smoothing ) {
		fprintf ( stderr, "conflicting --smooth/-nosmooth!\n" );
		exit ( EXIT_FAILURE );
	    }
	    smoothing_type = no_smoothing;
	    argpt++;

	} else if ( ( strcasecmp ( argv[argpt], "--CSF-parms" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-CSF-parms" ) == 0 ) ) {
	    /* print CSF parameters then exit */
	    ChungLeggeCSF_print_parms ( );
	    /* argpt++; */
	    exit ( EXIT_FAILURE );

#ifdef DEVA_VISIBILITY

	} else if ( strncasecmp ( argv[argpt], "--luminanceboundaries=",
		    strlen ( "--luminanceboundaries=" ) ) == 0 ) {
	    luminance_boundaries_file_name = argv[argpt] +
		strlen ( "--luminanceboundaries=" );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "-luminanceboundaries=",
		    strlen ( "-luminanceboundaries=" ) ) == 0 ) {
	    luminance_boundaries_file_name = argv[argpt] +
		strlen ( "-luminanceboundaries=" );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "--geometryboundaries=",
		    strlen ( "--geometryboundaries=" ) ) == 0 ) {
	    geometry_boundaries_file_name = argv[argpt] +
		strlen ( "--geometryboundaries=" );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "-geometryboundaries=",
		    strlen ( "-geometryboundaries=" ) ) == 0 ) {
	    geometry_boundaries_file_name = argv[argpt] +
		strlen ( "-geometryboundaries=" );
	    argpt++;

	} else if ( strncasecmp ( argv[argpt], "--reciprocal=",
		    strlen ( "--reciprocal=" ) ) == 0 ) {
	    measurement_type = reciprocal_measure;
	    scale_parameter = atof ( argv[argpt] +
		    strlen ( "--reciprocal=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-reciprocal=",
		    strlen ( "-reciprocal=" ) ) == 0 ) {
	    measurement_type = reciprocal_measure;
	    scale_parameter = atof ( argv[argpt] +
		    strlen ( "-reciprocal=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "--linear=",
		    strlen ( "--linear=" ) ) == 0 ) {
	    measurement_type = linear_measure;
	    scale_parameter = atof ( argv[argpt] + strlen ( "--linear=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-linear=",
		    strlen ( "-linear=" ) ) == 0 ) {
	    measurement_type = linear_measure;
	    scale_parameter = atof ( argv[argpt] + strlen ( "-linear=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "--Gaussian=",
		    strlen ( "--Gaussian=" ) ) == 0 ) {
	    measurement_type = Gaussian_measure;
	    scale_parameter = atof ( argv[argpt] + strlen ( "--Gaussian=" ) );
	    argpt++;
	} else if ( strncasecmp ( argv[argpt], "-Gaussian=",
		    strlen ( "-Gaussian=" ) ) == 0 ) {
	    measurement_type = Gaussian_measure;
	    scale_parameter = atof ( argv[argpt] + strlen ( "-Gaussian=" ) );
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--red-green" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-red-green" ) == 0 ) ) {
	    visualization_type = red_green_type;
	    argpt++;
	} else if ( ( strcasecmp ( argv[argpt], "--red-gray" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-red-gray" ) == 0 ) ) {
	    visualization_type = red_gray_type;
	    argpt++;

#endif	/* DEVA_VISIBILITY */

	} else {
	    fprintf ( stderr, "%s: invalid flag (%s)!\n", progname,
		    argv[argpt] );
	    return ( EXIT_FAILURE );    /* error exit */
	}
    }

    if ( preset_type != no_preset ) {
	if ( ( acuity_format != undefined_acuity_format ) ||
		( sensitivity_type != undefined_sensitivity ) ||
		( color_type != undefined_color ) ||
		( clip_type != undefined_clip ) ) {
	    fprintf ( stderr, "can't mix other arguements with preset!\n" );
	    return ( EXIT_FAILURE );	/* error return */
	}
	args_needed -= 2;	/* no acuity or contrast sensitivity values */

	switch ( preset_type ) {

	    case mild:

		acuity_format = Snellen;
		acuity = MILD_SNELLEN;

		sensitivity_type = pelli_robson;
		contrast_ratio =
		    PelliRobson2contrastratio ( MILD_PELLI_ROBSON );

		if ( MILD_SATURATION == 1.0 ) {
		    color_type = color;
		} else if ( MILD_SATURATION == 0.0 ) {
		    color_type = grayscale;
		} else {
		    color_type = saturation_value;
		}
		saturation = MILD_SATURATION;

		clip_type = auto_clip;
		acuity_type = cutoff;
		smoothing_type = smoothing;

		break;

	    case moderate:

		acuity_format = Snellen;
		acuity = MODERATE_SNELLEN;

		sensitivity_type = pelli_robson;
		contrast_ratio =
		    PelliRobson2contrastratio ( MODERATE_PELLI_ROBSON );

		if ( MODERATE_SATURATION == 1.0 ) {
		    color_type = color;
		} else if ( MODERATE_SATURATION == 0.0 ) {
		    color_type = grayscale;
		} else {
		    color_type = saturation_value;
		}
		saturation = MODERATE_SATURATION;

		clip_type = auto_clip;
		acuity_type = cutoff;
		smoothing_type = smoothing;

		break;

	    case severe:

		acuity_format = Snellen;
		acuity = SEVERE_SNELLEN;

		sensitivity_type = pelli_robson;
		contrast_ratio =
		    PelliRobson2contrastratio ( SEVERE_PELLI_ROBSON );

		if ( SEVERE_SATURATION == 1.0 ) {
		    color_type = color;
		} else if ( SEVERE_SATURATION == 0.0 ) {
		    color_type = grayscale;
		} else {
		    color_type = saturation_value;
		}
		saturation = SEVERE_SATURATION;

		clip_type = auto_clip;
		acuity_type = cutoff;
		smoothing_type = smoothing;

		break;

	    case profound:

		acuity_format = Snellen;
		acuity = PROFOUND_SNELLEN;

		sensitivity_type = pelli_robson;
		contrast_ratio =
		    PelliRobson2contrastratio ( PROFOUND_PELLI_ROBSON );

		if ( PROFOUND_SATURATION == 1.0 ) {
		    color_type = color;
		} else if ( PROFOUND_SATURATION == 0.0 ) {
		    color_type = grayscale;
		} else {
		    color_type = saturation_value;
		}
		saturation = PROFOUND_SATURATION;

		clip_type = auto_clip;
		acuity_type = cutoff;
		smoothing_type = smoothing;

		break;

	    default:

		internal_error ( );

		break;
	}
    }

    if ( ( argc - argpt ) != args_needed ) {
	print_usage ( );
	return ( EXIT_FAILURE );	/* error return */
    }

    /* Use defaults when appropriate */

    if ( acuity_format == undefined_acuity_format ) {
	acuity_format = DEFAULT_ACUITY_FORMAT;
    }

    if ( sensitivity_type == undefined_sensitivity ) {
	sensitivity_type = DEFAULT_SENSITIVITY_TYPE;
    }

    if ( color_type == undefined_color ) {
	color_type = DEFAULT_COLOR_TYPE;
    }

    if ( clip_type == undefined_clip ) {
	clip_type = DEFAULT_CLIP_TYPE;
    }

    if ( acuity_type == undefined_acuity_type ) {
	acuity_type = DEFAULT_ACUITY_TYPE;
    }

    if ( smoothing_type == undefined_smoothing ) {
	smoothing_type = DEFAULT_SMOOTHING_TYPE;
    }

    /* error checking */

    if ( color_type == saturation_value ) {
	if ( ( saturation < 0.0 ) || ( saturation > 1.0 ) ) {
	    fprintf ( stderr, "invalid saturation value (%f)!\n", saturation );
	    exit ( EXIT_FAILURE );
	}
	if ( DEVA_verbose ) {
	    fprintf ( stderr, "saturation set to %.2f\n", saturation );
	}
    }

    if ( clip_type == value_clip ) {
	if ( clip_value <= 0.0 ) {
	    fprintf ( stderr, "invalid clip value (%f)!\n", clip_value );
	    exit ( EXIT_FAILURE );
	}
	if ( DEVA_verbose ) {
	    fprintf ( stderr, "luminance values clipped to <= %.2f\n",
		    clip_value );
	}
    }

    /* color setting */

    switch ( color_type ) {

	case color:

	    saturation = 1.0;	/* full color */

	    break;

	case grayscale:

	    saturation = 0.0;	/* no color */

	    break;

	case saturation_value:

	    /* should already be set */

	    break;

	case undefined_color:
	default:

	    internal_error ( );

	    break;
    }


    /* boolean flag setting */

    switch ( smoothing_type ) {

	case no_smoothing:

	    smoothing_flag = FALSE;

	    if ( DEVA_verbose ) {
		fprintf ( stderr, "threshold smoothing disabled\n" );
	    }

	    break;

	case smoothing:

	    smoothing_flag = TRUE;

	    if ( DEVA_verbose ) {
		fprintf ( stderr, "threshold smoothing enabled\n" );
	    }

	    break;

	case undefined_smoothing:
	default:

	    internal_error ( );

	    break;
    }

    /* get acuity and contrast if not specified using a preset */

    if ( preset_type == no_preset ) {

	switch ( acuity_format ) {

	    case Snellen:
		acuity = parse_snellen ( argv[argpt++] );
		if ( ( acuity > SNELLEN_MAX ) || ( acuity < SNELLEN_MIN ) ) {
		    fprintf ( stderr, "implausible Snellen value (%s)!\n",
			    argv[argpt-1] );
		    exit ( EXIT_FAILURE );
		}

		break;

	    case logMAR:

		logMAR_arg = atof ( argv[argpt++] );
		if ( ( logMAR_arg > LOGMAR_MAX ) ||
			( logMAR_arg < LOGMAR_MIN ) ) {
		    fprintf ( stderr, "implausible logMAR value (%f)!\n",
			    logMAR_arg );
		    exit ( EXIT_FAILURE );
		}
		acuity = logMAR_to_Snellen_decimal ( logMAR_arg );

		break;

	    case undefined_acuity_format:
	    default:

		internal_error ( );

		break;
	}

	switch ( sensitivity_type ) {

	    case sensitivity_ratio:

		contrast_ratio = atof ( argv[argpt++] );
		if ( ( contrast_ratio > CONTRAST_RATIO_MAX ) || 
			( contrast_ratio < CONTRAST_RATIO_MIN ) ) {
		    fprintf ( stderr, "implausible contrast ratio (%f)!\n",
			    contrast_ratio );
		    exit ( EXIT_FAILURE );
		}

		break;

	    case pelli_robson:

		pelli_robson_score = atof ( argv[argpt++] );
		if ( ( pelli_robson_score > PELLI_ROBSON_MAX ) || 
			( pelli_robson_score < PELLI_ROBSON_MIN ) ) {
		    fprintf ( stderr, "implausible Pelli-Robson score (%f)!\n",
			    pelli_robson_score );
		    exit ( EXIT_FAILURE );
		}

		contrast_ratio =
		    PelliRobson2contrastratio ( pelli_robson_score );

		break;

	    case undefined_sensitivity:
	    default:

		internal_error ( );

		break;
	}
    }

    /* fine name of original Radiance file */
    input_file_name = argv[argpt++];

#ifdef DEVA_VISIBILITY

    /* file names of geometry files */
    coordinates_file_name = argv[argpt++];
    xyz_file_name = argv[argpt++];
    dist_file_name = argv[argpt++];
    nor_file_name = argv[argpt++];

#endif	/* DEVA_VISIBILITY */

    /* file name of output simulated low vision file */
    filtered_image_file_name = argv[argpt++];

#ifdef DEVA_VISIBILITY

    /* file name of output prediction of difficult-to-see geometry */
    hazards_file_name = argv[argpt++];

#endif	/* DEVA_VISIBILITY */

    if ( DEVA_verbose ) {
	/* needs to go to stderr in case output is sent to stdout */
	fprintf ( stderr, "acuity = 20/%d (logMar %.2f)",
	    (int) round ( Snellen_decimal_to_Snellen_denominator ( acuity ) ),
		Snellen_decimal_to_logMAR ( acuity ) );
	if ( acuity_type == peak_sensitivity ) {
	    fprintf ( stderr, " wrt peak" );
	}
	printf ( "\n" );

	fprintf ( stderr,
		"contrast sensitivity ratio = %.2f (Pelli-Robson score %.2f)\n",
		contrast_ratio, contrastratio2PelliRobson ( contrast_ratio ) );
    }

    if ( acuity_type == cutoff ) {
	acuity_adjustment = ChungLeggeCSF_cutoff_acuity_adjust ( acuity,
		contrast_ratio );
	if ( DEVA_verbose ) {
	    if ( contrast_ratio != 1.0 ) {
		fprintf ( stderr,
			"adjusting peak sensitivity frequency ratio to %.2f\n",
			acuity_adjustment );
	    }
	}
    } else {
	acuity_adjustment = acuity;
    }

    input_image = DEVA_xyY_image_from_radfilename ( input_file_name );
    	/*
	 * DEVA_xyY_image_from_radfilename copies VIEW record from Radiance
	 * .hdr file to input_image object.
	 */

    switch ( clip_type ) {

	case auto_clip:

#ifndef	AUTO_CLIP_MEDIAN
	    clip_value = auto_clip_level ( input_image );
#else
	    clip_value = auto_clip_level_median ( input_image );
#endif	/* AUTO_CLIP_MEDIAN */
	    if ( clip_value >= 0.0 ) {
		clip_max_value ( input_image, clip_value );

		if ( DEVA_verbose ) {
		    fprintf ( stderr, "autoclipped to <= %.2f\n", clip_value );
		}
	    } else if ( DEVA_verbose ) {
		fprintf ( stderr, "autoclip: clipping not needed\n" );
	    }

	    break;

	case value_clip:

	    clip_max_value ( input_image, clip_value );

	    break;

	case undefined_clip:
	default:

	    internal_error ( );

	    break;
    }

    if ( margin > 0.0 ) {
	/*
	 * Add margin around input image to reduce problems with top-bottom
	 * and left-right wraparound artifacts.
	 */

	/* margin sizes in pixels */
	v_margin = (int) round ( 0.5 * margin *
		DEVA_image_n_rows ( input_image ) );
	h_margin = (int) round ( 0.5 * margin *
		DEVA_image_n_cols ( input_image ) );

#ifdef UNIFORM_MARGINS
	/* make the margin based only on the smaller dimension */
	if ( v_margin > h_margin ) {
	    v_margin = h_margin;
	} else {
	    h_margin = v_margin;
	}
#endif	/* UNIFORM_MARGINS */

	/* add the margin */
	margin_image =  DEVA_xyY_add_margin ( v_margin, h_margin, input_image );

	/* filter the padded image */
	margin_filtered_image = deva_filter ( margin_image,
		acuity_adjustment, contrast_ratio, smoothing_flag,
		saturation );

	if ( DEVA_veryverbose ) {
	    fprintf ( stderr,
		    "deva_filter ( %s, %.4f, %.4f, %d, %.2f )\n",
		    input_file_name, acuity_adjustment, contrast_ratio,
		    smoothing_flag, saturation );
	}

	/* strip the padding back off */
	filtered_image = DEVA_xyY_strip_margin ( v_margin, h_margin,
		margin_filtered_image );

	/* clean up */
	DEVA_xyY_image_delete ( margin_image );
	DEVA_xyY_image_delete ( margin_filtered_image );

    } else {

	/* filter the unpadded image */
	filtered_image = deva_filter ( input_image, acuity_adjustment,
		contrast_ratio, smoothing_flag, saturation );

	if ( DEVA_veryverbose ) {
	    fprintf ( stderr,
		    "deva_filter ( %s, %.4f, %.4f, %d, %.2f )\n",
		    input_file_name, acuity_adjustment, contrast_ratio,
		    smoothing_flag, saturation );
	}
    }

    /* add command line to description */
    add_description_arguments ( filtered_image, argc, argv );

    /* output radiance file */
    DEVA_xyY_image_to_radfilename ( filtered_image_file_name, filtered_image );

    /* clean up */
    DEVA_xyY_image_delete ( input_image );

#ifdef DEVA_VISIBILITY

    /* read in geometry files */
    coordinates = DEVA_coordinates_from_filename ( coordinates_file_name );
    xyz = DEVA_geom3d_from_radfilename ( xyz_file_name );
    dist = DEVA_geom1d_from_radfilename ( dist_file_name );
    nor = DEVA_geom3d_from_radfilename ( nor_file_name );

    /* standardize distances (to cm) */
    standard_units_1D ( dist, coordinates );
    standard_units_3D ( xyz, coordinates );

    /*
     * Compute visual angle from geometry boundaries to nearest luminance
     * boundary.
     */
    hazards = deva_visibility ( filtered_image, coordinates, xyz, dist, nor,
	    position_patch_size, orientation_patch_size,
	    position_threshold, orientation_threshold,
	    luminance_boundaries_file_name, geometry_boundaries_file_name );

    /* clean up */
    DEVA_coordinates_delete ( coordinates );
    DEVA_XYZ_image_delete ( xyz );
    DEVA_float_image_delete ( dist );
    DEVA_XYZ_image_delete ( nor );

    /*
     * Make displayable file showing predicted geometry discontinuities that
     * are not visible at specified level of low vision.
     */
    hazards_visualization =
	visualize_hazards ( hazards, measurement_type, scale_parameter,
		visualization_type );

    DEVA_RGB_image_to_filename_png ( hazards_file_name, hazards_visualization );

    /* clean up */
    DEVA_float_image_delete ( hazards );
    DEVA_RGB_image_delete ( hazards_visualization );

#endif	/* DEVA_VISIBILITY */

    /* clean up */
    DEVA_xyY_image_delete ( filtered_image );

    return ( EXIT_SUCCESS );	/* normal exit */
}

static void
add_description_arguments ( DEVA_xyY_image *image, int argc, char *argv[] )
/*
 * Add current command line to description.
 */
{
    int	    argpt;

    if ( argc <= 0 ) {
	fprintf ( stderr, "add_description_arguments: invalid argc (%d)!\n",
		argc );
	exit ( EXIT_FAILURE );
    }

    for ( argpt = 0; argpt < argc; argpt++ ) {
	if ( argpt != 0 ) {
	    DEVA_image_description ( image ) =
		strcat_safe ( DEVA_image_description ( image ), " " );
	}
	DEVA_image_description ( image ) =
	    /* strcat_safe makes a copy that is guaranteed to be big enough */
	    strcat_safe ( DEVA_image_description ( image ), argv[argpt] );
    }

    DEVA_image_description ( image ) =
	strcat_safe ( DEVA_image_description ( image ), "\n" );
}

static void
print_usage ( void )
{
    fprintf ( stderr, "%s %s\n", progname, Usage );
    fprintf ( stderr, "\t\t\tor\n" );
    fprintf ( stderr, "%s %s\n", progname, Usage2 );
}

static void
internal_error ( void )
{
    fprintf ( stderr, "%s: internal error!\n", progname );
    exit ( EXIT_FAILURE );
}

static double
PelliRobson2contrastratio ( double PelliRobson_score )
/*
 * Converts Pelli-Robson Chart score to ratio of desired peak Michelson
 * contrast sensitivity to nominal normal vision peak contrast sensitivity.
 */
{
    double  requested_weber;	    /* low vision Weber contrast limit */
    double  requested_michelson;    /* low vision Michelson contrast limit */
    double  requested_sensitivity;  /* requested Michelson contrast, */
				    /* converted to sensitivity */
    double  normal_weber;	    /* normal vision Weber contrast limit */
    double  normal_michelson;	    /* normal vision Michelson contrast limit */
    double  normal_sensitivity;	    /* normal vision Michelson contrast */
    				    /* sensitivity */
    double  contrast_ratio;	    /* ratio or requested Michelson */
    				    /* sensitivity to normal sensitivity */

    requested_weber = exp10 ( -PelliRobson_score );
    requested_michelson = -requested_weber / ( requested_weber - 2.0 );
    if ( requested_michelson <= 0.0 ) {
	fprintf ( stderr, "PelliRobson2contrastratio: invalid contrast (%f)\n",
		requested_michelson );
	exit ( EXIT_FAILURE );
    }
    requested_sensitivity = 1.0 / requested_michelson;

    normal_weber = exp10 ( -PELLI_ROBSON_NORMAL );
    normal_michelson = -normal_weber / ( normal_weber - 2.0 );
    normal_sensitivity = 1.0 / normal_michelson;

    contrast_ratio = requested_sensitivity / normal_sensitivity;

    return ( contrast_ratio );
}

static double
contrastratio2PelliRobson ( double contrast_ratio )
/*
 * Converts ratio of low vision Michelson sensitivity to normal vision
 * Micheson sensitivity to equivalent Pelli-Robson Chart score.
 */
{
    double  normal_weber;	/* normal vision Weber contrast limit */
    double  normal_michelson;	/* normal vision Michelson contrast limit */
    double  requested_michelson;/* low vision Michelson contrast limit */
    double  requested_weber;	/* low vision Michelson contrast limit */
    double  PelliRobson_score;	/* low vision Pelli-Robson score */

    normal_weber = exp10 ( -PELLI_ROBSON_NORMAL );
    normal_michelson = -normal_weber / ( normal_weber - 2.0 );
    requested_michelson = normal_michelson / contrast_ratio;
    requested_weber =
	( 2.0 * requested_michelson ) / ( requested_michelson + 1.0 );

    PelliRobson_score = -log10 ( requested_weber );

    return ( PelliRobson_score );
}

#ifndef AUTO_CLIP_MEDIAN
static double
auto_clip_level ( DEVA_xyY_image *image )
/*
 * Suggests a clip level to apply to extreamly bright pixels to reduce
 * filter ringing.
 *
 * Uses a variant of the RADIANCE glare identification heuristic.  First,
 * average luminance is computed and used to set a preliminary glare
 * threshold value.  This average is not a robust estimator, since it is
 * strongly affected by very bright glare pixels or glare pixels covering
 * a large portion of the image.  To compensate for this, a second pass
 * is done in which a revised average luminance is computed based only on
 * pixels <= the preliminary glare threshold.  This revised average luminance
 * is then used to compute a revised glare threshold, which is returned as
 * the value of the function.
 *
 * NO_CLIP_LEVEL is returned if no clipping is needed.
 */
{
    int		    row, col;
    double	    max_luminance;
    double	    average_luminance_initial;
    double	    average_luminance_revised;
    double	    cutoff_initial;
    double	    cutoff_revised;
    unsigned int    glare_count;

    /* first pass */

    max_luminance = average_luminance_initial = 0.0;

    for ( row = 0; row < DEVA_image_n_rows ( image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( image ); col++ ) {
	    if ( max_luminance < DEVA_image_data (image, row, col ) . Y ) {
		max_luminance = DEVA_image_data (image, row, col ) . Y;
	    }

	    average_luminance_initial += DEVA_image_data (image, row, col ) . Y;
	}
    }

    average_luminance_initial /=
	( ((double) DEVA_image_n_rows ( image ) ) *
	    ((double) DEVA_image_n_cols ( image ) ) );
    cutoff_initial = CUTOFF_RATIO_MEAN * average_luminance_initial;

    if ( cutoff_initial >= max_luminance ) {
	/* no need for glare source clipping */
	return ( NO_CLIP_LEVEL );
    }

    /* second pass */

    average_luminance_revised = 0.0;
    glare_count = 0;

    for ( row = 0; row < DEVA_image_n_rows ( image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( image ); col++ ) {
	    if ( DEVA_image_data ( image, row, col ) . Y >
		    cutoff_initial ) {
		glare_count++;
	    } else {
		average_luminance_revised +=
		    DEVA_image_data (image, row, col) . Y;
	    }
	}
    }

    average_luminance_revised /=
	( ( ((double) DEVA_image_n_rows ( image ) ) *
	    ((double) DEVA_image_n_cols ( image ) ) ) -
	  ( (double) glare_count ) );
    cutoff_revised = CUTOFF_RATIO_MEAN * average_luminance_revised;

    /* printf ( "clip_level = %f\n", cutoff_revised ); */

    return ( cutoff_revised );
}

#else

#define	MAGNITUDE_HIST_NBINS	1000
#define	MEDIAN_EPSILON		0.0001	/* Used to avoid a boundary condition */
					/* computing percentile bin numbers. */

static double
auto_clip_level_median ( DEVA_xyY_image *image )
/*
 * Suggests a clip level to apply to extreamly bright pixels to reduce
 * filter ringing.
 *
 * Uses a variant of the RADIANCE glare identification heuristic based on
 * a multiple of the median luminance.
 *
 * NO_CLIP_LEVEL is returned if no clipping is needed.
 */
{
    int		    row, col;
    int		    n_rows, n_cols;
    unsigned int    magnitude_hist[MAGNITUDE_HIST_NBINS];
    int		    bin;
    double	    max_luminance;
    double	    norm;
    unsigned int    total_count;
    double	    percentile;
    double	    previous_percentile;
    double	    median;
    double	    cutoff;

    n_rows = DEVA_image_n_rows ( image );
    n_cols = DEVA_image_n_cols ( image );

    for ( bin = 0; bin < MAGNITUDE_HIST_NBINS; bin++ ) {
	magnitude_hist[bin] = 0;
    }

    max_luminance = 0.0;

    for ( row = 0; row < n_rows; row++ ) {
	for ( col = 0; col < n_cols; col++ ) {
	    max_luminance = fmax ( max_luminance,
		    DEVA_image_data (image, row, col ) . Y );
	}
    }

    if ( max_luminance <= 0.0 ) {
	fprintf ( stderr, "auto_clip_median: no non-zero luminance!\n" );
	exit ( EXIT_FAILURE );
    }

    /* Spread out luminance values amoung histogram bins. */
    norm = 1.0 / max_luminance;
    for ( row = 1; row < n_rows - 1; row++ ) {
	for ( col = 1; col < n_cols - 1; col++ ) {
	    bin = norm * DEVA_image_data ( image, row, col ) . Y *
		( ( (double) MAGNITUDE_HIST_NBINS ) - MEDIAN_EPSILON );
	    magnitude_hist[bin]++;
	}
    }

    percentile = 0.0;
    previous_percentile = -1.0;
    total_count = n_rows * n_cols;

    /* Search for bin index corresponding to median (percentile == 0.5. */
    for ( bin = MAGNITUDE_HIST_NBINS - 1; bin >= 0; bin-- ) {
	percentile += ((double) magnitude_hist[bin] ) / ((double) total_count );

	if ( percentile > 0.5 ) {
	    if ( ( 0.5 - previous_percentile ) < ( percentile - 0.5 ) ) {
		bin--;
	    }

	    break;
	}
    }

    median = ( ((double) bin ) + 0.5 ) /
	( ((double) MAGNITUDE_HIST_NBINS ) - MEDIAN_EPSILON ) * max_luminance;

    cutoff = CUTOFF_RATIO_MEDIAN * median;

    /* printf ( "median = %f, maximum = %f", median, max_luminance ); */

    if ( cutoff >= max_luminance ) {
	/* printf ( ", no clipping\n" ); */
	return ( NO_CLIP_LEVEL );
    } else {
	/* printf ( ", clip level = %f\n", cutoff ); */
	return ( cutoff );
    }
}

#endif

void
clip_max_value ( DEVA_xyY_image *image, double clip_value )
/*
 * In-pace clipping of xyY image object luminance (Y) values.
 */
{
    int	    row, col;

    for ( row = 0; row < DEVA_image_n_rows ( image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( image ); col++ ) {
	    if ( DEVA_image_data ( image, row, col ) . Y > clip_value ) {
		DEVA_image_data ( image, row, col ) . Y = clip_value;
	    }
	}
    }
}

#define	TYPE_FIELD_LENGTH	14

static void
deva_filter_print_presets ( void )
{
    int	    i;

    printf ( "%s:", PRESET_MILD );
    for ( i = 0; i < TYPE_FIELD_LENGTH - strlen ( PRESET_MILD ) - 1; i++ ) {
	printf ( " " );
    }
    printf ( "Snellen 20/%d",
	    (int) Snellen_decimal_to_Snellen_denominator ( MILD_SNELLEN ) );
    printf ( " (logMAR %g)\n", MILD_LOGMAR );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "Pelli-Robson score %g\n", MILD_PELLI_ROBSON );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "color saturation %.0f%%\n", 100.0 * MILD_SATURATION );

    printf ( "%s:", PRESET_MODERATE );
    for ( i = 0; i < TYPE_FIELD_LENGTH - strlen ( PRESET_MODERATE ) - 1; i++ ) {
	printf ( " " );
    }
    printf ( "Snellen 20/%d",
	    (int) Snellen_decimal_to_Snellen_denominator ( MODERATE_SNELLEN ) );
    printf ( " (logMAR %g)\n", MODERATE_LOGMAR );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "Pelli-Robson score %g\n", MODERATE_PELLI_ROBSON );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "color saturation %.0f%%\n", 100.0 * MODERATE_SATURATION );

    printf ( "%s:", PRESET_SEVERE );
    for ( i = 0; i < TYPE_FIELD_LENGTH - strlen ( PRESET_SEVERE ) - 1; i++ ) {
	printf ( " " );
    }
    printf ( "Snellen 20/%d",
	    (int) Snellen_decimal_to_Snellen_denominator ( SEVERE_SNELLEN ) );
    printf ( " (logMAR %g)\n", SEVERE_LOGMAR );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "Pelli-Robson score %g\n", SEVERE_PELLI_ROBSON );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "color saturation %.0f%%\n", 100.0 * SEVERE_SATURATION );

    printf ( "%s:", PRESET_PROFOUND );
    for ( i = 0; i < TYPE_FIELD_LENGTH - strlen ( PRESET_PROFOUND ) - 1; i++ ) {
	printf ( " " );
    }
    printf ( "Snellen 20/%d",
	    (int) Snellen_decimal_to_Snellen_denominator ( PROFOUND_SNELLEN ) );
    printf ( " (logMAR %g)\n", PROFOUND_LOGMAR );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "Pelli-Robson score %g\n", PROFOUND_PELLI_ROBSON );

    for ( i = 0; i < TYPE_FIELD_LENGTH; i++ ) {
	printf ( " " );
    }
    printf ( "color saturation %.0f%%\n", 100.0 * PROFOUND_SATURATION );
}

static void
deva_filter_print_defaults ( void )
{
    printf ( "default acuity specification: %s\n",
	    DEFAULT_ACUITY_FORMAT_STRING );
    printf ( "default acuity effect: %s\n", DEFAULT_ACUITY_TYPE_STRING );
    printf ( "default contrast sensitivity specification: %s\n",
	    DEFAULT_SENSITIVITY_TYPE_STRING );

    printf ( "default saturation: %s\n", DEFAULT_COLOR_TYPE_STRING );

    printf ( "CSF peak sensitivity = %.1f (1.0/Michelson) @ %.2f c/deg\n",
	    ChungLeggeCSF_MAX_SENSITIVITY, ChungLeggeCSF_PEAK_FREQUENCY );

    printf ( "default smoothing of thresholding artifacts: %s\n",
	    DEFAULT_SMOOTHING_TYPE_STRING );
}
