# $Id$

bin_PROGRAMS = rts2-scriptexec rts2-scriptor
noinst_HEADERS = rts2execcli.h rts2script.h rts2connimgprocess.h rts2selector.h rts2scriptelement.h \
		 rts2devscript.h rts2devcliphot.h rts2spiral.h rts2scriptblock.h rts2scriptguiding.h \
		 rts2execclidb.h rts2scriptelementacquire.h scriptexec.h rts2targetscr.h \
		 rts2setarget.h rts2sehex.h rts2swaitfor.h rts2scriptinterface.h

EXTRA_DIST = rts2selector.ec rts2devcliphot.ec rts2execcli.cpp rts2script.cpp rts2scriptelement.cpp \
		rts2devscript.cpp rts2connimgprocess.cpp rts2scriptelementacquire.cpp rts2spiral.cpp \
		rts2scriptblock.cpp rts2scriptguiding.cpp rts2targetscr.cpp rts2setarget.cpp rts2sehex.cpp \
		rts2swaitfor.cpp rts2scriptinterface.cpp

CLEANFILES = rts2selector.cpp rts2devcliphot.cpp

rts2_scriptexec_SOURCES = scriptexec.cpp 
rts2_scriptexec_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_scriptexec_LDADD = rts2execcli.o rts2script.o rts2scriptelement.o rts2devscript.o rts2spiral.o \
		rts2scriptblock.o rts2scriptguiding.o rts2targetscr.o rts2setarget.o rts2sehex.o rts2swaitfor.o \
		rts2scriptinterface.o

# LDFLAGS are defined in PGSQL/nonPGSQL

rts2_scriptor_SOURCES = scriptor.cpp 
rts2_scriptor_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_scriptor_LDADD = rts2execcli.o rts2script.o rts2scriptelement.o rts2devscript.o rts2spiral.o \
		rts2scriptblock.o rts2scriptguiding.o rts2targetscr.o rts2setarget.o rts2sehex.o rts2swaitfor.o \
		rts2scriptinterface.o

if PGSQL

rts2_scriptexec_CXXFLAGS += @LIBPG_CFLAGS@
rts2_scriptexec_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@
rts2_scriptexec_LDADD += rts2scriptelementacquire.o rts2connimgprocess.o

rts2_scriptor_CXXFLAGS += @LIBPG_CFLAGS@
rts2_scriptor_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@
rts2_scriptor_LDADD += rts2scriptelementacquire.o rts2connimgprocess.o

bin_PROGRAMS += rts2-executor rts2-imgproc rts2-selector rts2-seltest rts2-targetinfo rts2-marchive \
	       rts2-plan

nodist_rts2_executor_SOURCES = rts2devcliphot.cpp
rts2_executor_SOURCES = executor.cpp rts2execclidb.cpp
rts2_executor_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_executor_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@
rts2_executor_LDADD = rts2execcli.o rts2script.o rts2scriptelement.o rts2devscript.o rts2connimgprocess.o rts2scriptelementacquire.o \
		rts2spiral.o rts2scriptblock.o rts2scriptguiding.o rts2setarget.o rts2sehex.o rts2swaitfor.o

rts2_imgproc_SOURCES = imgproc.cpp 
rts2_imgproc_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_imgproc_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@
rts2_imgproc_LDADD = rts2connimgprocess.o

nodist_rts2_selector_SOURCES = rts2selector.cpp
rts2_selector_SOURCES = selector.cpp
rts2_selector_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_selector_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

nodist_rts2_seltest_SOURCES = rts2selector.cpp
rts2_seltest_SOURCES = seltest.cpp
rts2_seltest_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_seltest_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

rts2_targetinfo_SOURCES = targetinfo.cpp
rts2_targetinfo_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_targetinfo_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@
rts2_targetinfo_LDADD = rts2connimgprocess.o rts2script.o rts2scriptelement.o rts2scriptelementacquire.o rts2spiral.o rts2scriptblock.o \
		rts2scriptguiding.o rts2setarget.o rts2sehex.o rts2swaitfor.o

rts2_marchive_SOURCES = marchive.cpp
rts2_marchive_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_marchive_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

rts2_plan_SOURCES = planapp.cpp
rts2_plan_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_plan_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

rts2_scriptexec_LDFLAGS = ../writers/librts2image.a ../utils/librts2block.a @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@
rts2_scriptor_LDFLAGS = ../writers/librts2image.a ../utils/librts2block.a @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

endif
