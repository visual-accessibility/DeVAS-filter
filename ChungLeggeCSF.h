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

#ifndef	__DEVA_ChungLeggeCSF_H
#define	__DEVA_ChungLeggeCSF_H

#include <stdlib.h>
#include <stdio.h>
#include "deva-license.h"	/* DEVA open source license */

/*
 * Values for 20/20 vision:
 */
#define	ChungLeggeCSF_MAX_SENSITIVITY	199.0
	/* Pelli-Robson score of 2.0 is 0.01 Weber contrast which is */
	/* 0.005 Michelson contrast.  This yields a Michelson sensitivity */
	/* of 199.  Confirmed by testing minimum contrast of readable */
	/* characters filtered to simulate normal vision */

#define ChungLeggeCSF_PEAK_FREQUENCY	0.914	/* cutoff: 14.0 cpd */
	/* Set based on empirical testing of smallest readable filtered */
	/* maximum contrast Sloan characters, using peak sensitivity as above */

#define	ChungLeggeCSF_K_LEFT		0.68	/* magic constant (left) */
#define	ChungLeggeCSF_K_RIGHT		1.28	/* magic constant (right) */

#ifndef	TRUE
#define	TRUE		1
#endif	/* TRUE */
#ifndef	FALSE
#define	FALSE		0
#endif	/* FALSE */

#ifdef __cplusplus
extern "C" {
#endif

double	ChungLeggeCSF ( double spatial_frequency, double acuity_adjust,
	    double contrast_sensitivity_adjust );
double	ChungLeggeCSF_peak_sensitivity ( double acuity_adjust,
	    double contrast_sensitivity_adjust );
double	ChungLeggeCSF_peak_frequency ( double acuity_adjust,
	    double contrast_sensitivity_adjust );
double	ChungLeggeCSF_cutoff_frequency ( double acuity_adjust,
	    double contrast_sensitivity_adjust );
double	ChungLeggeCSF_peak_from_cutoff ( double cutoff_frequency,
	    double contrast_sensitivity_adjust );
void	ChungLeggeCSF_print_stats ( double acuity_adjust,
	    double contrast_sensitivity_adjust );
void	ChungLeggeCSF_print_parms ( void );
double	ChungLeggeCSF_cutoff_acuity_adjust ( double acuity_adjust,
	    double contrast_sensitivity_adjust );

void	ChungLeggeCSF_set_peak_sensitivity ( double new_peak_sensitivity );
void	ChungLeggeCSF_set_peak_frequency ( double new_peak_frequency );


#ifdef __cplusplus
}
#endif

#endif  /* __DEVA_ChungLeggeCSF_H */
