# $Id$

bin_PROGRAMS = rts2-executor rts2-imgproc rts2-selector rts2-seltest rts2-targetinfo rts2-marchive
noinst_HEADERS = rts2execcli.h rts2script.h rts2connimgprocess.h rts2selector.h rts2scriptelement.h \
		 rts2devscript.h rts2devcliphot.h rts2spiral.h rts2scriptblock.h

EXTRA_DIST = rts2selector.ec rts2devcliphot.ec

rts2_executor_SOURCES = executor.cpp rts2execcli.cpp rts2script.cpp rts2scriptelement.cpp rts2connimgprocess.cpp rts2devscript.cpp \
			rts2devcliphot.cpp rts2spiral.cpp rts2scriptblock.cpp
rts2_executor_CXXFLAGS = @LIBPG_CFLAGS@
rts2_executor_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2imagedb.a ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_imgproc_SOURCES = imgproc.cpp rts2connimgprocess.cpp 
rts2_imgproc_CXXFLAGS = @LIBPG_CFLAGS@
rts2_imgproc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2imagedb.a ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_selector_SOURCES = selector.cpp rts2selector.cpp
rts2_selector_CXXFLAGS = @LIBPG_CFLAGS@
rts2_selector_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_seltest_SOURCES = seltest.cpp rts2selector.cpp
rts2_seltest_CXXFLAGS = @LIBPG_CFLAGS@
rts2_seltest_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_targetinfo_SOURCES = targetinfo.cpp 
rts2_targetinfo_CXXFLAGS = @LIBPG_CFLAGS@
rts2_targetinfo_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_marchive_SOURCES = marchive.cpp
rts2_marchive_CXXFLAGS = @LIBPG_CFLAGS@
rts2_marchive_LDFLAGS = @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2imagedb.a ../utils/librts2block.a ../utilsdb/librts2blockdb.a

%.cpp: %.ec
	@ECPG@ -o $@ $^
