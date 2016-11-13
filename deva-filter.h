#ifndef __DEVAFILTER_H
#define __DEVAFILTER_H

#include <stdlib.h>
#include "deva-image.h"
#include "deva-license.h"       /* DEVA open source license */

/* Control informational output of deva_filter ( ) */
extern int	DEVA_verbose;		/* print generally useful info */
extern int	DEVA_veryverbose;	/* print debugging info */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DEVA_xyY_image	*deva_filter ( DEVA_xyY_image *input_image, double acuity,
		    double contrast_sensitivity, int smoothing_flag,
       		    double saturation );
void		deva_filter_print_version ( void );

#ifdef __cplusplus
}
#endif

#endif  /* __DEVAFILTER_H */
