/*
 * From: Susana Chung and Gordon Legge, "Comparing the Shape of the Contrast
 * Sensitivity Functions of Normal and Low Vision," Investigative Ophthalmology
 * & Visual Science, 57(1), 2016.
 *
 * Normal vision peak sensitivity chosen based on recognition performance 
 * for letters with Pelli-Robson Chart score of 2.0.  Normal vision peak
 * sensitivity frequency chosen based on recognition performance at cutoff
 * for maximum contrast letters with CSF shifted to simulate acuities
 * ranging from logMAR 0.2 to logMAR 1.6.
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ChungLeggeCSF.h"
#include "devas-license.h"	/* DeVAS open source license */

#define	SQ(x)		( (x) * (x) )
#define	exp10(x)	pow ( 10.0, x )		/* can't count on availablility
					   	   of exp10 */

static double	peak_sensitivity = ChungLeggeCSF_MAX_SENSITIVITY;
static double	peak_frequency = ChungLeggeCSF_PEAK_FREQUENCY;

static double	cutoff_frequency_to_logMAR ( double frequency );
static double	peak_frequency_to_logMAR ( double frequency );

double
ChungLeggeCSF ( double spatial_frequency, double acuity_adjust,
				double contrast_sensitivity_adjust )
/*
 * Input:
 *
 *   spatial_frequency:	Spatial frequency in, in cycles/degree, for which
 *   			sensitivity is to be calculated.
 *
 *   acuity_adjust:	Adustment for frequency at which peak sensitivity
 *   			occurs.
 *
 *   			1.0:	peak sensitivity frequency corresponds to
 *   				"normal" vision
 *   			< 1.0:	peak sensitivity frequency is less than for
 *   				"normal" vision
 *			Allowable range is [1.0 - 0.0) (no support for
 *			hyper-acuity!)
 *
 *   contrast_sensitivity_adjust:
 *
 *   			Ratio of peak contrast sensitivity to normal vision
 *   			peak sensitivity. Allowable range is [1.0 - 0.0)
 *
 * Output:
 *
 *   Sensitivity (reciprocal of Michelson contrast at threshold).
 */
{
    double  S;		/* log Michelson contrast sensitivity */
    double  k;		/* magic constant from Pelli, Legge, & Rubin (1987) */

    if ( spatial_frequency <= 0.0 ) {
	fprintf ( stderr, "ChungLeggeCSF: invalid spatial frequency (%g)!\n",
	       spatial_frequency );
	exit ( EXIT_FAILURE );
    }
    /* if ( ( acuity_adjust > 1.0) || ( acuity_adjust <= 0.0 ) ) { */
    if ( acuity_adjust <= 0.0 ) {
	/* allow for hyper acuity? */
	fprintf ( stderr, "ChungLeggeCSF: invalid acuity_adjust (%g)!\n",
		acuity_adjust );
	exit ( EXIT_FAILURE );
    }
    if ( ( contrast_sensitivity_adjust <= 0.0) ||
	    ( contrast_sensitivity_adjust > 1.0 ) ) {
	fprintf ( stderr, "ChungLeggeCSF: invalid contrast sensitivity (%g)!\n",
		contrast_sensitivity_adjust );
	exit ( EXIT_FAILURE );
    }

    if ( spatial_frequency <
	    ( peak_frequency * acuity_adjust ) ) {
	k = ChungLeggeCSF_K_LEFT;
    } else {
	k = ChungLeggeCSF_K_RIGHT;
    }

    S = ( log10 ( peak_sensitivity ) + log10 ( contrast_sensitivity_adjust ) ) -
	    ( SQ ( k ) * ( SQ ( log10 ( spatial_frequency ) -
		    ( log10 ( peak_frequency ) + log10 ( acuity_adjust )
		      ) ) ) );

    return ( exp10 ( S ) );
}

double
ChungLeggeCSF_peak_sensitivity ( double acuity_adjust,
	double contrast_sensitivity_adjust )
