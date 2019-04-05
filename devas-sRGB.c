/*
 * Conversion between sRGB, RGBf, and XYZ.
 * Float values are assumed to be in the range [0.0 -- 1.0] and linearly
 * encoded in luminance.  8-bit values are assumed to use the sRGB
 * non-linear encoding.
 *
 * Note that the assumption that float values are <= 1.0 is frequently
 * violated in Radiance images!!!
 *
 * RGBf and XYZ representations use the same scaling, which is different
 * than the convention used in RADIANCE image files.
 ******************************************************************************
 * This should probably be changed, but care would be needed to make sure all *
 * instances were handled correctly.  Currently, WHTEFFICACY scaling is done  *
 * when appropriate in radianceIO.c.					      *
 ******************************************************************************
 *
 * Uses algorithmic definition of sRGB, not color management software, and
 * ignores blackpoint, whitepoint, and other subtleties of the sRGB profile.
 *
 * Algorithm and conversion values taken from
 * <http://en.wikipedia.org/wiki/SRGB>.
 */

#include <math.h>
#include "devas-sRGB.h"

static float	sRGB_to_XYZ_matrix[3][3] = {
    			{  0.4124564,  0.3575761,  0.1804375 },
			{  0.2126729,  0.7151522,  0.0721750 },
			{  0.0193339,  0.1191920,  0.9503041 }
};


static float	XYZ_to_sRGB_matrix[3][3] = {
			{  3.2404542, -1.5371385, -0.4985314 },
			{ -0.9692660,  1.8760108,  0.0415560 },
			{  0.0556434, -0.2040259,  1.0572252 }
};

DeVAS_float
gray_to_Y ( DeVAS_gray gray )
/*
 * Use sRGB decoding from gray
 */
{
    float   CsRGBf;
    float   Clinear;

    /* don't have to worry about clipping going this direction! */

    CsRGBf = gray / 255.0;

    if ( CsRGBf <= 0.04045 ) {
	Clinear = CsRGBf / 12.92;
    } else {
	Clinear =  pow ( ( CsRGBf + 0.055 ) / ( 1.0 + 0.055 ), 2.4 );
    }

    return ( Clinear );
}

DeVAS_float
graylinear_to_Y ( DeVAS_gray gray )
/*
 * Use linear decoding from gray
 */
{
    float   Clinear;

    /* don't have to worry about clipping going this direction! */

    Clinear = gray / 255.0;

    return ( Clinear );
}

DeVAS_RGB
Y_to_sRGB ( DeVAS_float Y )
{
    DeVAS_RGB  sRGB;

    sRGB.red = sRGB.green = sRGB.blue = Y_to_gray ( Y );

    return ( sRGB );
}

DeVAS_RGB
Y_to_RGB ( DeVAS_float Y )
/*
 * linear mapping between Y and RGB values.
 */
{
    DeVAS_RGB  RGB;

    RGB.red = RGB.green = RGB.blue = Y_to_graylinear ( Y );

    return ( RGB );
}

DeVAS_gray
Y_to_gray ( DeVAS_float Y )
/*
 * Use sRGB encoding for gray value.
 */
{
    float   float_value;
    DeVAS_gray gray_value;

    /* clip going this direction (may need to tone map first!!!) */

    if ( Y < 0.0 ) {	/* always a bad thing! */
	Y = 0.0;
    }

    if ( Y > 1.0 ) {	/* probably a bad thing! */
	Y = 1.0;
    }

    if ( Y <= 0.0031308 ) {
	float_value = 12.92 * Y;
    } else {
	float_value = ( ( 1.0 + 0.055 ) * powf ( Y, 1.0 / 2.4 ) ) - 0.055;
    }

    gray_value = (int) round ( 255.0 * float_value );

    return ( gray_value );
}

DeVAS_gray
Y_to_graylinear ( DeVAS_float Y )
/*
 * Use linear encoding for gray value.
 */
{
    DeVAS_gray gray_value;

    /* clip going this direction (may need to tone map first!!!) */

    if ( Y < 0.0 ) {	/* always a bad thing! */
	Y = 0.0;
    }

    if ( Y > 1.0 ) {	/* probably a bad thing! */
	Y = 1.0;
    }

    gray_value = (int) round ( 255.0 * Y );

    return ( gray_value );
}

