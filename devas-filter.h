#ifndef __DeVAS_FILTER_H
#define __DeVAS_FILTER_H

#include <stdlib.h>
#include "devas-image.h"
#include "devas-license.h"       /* DeVAS open source license */

/* Control informational output of devas_filter ( ) */
extern int	DeVAS_verbose;		/* print generally useful info */
extern int	DeVAS_veryverbose;	/* print debugging info */

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

DeVAS_xyY_image	*devas_filter ( DeVAS_xyY_image *input_image, double acuity,
		    double contrast_sensitivity, int smoothing_flag,
       		    double saturation );
void		devas_filter_print_version ( void );

#ifdef __cplusplus
}
#endif

#endif  /* __DeVAS_FILTER_H */
