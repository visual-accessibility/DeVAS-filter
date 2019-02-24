#ifndef __DEVA_PRESETS_H
#define __DEVA_PRESETS_H

/*
 * preset values
 * >= version 3.3.00
 *
 * Personal communication (G.E. Legge & Y-Z Xiong, Feb. 19, 2019).
 * 
 * The correlation between CS and VA: r = -0.70, p < 0.001
 * Linear regression including all subjects: CS = 1.63 - 0.76*VA. 
 * Linear regression without normals: CS = 1.62 - 0.74*VA.
 */

#define VA2CS_ALL(VA)		((1.63) - (0.76*VA))
#define VA2CS_WON(VA)		((1.62) - (0.74*VA))

#define	PRESET_NORMAL			"normal"
#define	NORMAL_SNELLEN			(20.0/20.0)
#define	NORMAL_LOGMAR			0.0
#define	NORMAL_PELLI_ROBSON		2.0
#define	NORMAL_SATURATION		1.0

#define	PRESET_MILD			"mild"
#define	MILD_SNELLEN			(20.0/45.0)
#define	MILD_LOGMAR			0.35
#define	MILD_PELLI_ROBSON		VA2CS_ALL(MILD_LOGMAR)
#define	MILD_SATURATION			.75

#define	PRESET_MODERATE			"moderate"
#define	MODERATE_SNELLEN		(20.0/115.0)
#define	MODERATE_LOGMAR			0.75
#define	MODERATE_PELLI_ROBSON		VA2CS_ALL(MODERATE_LOGMAR)
#define	MODERATE_SATURATION		.4

#define	PRESET_LEGALBLIND		"legalblind"
#define	LEGALBLIND_SNELLEN		(20.0/200.0)
#define	LEGALBLIND_LOGMAR		1.0
#define	LEGALBLIND_PELLI_ROBSON		VA2CS_ALL(LEGALBLIND_LOGMAR)
#define	LEGALBLIND_SATURATION		.3

#define	PRESET_SEVERE			"severe"
#define	SEVERE_SNELLEN			(20.0/285.0)
#define	SEVERE_LOGMAR			1.15
#define	SEVERE_PELLI_ROBSON		VA2CS_ALL(SEVERE_LOGMAR)
#define	SEVERE_SATURATION		.25

#define	PRESET_PROFOUND			"profound"
#define	PROFOUND_SNELLEN		(20.0/710.0)
#define	PROFOUND_LOGMAR			1.55
#define	PROFOUND_PELLI_ROBSON		VA2CS_ALL(PROFOUND_LOGMAR)
#define	PROFOUND_SATURATION		0.0

#endif	/* __DEVA_PRESETS_H */
