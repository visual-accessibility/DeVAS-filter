This directory contains those source files from version 5.0.a.12 of the
Radiance distribution needed to read and write Radiance format (.hdr)
image files.  The Radiance 5.0.a.12 source files are included without
modification, except that:

* The define of RCSid[] in the .c files has been commented out to quiet
  some compilers that squawk about unused variables.

* "tiff.h" has been renamed and the associated include in "rtmath.h"
   changed, so as not to conflict with libtiff-dev, if installed.