/*
 * Peak sensitivity (reciprocal of Michelson contrast at threshold).
 */
{
    return ( contrast_sensitivity_adjust * peak_sensitivity );
}

double
ChungLeggeCSF_peak_frequency ( double acuity_adjust,
	double contrast_sensitivity_adjust )
/*
 * Frequency at which peak sensitivity occurs.
 */
{
    return ( peak_frequency * acuity_adjust );
}

double
ChungLeggeCSF_cutoff_frequency ( double acuity_adjust,
	double contrast_sensitivity_adjust )
/*
 * High frequency where log_10 ( sensitivity) == 0:
 *
 *   ( Sn + log10(f) ) - ( k^2 * ( f - ( Fn + log10(a) ) )^2 ) = 0
 *
 *	f:  log_10 spatial frequency
 *	Sn: log_10 normal vision peak sensitivity
 *	Fn: log_10 frequency at which normal vision peak sensitivity occurs
 *	a:  acuity_adjust
 *	c:  contrast_sensitivity_adjust
 *	k:  parabola slope parameter
 *
 * Solution (from MATLAB symbolic math):
 *
 *   f = Fn + ln(a)/ln(10) + ((ln(c) + Sn*ln(10))^(1/2))/(k*ln(10)^(1/2))
 *
 * Dan's simplification:
 *
 *   f = Fn + log10(a) = ((log10(c) + Sn)^(1/2))/k
 */
{
    double  cutoff_frequency;
    double  Sn, Fn, a, c, k;	/* to simplify notation */

    Sn = log10 ( peak_sensitivity );
    Fn = log10 ( peak_frequency );
    a = acuity_adjust;
    c = contrast_sensitivity_adjust;
    k = ChungLeggeCSF_K_RIGHT; 

    /*********************************
    cutoff_frequency_old_method = exp10 ( Fn + ( log ( a ) / log ( 10.0 ) ) +
	    ( sqrt ( log ( c ) + ( Sn * log ( 10.0 ) ) ) /
		( k * sqrt ( log (10.0 ) ) ) ) );
     *********************************/

    cutoff_frequency = exp10 ( Fn + log10 ( a ) +
	    ( sqrt ( log10 ( c ) + Sn ) / k ) );

    if ( cutoff_frequency <= 0.0 ) {
	fprintf ( stderr,
		"ChungLeggeCSF_cutoff_frequency: frequency <= 0.0!\n" );
	exit ( EXIT_FAILURE );
    }

    return ( cutoff_frequency );
}

double
ChungLeggeCSF_peak_from_cutoff ( double cutoff_frequency,
	double contrast_sensitivity_adjust )
/*
 * Solve for peak sensitivity frequency given cutoff
 *
 *	S_0 - ( k_right * ( F_cutoff-F_0 )^2 ) = 0
 *
 *		S_0, F0, and F_cutoff in log10 units.
 *
 * solve for log10(Fp):
 *
 *   ( Sn + log10(c) ) - ( ( k^2 ) * ( Fc - Fp )^2 ) = 0
 *
 *	Fc: log10 cutoff frequency
 *	Sn: log10 normal vision peak sensitivity
 *	Fp: log10 frequency at which peak sensitivity occurs
 *	c:  contrast_sensitivity_adjust
 *	k:  parabola slope parameter
 *
 * Solution (from MATLAB symbolic math):
 *
 *   Fp = Fc - ((ln(c) + Sn*ln(10))^(1/2))/(k*ln(10)^(1/2))
 *
 * Dan's simplification:
 *
 *   Fp = Fc - (((log10(c)+Sn)^(1/2))/k)
 */
{
    double  peak_sensitivity_frequency;
    double  Sn, Fc, c, k;	/* to simplify notation */

    Fc = log10 ( cutoff_frequency );
    c = contrast_sensitivity_adjust;
    Sn = log10 ( peak_sensitivity );
    k = ChungLeggeCSF_K_RIGHT;

    /********************
    peak_sensitivity_frequency_old_method =
	exp10 ( Fc - ( ( sqrt ( log ( c ) + ( Sn * log ( 10.0 ) ) ) ) /
		( k * sqrt ( log ( 10.0 ) ) ) ) );
     ********************/

    peak_sensitivity_frequency =
	exp10 ( Fc - ( sqrt ( log10 ( c ) + Sn ) / k ) );

    return ( peak_sensitivity_frequency );
}

