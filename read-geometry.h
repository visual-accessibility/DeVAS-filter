/*
 * Read DeVAS geometry files.
 *
 * Canonical units for position/distance are centimeter.
 */

#ifndef	__DeVAS_READ_GEOMETRY_H
#define	__DeVAS_READ_GEOMETRY_H

#include <stdlib.h>
#include <stdio.h>
#include "devas-image.h"
#include "radiance/fvect.h"     /* for FVECT type */
#include "radiance/view.h"      /* for VIEW structure */
#include "devas-license.h"       /* DeVAS open source license */

typedef enum { unknown_unit, centimeters, meters, inches, feet }
							DeVAS_geom_units;

typedef struct {
    DeVAS_geom_units units;
    double	    convert_to_centimeters;
    VIEW	    view;
} DeVAS_coordinates;

#ifdef __cplusplus
extern "C" {
#endif

VIEW		    devas_get_VIEW_from_filename ( char *filename, int *n_rows,
			int *c_cols );
void		    devas_print_VIEW ( VIEW view );
int		    DeVAS_geom_dim_from_radfilename ( char *filename );
int		    DeVAS_geom_dim_from_radfile ( FILE *radiance_fp,
       			char *filename );
DeVAS_XYZ_image	    *DeVAS_geom3d_from_radfilename ( char *filename );
DeVAS_XYZ_image	    *DeVAS_geom3d_from_radfile ( FILE *radiance_fp,
			char *filename );
DeVAS_float_image    *DeVAS_geom1d_from_radfilename ( char *filename );
DeVAS_float_image    *DeVAS_geom1d_from_radfile ( FILE *radiance_fp,
       			char *filename );
DeVAS_coordinates    *DeVAS_coordinates_from_filename ( char *filename );
DeVAS_coordinates    *DeVAS_coordinates_from_file ( FILE *file, char *name );
void		    DeVAS_print_coordinates ( DeVAS_coordinates *coordinates );
DeVAS_coordinates    *DeVAS_coordinates_new ( void );
void		    DeVAS_coordinates_delete ( DeVAS_coordinates *coordinates );
void		    standard_units_3D ( DeVAS_XYZ_image *threeDgeom,
	                        DeVAS_coordinates *coordinates );
void		    standard_units_1D ( DeVAS_float_image *oneDgeom,
	                        DeVAS_coordinates *coordinates );
#ifdef __cplusplus
}
#endif

#endif	/* __DeVAS_READ_GEOMETRY_H */