DeVAS_RGBf
sRGB_to_RGBf (DeVAS_RGB sRGB )
{
    DeVAS_RGBf RGBf;

    RGBf.red = gray_to_Y ( sRGB.red );
    RGBf.green = gray_to_Y ( sRGB.green );
    RGBf.blue = gray_to_Y ( sRGB.blue );

    return ( RGBf );
}

DeVAS_RGBf
RGB_to_RGBf (DeVAS_RGB RGB )
{
    DeVAS_RGBf RGBf;

    RGBf.red = graylinear_to_Y ( RGB.red );
    RGBf.green = graylinear_to_Y ( RGB.green );
    RGBf.blue = graylinear_to_Y ( RGB.blue );

    return ( RGBf );
}

DeVAS_XYZ
sRGB_to_XYZ ( DeVAS_RGB sRGB )
{
    DeVAS_XYZ  XYZ;
    DeVAS_RGBf RGBf;

    RGBf.red = gray_to_Y ( sRGB.red );
    RGBf.green = gray_to_Y ( sRGB.green );
    RGBf.blue = gray_to_Y ( sRGB.blue );

    XYZ = RGBf_to_XYZ ( RGBf );

    return ( XYZ );
}

DeVAS_XYZ
RGB_to_XYZ ( DeVAS_RGB RGB )
{
    DeVAS_XYZ  XYZ;
    DeVAS_RGBf RGBf;

    RGBf.red = graylinear_to_Y ( RGB.red );
    RGBf.green = graylinear_to_Y ( RGB.green );
    RGBf.blue = graylinear_to_Y ( RGB.blue );

    XYZ = RGBf_to_XYZ ( RGBf );

    return ( XYZ );
}

DeVAS_float
sRGB_to_Y ( DeVAS_RGB sRGB )
{
    DeVAS_float	Y;
    DeVAS_RGBf	RGBf;

    RGBf.red = gray_to_Y ( sRGB.red );
    RGBf.green = gray_to_Y ( sRGB.green );
    RGBf.blue = gray_to_Y ( sRGB.blue );

    Y = ( sRGB_to_XYZ_matrix[1][0] * RGBf.red ) +
		( sRGB_to_XYZ_matrix[1][1] * RGBf.green ) +
		( sRGB_to_XYZ_matrix[1][2] * RGBf.blue );

    return ( Y );
}

DeVAS_float
RGB_to_Y ( DeVAS_RGB RGB )
{
    DeVAS_float	Y;
    DeVAS_RGBf	RGBf;

    RGBf.red = graylinear_to_Y ( RGB.red );
    RGBf.green = graylinear_to_Y ( RGB.green );
    RGBf.blue = graylinear_to_Y ( RGB.blue );

    Y = ( sRGB_to_XYZ_matrix[1][0] * RGBf.red ) +
		( sRGB_to_XYZ_matrix[1][1] * RGBf.green ) +
		( sRGB_to_XYZ_matrix[1][2] * RGBf.blue );

    return ( Y );
}

DeVAS_RGB
RGBf_to_sRGB ( DeVAS_RGBf RGBf )
{
    DeVAS_RGB  sRGB;
    float   max_value;

    /* clip so largest RGBf value is <= 1.0 */

    max_value = fmaxf ( RGBf.red, fmaxf ( RGBf.green, RGBf.blue ) );
    if ( max_value > 1.0 ) {
	RGBf.red /= max_value;
	RGBf.green /= max_value;
	RGBf.blue /= max_value;
    }

    /* Y_to_gray clips to >= 0 */

    sRGB.red = Y_to_gray ( RGBf.red );
    sRGB.green = Y_to_gray ( RGBf.green );
    sRGB.blue = Y_to_gray ( RGBf.blue );

    return ( sRGB );
}

DeVAS_RGB
RGBf_to_RGB ( DeVAS_RGBf RGBf )
/*
 * linear RGB encoding
 */
{
    DeVAS_RGB  RGB;
    float   max_value;

    /* clip so largest RGBf value is <= 1.0 */

    max_value = fmaxf ( RGBf.red, fmaxf ( RGBf.green, RGBf.blue ) );
    if ( max_value > 1.0 ) {
	RGBf.red /= max_value;
	RGBf.green /= max_value;
	RGBf.blue /= max_value;
    }

    /* Y_to_gray clips to >= 0 */

    RGB.red = Y_to_graylinear ( RGBf.red );
    RGB.green = Y_to_graylinear ( RGBf.green );
    RGB.blue = Y_to_graylinear ( RGBf.blue );

    return ( RGB );
}

