/*
 * Convert acuity values between different measures.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include "acuity-conversion.h"
#include "deva-license.h"	/* DEVA open source license */

/* The following are support routines for parse_snellen () */
static int	count_character ( char *string, char c );
static double	convert_single_value ( char *snellen_string,
		    char *full_snellen_string );
static double	convert_double_value ( char *snellen_string );
static void	not_snellen_string ( char *string );

double
Snellen_decimal_to_logMAR ( double Snellen_decimal )
/*
 * Convert Snellen fraction, expressed as a single decimal number (e.g.,
 * 20/100 is indicated as 0.2) to logMAR value.
 */
{
    double  logMAR;

    if ( Snellen_decimal <= 0.0 ) {
	fprintf ( stderr, "Snellen_decimal_to_logMAR: invalid value (%g)!\n",
		Snellen_decimal );
	exit ( EXIT_FAILURE );
    }

    logMAR = -log10 ( Snellen_decimal );

    if ( logMAR == -0.0 ) {
	logMAR = 0.0;
    }


    return ( logMAR );
}

double
logMAR_to_Snellen_decimal ( double logMAR )
/*
 * Convert logMAR value to Snellen fraction, expressed as a single decimal
 * number (e.g., 20/100 is indicated as 0.2).
 */
{
    return ( pow ( 10.0, -logMAR ) );
}

double
Snellen_decimal_to_Snellen_denominator ( double Snellen_decimal )
{
    if ( Snellen_decimal <= 0.0 ) {
	fprintf ( stderr,
		"Snellen_decimal_to_Snellen_denominator: invalid value (%g)!\n",
		Snellen_decimal );
	exit ( EXIT_FAILURE );
    }

    return ( SNELLEN_NUMERATOR / Snellen_decimal );
}

double
Snellen_denominator_to_Snellen_decimal ( double Snellen_denominator )
/*
 * Assume U.S. standard numerator.
 */
{
    if ( Snellen_denominator <= 0.0 ) {
	fprintf ( stderr,
		"Snellen_denominator_to_Snellen_decimal: invalid value (%g)!\n",
		Snellen_denominator );
	exit ( EXIT_FAILURE );
    }

    return ( SNELLEN_NUMERATOR / Snellen_denominator );
}

double
Snellen_denominator_to_logMAR ( double Snellen_denominator )
{
    return ( Snellen_decimal_to_logMAR (
		Snellen_denominator_to_Snellen_decimal ( Snellen_denominator )
	    ) );
}

double
logMAR_to_Snellen_denominator ( double logMAR )
{
    return ( Snellen_decimal_to_Snellen_denominator (
		logMAR_to_Snellen_decimal ( logMAR )
	    ) );
}


double
parse_snellen ( char *snellen_string )
/*
 * Return the numeric value of a string that is either a single positive
 * floating point number or of the form "n/m" where n and m are each
 * positive floating point numbers.
 */
{
    if ( count_character ( snellen_string, '/' ) == 0 ) {
	return ( convert_single_value ( snellen_string, snellen_string ) );
    } else if ( count_character ( snellen_string, '/' ) == 1 ) {
	return ( convert_double_value ( snellen_string ) );
    } else {
	not_snellen_string ( snellen_string );
    }

    return ( 0.0 );
}

static int
count_character ( char *string, char c )
{
    char    *cpt;
    int	    count;

    count = 0;

    cpt = string;
    while ( *cpt != '\0' ) {
	if ( *cpt == c ) {
	    count++;
	}
	cpt++;
    }

    return ( count );
}

static double
convert_single_value ( char *snellen_string, char *full_snellen_string )
/*
 * Convert a string consisting *only* of a single, positive, possibly
 * floating point number.
 */
{
    double  value;
    char    *endptr;

    value = strtod ( snellen_string, &endptr );
    if ( ( value <= 0.0 ) || ( *endptr != '\0' ) ) {
	not_snellen_string ( full_snellen_string );
    }

    return ( value );
}

static double
convert_double_value ( char *snellen_string )
{
    char    *tmp_string;
    char    *cpt;
    double  numerator, denominator;

    tmp_string = strdup ( snellen_string );
    if ( tmp_string == NULL ) {
	fprintf ( stderr, "parse_snellen: malloc failed!\n" );
	exit ( EXIT_FAILURE );
    }

    cpt = strchr ( tmp_string, '/' );
    if ( cpt == NULL ) {
	fprintf ( stderr, "parse_snellen: internal error!\n" );
	exit ( EXIT_FAILURE );
    }

    *cpt = '\0';
    cpt++;

    numerator = convert_single_value ( tmp_string, snellen_string );
    denominator = convert_single_value ( cpt, snellen_string );

    free ( tmp_string );

    return ( numerator / denominator );
}

static void
not_snellen_string ( char *string )
{
    fprintf ( stderr, "\"%s\" is not a valid snellen number!\n", string );
    exit ( EXIT_FAILURE );
}
