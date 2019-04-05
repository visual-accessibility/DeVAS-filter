/*
 * Convert acuity values between different measures.
 */

#ifndef __DeVAS_ACUITY_CONVERSION_H
#define __DeVAS_ACUITY_CONVERSION_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "devas-license.h"	/* DeVAS open source license */

#define	SNELLEN_NUMERATOR	20.0	/* U.S. standard */
					/* change to 6.0 for metric */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

double	Snellen_decimal_to_logMAR ( double Snellen_decimal );
double	logMAR_to_Snellen_decimal ( double logMAR );
double	Snellen_decimal_to_Snellen_denominator ( double Snellen_decimal );
double	Snellen_denominator_to_Snellen_decimal ( double Snellen_denominator );
double	Snellen_denominator_to_logMAR ( double Snellen_denominator );
double	logMAR_to_Snellen_denominator ( double logMAR );
double	parse_snellen ( char *snellen_string );


#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_ACUITY_CONVERSION_H */