DeVAS_float
RGBf_to_Y ( DeVAS_RGBf RGBf )
{
    DeVAS_float Y;

    Y = ( sRGB_to_XYZ_matrix[1][0] * RGBf.red ) +
		( sRGB_to_XYZ_matrix[1][1] * RGBf.green ) +
		( sRGB_to_XYZ_matrix[1][2] * RGBf.blue );

    return ( Y );
}

DeVAS_XYZ
RGBf_to_XYZ ( DeVAS_RGBf RGBf )
{
    DeVAS_XYZ  XYZ;

    XYZ.X = ( sRGB_to_XYZ_matrix[0][0] * RGBf.red ) +
		( sRGB_to_XYZ_matrix[0][1] * RGBf.green ) +
		( sRGB_to_XYZ_matrix[0][2] * RGBf.blue );

    XYZ.Y = ( sRGB_to_XYZ_matrix[1][0] * RGBf.red ) +
		( sRGB_to_XYZ_matrix[1][1] * RGBf.green ) +
		( sRGB_to_XYZ_matrix[1][2] * RGBf.blue );

    XYZ.Z = ( sRGB_to_XYZ_matrix[2][0] * RGBf.red ) +
		( sRGB_to_XYZ_matrix[2][1] * RGBf.green ) +
		( sRGB_to_XYZ_matrix[2][2] * RGBf.blue );

    return ( XYZ );
}

DeVAS_RGB
XYZ_to_sRGB ( DeVAS_XYZ XYZ )
{
    return ( RGBf_to_sRGB ( XYZ_to_RGBf ( XYZ ) ) );
}

DeVAS_RGB
XYZ_to_RGB ( DeVAS_XYZ XYZ )
{
    return ( RGBf_to_RGB ( XYZ_to_RGBf ( XYZ ) ) );
}

DeVAS_RGBf
XYZ_to_RGBf ( DeVAS_XYZ XYZ )
{
    DeVAS_RGBf RGBf;

    RGBf.red = ( XYZ_to_sRGB_matrix[0][0] * XYZ.X ) +
		( XYZ_to_sRGB_matrix[0][1] * XYZ.Y ) +
		( XYZ_to_sRGB_matrix[0][2] * XYZ.Z );

    RGBf.green = ( XYZ_to_sRGB_matrix[1][0] * XYZ.X ) +
		( XYZ_to_sRGB_matrix[1][1] * XYZ.Y ) +
		( XYZ_to_sRGB_matrix[1][2] * XYZ.Z );

    RGBf.blue = ( XYZ_to_sRGB_matrix[2][0] * XYZ.X ) +
		( XYZ_to_sRGB_matrix[2][1] * XYZ.Y ) +
		( XYZ_to_sRGB_matrix[2][2] * XYZ.Z );

    return ( RGBf );
}

DeVAS_xyY
XYZ_to_xyY ( DeVAS_XYZ XYZ )
{
    DeVAS_xyY    xyY;
    float       norm;

    norm = XYZ.X + XYZ.Y + XYZ.Z;
    if ( norm <= 0.0 ) {	/* add epsilon tolerance? */
	xyY.x = xyY.y = 0.33333333;
	xyY.Y = 0.0;
    } else {
	xyY.x = XYZ.X / norm;
	xyY.y = XYZ.Y / norm;
	xyY.Y = XYZ.Y;
    }

    return ( xyY );
}

DeVAS_XYZ
xyY_to_XYZ ( DeVAS_xyY xyY )
{
    DeVAS_XYZ    XYZ;

    if ( xyY.y <= 0.0 ) {	/* add epsilon tolerance? */
	XYZ.X = XYZ.Y = XYZ.Z = 0.0;
    } else {
	XYZ.X = ( xyY.x * xyY.Y ) / xyY.y;
	XYZ.Y = xyY.Y;
	XYZ.Z = ( ( 1.0 - xyY.x - xyY.y ) * xyY.Y ) / xyY.y;
    }

    return ( XYZ );
}
