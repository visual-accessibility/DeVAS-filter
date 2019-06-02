#ifndef __DeVAS_PRESETS_H
#define __DeVAS_PRESETS_H

/*
 * preset values
 * >= version 3.3.00
 *
 * Personal communication (G.E. Legge & Y-Z Xiong, Feb. 19, 2019).
 *
 * Visual acuity (VA) in logMAR units
 * Contrast sensitivity (CS) in Pelli-Robson units
 * 
 * email from Yingzi on 5/8/2019:
 *
 *   All (N = 1032): CS = 1.72 -0.69*VA, R2 = 0.51
 *   Normal (N = 294): CS = 1.77 -0.77*VA, R2 = 0.14
 *   Cataract (N = 450): CS = 1.72 -0.38*VA, R2 = 0.15
 *   AMD (N = 172): CS = 1.65 -0.60*VA, R2 = 0.56
 *   Glaucoma (N = 116): CS = 1.67 -0.78*VA, R2 = 0.61
 */

#define VA2CS_ALL(VA)		(1.72-(0.69*(VA)))
					/* all including normals */

#define	VA2CS_NORMAL(VA)	(1.77-(0.77*(VA))
					/* normals only */

#define VA2CS_withoutN(VA)	(1.62 - (0.74*(VA)))	/* all except normals */

#define	VA2CS_cataract(VA)	(1.72 - (0.38*(VA)))	/* cataracts */

#define VA2CS_AMD(VA)		(1.65 - (0.60*(VA)))	/* age-related macular
							   degeneration */

#define	VA2CS_glaucoma(VA)	(1.67 - (0.78*(VA)))	/* glaucoma */


/*
 * Color:
 *
 * Signifcant loss of acuity and contrast sensitivity is often associated with
 * color vision deficiencies, though there is little data on the correlation
 * between the severity of acuity and contrast loss and the type and severity
 * of color vision loss.  Visibility judgments made by devas-visibility depend
 * only on luminance and so are unaffected by color deficits.  For simulation
 * of low vision views produced by devas-filter and devas-visibility, we have
 * chosen to reduce the color saturation as a function of acuity using the
 * following formula:  s = 1 - (0.667 * l ), where s is the fraction of
 * saturation retained, l is the logMAR value, and s is constrained to s <= 1.
 * Note that this is not based on any impirical evidence and is only an
 * aesthetic judgment made by the devas-filter software designer.
 */

#define	REDUCED_COLOR_SAT(LOGMAR)	(fmax((1.0-(0.667*(LOGMAR))),0.0))


/*
 * Presets
 */

/*
 * Our modeling does not address small differences in visibility over the
 * range of normal vision i.e., from -0.3 to 0.3 logMAR. 
#define	PRESET_NORMAL			"normal"
#define	NORMAL_SNELLEN			(20.0/20.0)
#define	NORMAL_LOGMAR			0.0
#define	NORMAL_PELLI_ROBSON		2.0
#define	NORMAL_SATURATION		1.0
 */
#define	LOGMAR_MIN_LIMIT	0.3	/* Our modeling does not address small
					   differences in visibility over the
					   range of normal vision, i.e., from
					   -0.3 to 0.3 logMAR. */

#define	PRESET_MILD			"mild"
#define	MILD_SNELLEN			(20.0/45.0)
#define	MILD_LOGMAR			0.35
#define	MILD_PELLI_ROBSON		VA2CS_ALL(MILD_LOGMAR)
#define	MILD_SATURATION			REDUCED_COLOR_SAT (MILD_LOGMAR) //.75

#define	PRESET_MODERATE			"moderate"
#define	MODERATE_SNELLEN		(20.0/115.0)
#define	MODERATE_LOGMAR			0.75
#define	MODERATE_PELLI_ROBSON		VA2CS_ALL(MODERATE_LOGMAR)
#define	MODERATE_SATURATION		REDUCED_COLOR_SAT(MODERATE_LOGMAR) //.4

#define	PRESET_LEGALBLIND		"legalblind"
#define	LEGALBLIND_SNELLEN		(20.0/200.0)
#define	LEGALBLIND_LOGMAR		1.0
#define	LEGALBLIND_PELLI_ROBSON		VA2CS_ALL(LEGALBLIND_LOGMAR)
#define	LEGALBLIND_SATURATION		REDUCED_COLOR_SAT(LEGALBLIND_LOGMAR)//.3

#define	PRESET_SEVERE			"severe"
#define	SEVERE_SNELLEN			(20.0/285.0)
#define	SEVERE_LOGMAR			1.15
#define	SEVERE_PELLI_ROBSON		VA2CS_ALL(SEVERE_LOGMAR)
#define	SEVERE_SATURATION		REDUCED_COLOR_SAT(SEVERE_LOGMAR) //.25

#define	PRESET_PROFOUND			"profound"
#define	PROFOUND_SNELLEN		(20.0/710.0)
#define	PROFOUND_LOGMAR			1.55
#define	PROFOUND_PELLI_ROBSON		VA2CS_ALL(PROFOUND_LOGMAR)
#define PROFOUND_SATURATION		REDUCED_COLOR_SAT(PROFOUND_LOGMAR)//


#endif	/* __DeVAS_PRESETS_H */
