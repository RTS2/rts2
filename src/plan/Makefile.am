# $Id$

bin_PROGRAMS = rts2-executor rts2-imgproc rts2-selector rts2-seltest rts2-targetinfo
noinst_HEADERS = rts2execcli.h rts2script.h rts2connimgprocess.h rts2selector.h

rts2_executor_SOURCES = executor.cpp rts2execcli.cpp rts2script.cpp
rts2_executor_CXXFLAGS = @LIBPG_CFLAGS@
rts2_executor_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2imagedb.a ../utils/librts2block.a ../utilsdb/librts2blockdb.a ../utils/objectcheck.o

rts2_imgproc_SOURCES = imgproc.cpp rts2connimgprocess.cpp 
rts2_imgproc_CXXFLAGS = @LIBPG_CFLAGS@
rts2_imgproc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2imagedb.a ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_selector_SOURCES = selector.cpp rts2selector.cpp
rts2_selector_CXXFLAGS = @LIBPG_CFLAGS@
rts2_selector_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a ../utils/objectcheck.o

rts2_seltest_SOURCES = seltest.cpp rts2selector.cpp
rts2_seltest_CXXFLAGS = @LIBPG_CFLAGS@
rts2_seltest_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a ../utils/objectcheck.o

rts2_targetinfo_SOURCES = targetinfo.cpp 
rts2_targetinfo_CXXFLAGS = @LIBPG_CFLAGS@
rts2_targetinfo_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a ../utils/objectcheck.o

EXTRA_DIST = rts2selector.ec

%.cpp: %.ec
	@ECPG@ -o $@ $^
