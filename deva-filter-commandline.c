/*
 * Command line interface for deva-filter simulation of reduced acuity
 * and contrast sensitivity.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>		/* for strcmp, strlen */
#include <strings.h>		/* for strcasecmp, strncasecmp */
#include <libgen.h>		/* for basename */
#include "deva-image.h"
#include "deva-filter.h"
#include "KLT-filter.h"
#include "deva-utils.h"
#include "radianceIO.h"
#include "acuity-conversion.h"
#include "ChungLeggeCSF.h"
#include "deva-license.h"	/* DEVA open source license */

char	*progname;	/* one radiance routine requires this as a global */
char	*Usage =
	    "--mild|--moderate|--significant|--severe input.hdr output.hdr";
char	*Usage2 = "[--snellen|--logMAR] [--sensitivity-ratio|--pelli-robson]"
	    "\n\t[--autoclip|--clip=<level>] [--color|--grayscale] [--verbose]"
	    "\n\t\tacuity contrast input.hdr output.hdr";
int	args_needed = 4;

/* option flags */
typedef enum { no_preset, mild, moderate, significant, severe } PresetType;
typedef	enum { undefined_acuity_format, Snellen, logMAR } AcuityFormat;
typedef enum { undefined_sensitivity, sensitivity_ratio, pelli_robson }
							SensitivityType;
typedef enum { undefined_color, color, grayscale, saturation_value } ColorType;
typedef enum { undefined_clip, auto_clip, value_clip } ClipType;
/* hidden option flags */
typedef	enum { undefined_acuity_type, peak_sensitivity, cutoff }
							AcuityType;
typedef enum { undefined_smoothing, no_smoothing, smoothing } SmoothingType;
typedef enum { undefined_filter, use_DEVA_filter, use_KLT_filter } FilterType;

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

/* pre-set values */

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

#define	LOGMAR_MAX		2.3
#define	LOGMAR_MIN		(-0.65)
#define	SNELLEN_MAX		4.0
#define	SNELLEN_MIN		0.005

#define	CONTRAST_RATIO_MIN	0.01
#define	CONTRAST_RATIO_MAX	2.5

#define	PELLI_ROBSON_MIN	0.0
#define	PELLI_ROBSON_MAX	2.4
#define PELLI_ROBSON_NORMAL	2.0	/* normal vision score on chart */

#define	GLARE_CUTOFF_RATIO	7.0	/* RADIANCE identifies glare sources */
					/* as being brighter than 7 times the */
					/* average luminance level. */
#define	NO_CLIP_LEVEL		-1.0	/* don't clip values */

#define	exp10(x)	pow ( 10.0, (x) ) /* can't count on availablility
					     of exp10 */

static void	add_description_arguments ( DEVA_xyY_image *image, int argc,
		    char *argv[] );
