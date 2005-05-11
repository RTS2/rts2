# $Id$

bin_PROGRAMS = rts2-executor rts2-imgproc

noinst_HEADERS = target.h rts2execcli.h rts2script.h

rts2_executor_SOURCES = executor.cpp target.cpp sub_targets.cpp rts2execcli.cpp rts2script.cpp
rts2_executor_CXXFLAGS = @LIBPG_CFLAGS@
rts2_executor_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2image.a ../utils/librts2block.a ../utils/objectcheck.o

rts2_imgproc_SOURCES = imgproc.cpp
rts2_imgproc_CXXFLAGS = @LIBPG_CFLAGS@
rts2_imgproc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a

EXTRA_DIST = target.ec sub_targets.ec rts2execcli.h

%.cpp: %.ec
	@ECPG@ -o $@ $^
