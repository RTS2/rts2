# $Id$

bin_PROGRAMS = rts2-executor

noinst_HEADERS = target.h rts2execcli.h rts2script.h

rts2_executor_SOURCES = executor.cpp target.cpp sub_targets.cpp rts2execcli.cpp rts2script.cpp
rts2_executor_CXXFLAGS = @LIBPG_CFLAGS@
rts2_executor_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_PTHREAD@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2image.a ../utils/librts2block.a ../utils/objectcheck.o

EXTRA_DIST = target.ec sub_targets.ec rts2execcli.h

%.cpp: %.ec
	@ECPG@ -o $@ $^
