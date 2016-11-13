#ifndef __KLTFILTER_H
#define __KLTFILTER_H

#include <stdlib.h>
#include "deva-image.h"
#include "deva-license.h"       /* DEVA open source license */

#define	KLTCSF_MAX_SENSITIVITY	200.0
#define	KLTCSF_PEAK_FREQUENCY	1.959	/* cutoff: 30.0 cpd */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_xyY_image	*KLT_filter ( DEVA_xyY_image *input_image, double acuity,
		    double contrast_sensitivity, double saturation );
void		KLT_filter_print_version ( void );

#ifdef __cplusplus
}
#endif

#endif  /* __KLTFILTER_H */
