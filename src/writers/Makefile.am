# $Id$

noinst_LIBRARIES = librts2fits.a

noinst_HEADERS = fits.h process_image.h

librts2fits_a_SOURCES = fits.c process_image.c

bin_PROGRAMS = rts2-median-filter

rts2_median_filter_SOURCES = median_filter.c
rts2_median_filter_LDFLAGS = @LIB_CFITSIO@
