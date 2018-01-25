#ifndef __DEVA_RADIANCE_HEADER_H
#define __DEVA_RADIANCE_HEADER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "radiance/color.h"
#include "radiance/platform.h"
#include "radiance/resolu.h"
#include "radiance/fvect.h"	/* must preceed include of view.h */
#include "radiance/view.h"
#include "deva-license.h"	/* DEVA open source license */

#ifndef	TRUE
#define	TRUE		1
#endif	/* TRUE */
#ifndef	FALSE
#define	FALSE		0
#endif	/* FALSE */

typedef enum {
    radcolor_unknown,
    radcolor_missing,
    radcolor_rgbe,
    radcolor_xyze
} RadianceColorFormat;

#ifdef __cplusplus
extern "C" {
#endif

void
DEVA_read_radiance_header ( FILE *radiance_fp, int *n_rows_p, int *n_cols_p,
	RadianceColorFormat *color_format_p, VIEW *view_p, int *exposure_set_p,
	double *exposure_p, char **header_text_p );

void
DEVA_write_radiance_header ( FILE *radiance_fp, int n_rows, int n_cols,
	RadianceColorFormat color_format, VIEW view, int set_exposure,
	double exposure, char *other_info );

#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_RADIANCE_HEADER_H */