static void	print_usage ( void );
static void	internal_error ( void );
static double	PelliRobson2contrastratio ( double PelliRobson_score );
static double	contrastratio2PelliRobson ( double contrast_ratio );
static double	auto_clip_level ( DEVA_xyY_image *image );
static void	clip_max_value ( DEVA_xyY_image *image, double clip_value );
static void	deva_filter_print_defaults ( void );

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
    FilterType		filter_type = undefined_filter;

    /* initialized next four variables to quiet uninitialized warning */
    double		saturation = -1.0;
    double		clip_value = -1.0;
    double		acuity = -1.0;		/* ratio to normal */
    double		contrast_ratio = -1.0;	/* ratio to normal */

    int			smoothing_flag;	/* argument to deva_filter ( ) */

    double		acuity_adjustment;	/* CSF peak adjustment */
    double		logMAR_arg;		/* used for sanity check */
    double		pelli_robson_score;	/* log contrast */
    char		*input_file_name;
    char		*output_file_name;
    DEVA_xyY_image	*input_image;		/* Y values in cd/m^2 */
    DEVA_xyY_image	*filtered_image;	/* Y values in cd/m^2 */

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
	
	} else if ( ( strcasecmp ( argv[argpt], "--significant" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-significant" ) == 0 ) ) {
	    /* preset for significant loss of acuity and contrast */
	    if ( ! ( ( preset_type == no_preset ) ||
				( preset_type == significant ) ) ) {
		fprintf ( stderr, "conflicting preset values!\n" );
		exit ( EXIT_FAILURE );
	    }
	    preset_type = significant;
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

	} else if ( ( strcasecmp ( argv[argpt], "--version" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-version" ) == 0 ) ) {
	    /* print version number then exit */
	    deva_filter_print_version ( );
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

	} else if ( ( strcasecmp ( argv[argpt], "--KLTfilter" ) == 0 ) ||
		( strcasecmp ( argv[argpt], "-KLTfilter" ) == 0 ) ) {
	    /* Use KLT filter (should not be in final version!) */
	    filter_type = use_KLT_filter;
	    argpt++;

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

		acuity_type = Snellen;
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
		/* use default unless --KLTfilter flag is given */
		/* filter_type = use_DEVA_filter; */

		break;

	    case moderate:

		acuity_type = Snellen;
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
		/* use default unless --KLTfilter flag is given */
		/* filter_type = use_DEVA_filter; */

		break;

	    case significant:

		acuity_type = Snellen;
		acuity = SIGNIFICANT_SNELLEN;

		sensitivity_type = pelli_robson;
		contrast_ratio =
		    PelliRobson2contrastratio ( SIGNIFICANT_PELLI_ROBSON );

		if ( SIGNIFICANT_SATURATION == 1.0 ) {
		    color_type = color;
		} else if ( SIGNIFICANT_SATURATION == 0.0 ) {
		    color_type = grayscale;
		} else {
		    color_type = saturation_value;
		}
		saturation = SIGNIFICANT_SATURATION;

		clip_type = auto_clip;
		acuity_type = cutoff;
		smoothing_type = smoothing;
		/* use default unless --KLTfilter flag is given */
		/* filter_type = use_DEVA_filter; */

		break;

	    case severe:

		acuity_type = Snellen;
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
		/* use default unless --KLTfilter flag is given */
		/* filter_type = use_DEVA_filter; */

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

    if ( filter_type == undefined_filter ) {
	filter_type = DEFAULT_FILTER_TYPE;
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

    input_file_name = argv[argpt++];
    output_file_name = argv[argpt++];

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

    input_image = DEVA_xyY_from_radfilename ( input_file_name );

    switch ( clip_type ) {

	case auto_clip:

	    clip_value = auto_clip_level ( input_image );
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

    switch ( filter_type ) {

	case use_DEVA_filter:

	    filtered_image = deva_filter ( input_image, acuity_adjustment,
		    contrast_ratio, smoothing_flag, saturation );

	    if ( DEVA_veryverbose ) {
		fprintf ( stderr, "deva_filter ( %s, %.4f, %.4f, %d, %.2f )\n",
			input_file_name, acuity_adjustment, contrast_ratio,
			smoothing_flag, saturation );
	    }

	    break;

	case use_KLT_filter:

	    /* Used to compare deva-filter to CSF as MTF filter */
	    /* Should not be used for production work! */

	    filtered_image = KLT_filter ( input_image, acuity_adjustment,
		    contrast_ratio, saturation );

	    if ( DEVA_veryverbose ) {
		fprintf ( stderr, "KLT_filter ( %s, %.4f, %.4f, %.2f )\n",
			input_file_name, acuity_adjustment, contrast_ratio,
			saturation );
	    }

	    break;

	case undefined_filter:
	default:

	    internal_error ( );

	    break;
    }

    add_description_arguments ( filtered_image, argc, argv );

    DEVA_xyY_to_radfilename ( output_file_name, filtered_image );

    DEVA_xyY_image_delete ( input_image );
    DEVA_xyY_image_delete ( filtered_image );

    return ( EXIT_SUCCESS );	/* normal exit */
}

static void
add_description_arguments ( DEVA_xyY_image *image, int argc, char *argv[] )
/*
 * Add current command line to description.
 * Need pointer to image object, since we are changing the object properties.
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
    double  requested_weber, requested_michelson, requested_sensitivity;
    double  normal_weber, normal_michelson, normal_sensitivity;
    double  contrast_ratio;

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
    double  normal_weber;
    double  normal_michelson;
    double  requested_michelson;
    double  requested_weber;
    double  PelliRobson_score;

    normal_weber = exp10 ( -PELLI_ROBSON_NORMAL );
    normal_michelson = -normal_weber / ( normal_weber - 2.0 );
    requested_michelson = normal_michelson / contrast_ratio;
    requested_weber =
	( 2.0 * requested_michelson ) / ( requested_michelson + 1.0 );

    PelliRobson_score = -log10 ( requested_weber );

    return ( PelliRobson_score );
}

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
    double	    glare_cutoff_initial;
    double	    glare_cutoff_revised;
    unsigned int    glare_count;

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
    glare_cutoff_initial = GLARE_CUTOFF_RATIO * average_luminance_initial;

    if ( glare_cutoff_initial >= max_luminance ) {
	/* no need for glare source clipping */
	return ( NO_CLIP_LEVEL );
    }

    average_luminance_revised = 0.0;
    glare_count = 0;

    for ( row = 0; row < DEVA_image_n_rows ( image ); row++ ) {
	for ( col = 0; col < DEVA_image_n_cols ( image ); col++ ) {
	    if ( DEVA_image_data ( image, row, col ) . Y >
		    glare_cutoff_initial ) {
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
    glare_cutoff_revised = GLARE_CUTOFF_RATIO * average_luminance_revised;

    return ( glare_cutoff_revised );
}

void
clip_max_value ( DEVA_xyY_image *image, double clip_value )
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