This directory contains those source files from version 4.2 of the
Radiance distribution needed to read and write Radiance format (.hdr)
image files.  The Radiance 4.2 source files are included without
modification, except that "tiff.h" has been renamed and the associated
include in "rtmath.h" changed, so as not to conflict with libtiff-dev,
if installed.
