/*
 * Read DEVA geometry files.
 *
 * Canonical units for position/distance are centimeter.
 */

#ifndef	__DEVA_READ_GEOMETRY_H
#define	__DEVA_READ_GEOMETRY_H

#include <stdlib.h>
#include <stdio.h>
#include "deva-image.h"
#include "radiance/fvect.h"     /* for FVECT type */
#include "radiance/view.h"      /* for VIEW structure */
#include "deva-license.h"       /* DEVA open source license */

typedef enum { unknown_unit, centimeters, meters, inches, feet }
							DEVA_geom_units;

typedef struct {
    DEVA_geom_units units;
    double	    convert_to_centimeters;
    VIEW	    view;
} DEVA_coordinates;

#ifdef __cplusplus
extern "C" {
#endif

VIEW		    deva_get_VIEW_from_filename ( char *filename, int *n_rows,
			int *c_cols );
void		    deva_print_VIEW ( VIEW view );
int		    DEVA_geom_dim_from_radfilename ( char *filename );
int		    DEVA_geom_dim_from_radfile ( FILE *radiance_fp,
       			char *filename );
DEVA_XYZ_image	    *DEVA_geom3d_from_radfilename ( char *filename );
DEVA_XYZ_image	    *DEVA_geom3d_from_radfile ( FILE *radiance_fp,
			char *filename );
DEVA_float_image    *DEVA_geom1d_from_radfilename ( char *filename );
DEVA_float_image    *DEVA_geom1d_from_radfile ( FILE *radiance_fp,
       			char *filename );
DEVA_coordinates    *DEVA_coordinates_from_filename ( char *filename );
DEVA_coordinates    *DEVA_coordinates_from_file ( FILE *file, char *name );
void		    DEVA_print_coordinates ( DEVA_coordinates *coordinates );
DEVA_coordinates    *DEVA_coordinates_new ( void );
void		    DEVA_coordinates_delete ( DEVA_coordinates *coordinates );
void		    standard_units_3D ( DEVA_XYZ_image *threeDgeom,
	                        DEVA_coordinates *coordinates );
void		    standard_units_1D ( DEVA_float_image *oneDgeom,
	                        DEVA_coordinates *coordinates );
#ifdef __cplusplus
}
#endif

#endif	/* __DEVA_READ_GEOMETRY_H */
