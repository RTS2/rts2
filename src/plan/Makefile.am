# $Id$

bin_PROGRAMS = rts2-scriptexec
noinst_HEADERS = rts2execcli.h rts2script.h rts2connimgprocess.h rts2selector.h rts2scriptelement.h \
		 rts2devscript.h rts2devcliphot.h rts2spiral.h rts2scriptblock.h rts2scriptguiding.h \
		 rts2execclidb.h rts2scriptelementacquire.h scriptexec.h rts2targetscr.h \
		 rts2setarget.h rts2sehex.h

EXTRA_DIST = rts2selector.ec rts2devcliphot.ec

CLEANFILES = rts2selector.cpp rts2devcliphot.cpp

rts2_scriptexec_SOURCES = scriptexec.cpp rts2execcli.cpp rts2script.cpp rts2scriptelement.cpp rts2devscript.cpp \
			rts2spiral.cpp rts2scriptblock.cpp rts2scriptguiding.cpp rts2targetscr.cpp \
			rts2setarget.cpp rts2sehex.cpp
rts2_scriptexec_CXXFLAGS = -DNOT_PGSQL @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_scriptexec_LDFLAGS = @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../writers/librts2image.a ../utils/librts2block.a 

if PGSQL

bin_PROGRAMS += rts2-executor rts2-imgproc rts2-selector rts2-seltest rts2-targetinfo rts2-marchive \
	       rts2-plan

rts2_executor_SOURCES = executor.cpp rts2execcli.cpp rts2script.cpp rts2scriptelement.cpp rts2connimgprocess.cpp rts2devscript.cpp \
			rts2devcliphot.cpp rts2spiral.cpp rts2scriptblock.cpp rts2scriptguiding.cpp rts2execclidb.cpp rts2scriptelementacquire.cpp \
			rts2setarget.cpp rts2sehex.cpp
rts2_executor_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_executor_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a 

rts2_imgproc_SOURCES = imgproc.cpp rts2connimgprocess.cpp 
rts2_imgproc_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_imgproc_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a 

rts2_selector_SOURCES = selector.cpp rts2selector.cpp
rts2_selector_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_selector_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_seltest_SOURCES = seltest.cpp rts2selector.cpp
rts2_seltest_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_seltest_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a 

rts2_targetinfo_SOURCES = targetinfo.cpp rts2script.cpp rts2scriptelement.cpp rts2scriptblock.cpp rts2scriptguiding.cpp rts2connimgprocess.cpp \
			  rts2spiral.cpp rts2scriptelementacquire.cpp rts2setarget.cpp rts2sehex.cpp
rts2_targetinfo_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_targetinfo_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a

rts2_marchive_SOURCES = marchive.cpp
rts2_marchive_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_marchive_LDFLAGS = @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a 

rts2_plan_SOURCES = planapp.cpp
rts2_plan_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_plan_LDFLAGS = @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a 

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