void
ChungLeggeCSF_print_stats ( double acuity_adjust,
	double contrast_sensitivity_adjust )
{
    double  cutoff_frequency;
    double  logMAR;

  printf ( "ChungLeggeCSF: peak_sensitivity: %.1f at %.2f c/d (logMAR %.2f)\n",
		ChungLeggeCSF_peak_sensitivity ( acuity_adjust,
		    contrast_sensitivity_adjust ),
		ChungLeggeCSF_peak_frequency ( acuity_adjust,
		    contrast_sensitivity_adjust ),
	        peak_frequency_to_logMAR (
		    ChungLeggeCSF_peak_frequency ( acuity_adjust,
			contrast_sensitivity_adjust )
		    ) );

    cutoff_frequency = ChungLeggeCSF_cutoff_frequency ( acuity_adjust,
	    contrast_sensitivity_adjust );
    logMAR = cutoff_frequency_to_logMAR ( cutoff_frequency );
    /* deal with -0.0 for small negative values */
    if ( logMAR == -0.0 ) {
	logMAR = 0.0;
    }

    printf ( "ChungLeggeCSF: cutoff frequency: %.2f c/d (logMAR %.2f)\n",
	    	cutoff_frequency, logMAR );
}

void
ChungLeggeCSF_print_parms ( void )
{
    fprintf ( stderr,
	    "ChungLeggeCSF: max sensitivity (normal) = %.2f @ %.2f c/d\n",
	    peak_sensitivity, peak_frequency );
}

double
ChungLeggeCSF_cutoff_acuity_adjust ( double old_acuity_adjust, 
	double contrast_sensitivity_adjust )
/*
 * acuity_adjust value to pass to ChungLeggeCSF if original value was
 * with respect to cutoff frequency, not frequency of peak sensitivity.
 */
{
    double  normal_vision_cutoff;
    double  desired_cutoff;
    double  desired_peak_frequency;
    double  new_acuity_adjust;

    normal_vision_cutoff = ChungLeggeCSF_cutoff_frequency ( 1.0, 1.0 );

    desired_cutoff = old_acuity_adjust * normal_vision_cutoff;

    desired_peak_frequency = ChungLeggeCSF_peak_from_cutoff ( desired_cutoff,
	    contrast_sensitivity_adjust );

    new_acuity_adjust = desired_peak_frequency /
	ChungLeggeCSF_peak_frequency ( 1.0, contrast_sensitivity_adjust );

    return ( new_acuity_adjust );
}

void
ChungLeggeCSF_set_peak_sensitivity ( double new_peak_sensitivity )
{
    peak_sensitivity = new_peak_sensitivity;
}

void
ChungLeggeCSF_set_peak_frequency ( double new_peak_frequency )
{
    peak_frequency = new_peak_frequency;
}

static double
cutoff_frequency_to_logMAR ( double frequency )
{
    /*
     * ChungLeggeCSF_cutoff_frequency ( 1.0, 1.0 ) is spatial frequency
     * corresponding to logMAR 0.0
     */
    return ( log10 ( ChungLeggeCSF_cutoff_frequency ( 1.0, 1.0 ) /
		frequency ) );
}

static double
peak_frequency_to_logMAR ( double frequency )
{
    /*
     * ChungLeggeCSF_cutoff_frequency ( 1.0, 1.0 ) is spatial frequency
     * corresponding to logMAR 0.0
     */
    return ( log10 ( ChungLeggeCSF_cutoff_frequency ( 1.0, 1.0 ) /
		frequency ) );
}
