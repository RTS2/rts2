# $Id$

bin_PROGRAMS = rts2-scriptexec rts2-scriptor rts2-imgproc
noinst_HEADERS = rts2execcli.h script.h connimgprocess.h rts2selector.h element.h \
		 devscript.h rts2devcliphot.h rts2spiral.h elementblock.h elementguiding.h \
		 rts2execclidb.h elementacquire.h scriptexec.h rts2targetscr.h \
		 elementtarget.h elementhex.h elementwaitfor.h rts2scriptinterface.h operands.h elementexe.h \
		 printtarget.h connexe.h connselector.h executorque.h

EXTRA_DIST = rts2selector.ec rts2devcliphot.ec

CLEANFILES = rts2selector.cpp rts2devcliphot.cpp

PLAN_STDLIBS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

noinst_LIBRARIES = libscript.a

libscript_a_SOURCES = rts2execcli.cpp script.cpp connimgprocess.cpp element.cpp devscript.cpp rts2spiral.cpp \
		elementblock.cpp elementguiding.cpp rts2targetscr.cpp elementtarget.cpp elementhex.cpp elementwaitfor.cpp \
		rts2scriptinterface.cpp operands.cpp elementexe.cpp printtarget.cpp connexe.cpp connselector.cpp connselector.h
libscript_a_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@


rts2_scriptexec_SOURCES = scriptexec.cpp 
rts2_scriptexec_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

# LDFLAGS are defined in PGSQL/nonPGSQL

rts2_scriptor_SOURCES = scriptor.cpp 
rts2_scriptor_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

if PGSQL

libscript_a_SOURCES += rts2execclidb.cpp elementacquire.cpp

rts2_scriptexec_CXXFLAGS += @LIBPG_CFLAGS@
rts2_scriptexec_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_scriptor_CXXFLAGS += @LIBPG_CFLAGS@
rts2_scriptor_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

bin_PROGRAMS += rts2-executor rts2-selector rts2-seltest rts2-targetinfo rts2-marchive \
	       rts2-plan 

nodist_rts2_executor_SOURCES = rts2devcliphot.cpp
rts2_executor_SOURCES = executor.cpp executorque.cpp
rts2_executor_CXXFLAGS = ${PLAN_STDLIBS}
rts2_executor_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_imgproc_SOURCES = imgproc.cpp 
rts2_imgproc_CXXFLAGS = ${PLAN_STDLIBS}
rts2_imgproc_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

nodist_rts2_selector_SOURCES = rts2selector.cpp
rts2_selector_SOURCES = selector.cpp
rts2_selector_CXXFLAGS = ${PLAN_STDLIBS}
rts2_selector_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

nodist_rts2_seltest_SOURCES = rts2selector.cpp
rts2_seltest_SOURCES = seltest.cpp
rts2_seltest_CXXFLAGS = ${PLAN_STDLIBS}
rts2_seltest_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_targetinfo_SOURCES = targetinfo.cpp
rts2_targetinfo_CXXFLAGS = ${PLAN_STDLIBS}
rts2_targetinfo_LDADD = libscript.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_marchive_SOURCES = marchive.cpp
rts2_marchive_CXXFLAGS = ${PLAN_STDLIBS}
rts2_marchive_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_plan_SOURCES = planapp.cpp
rts2_plan_CXXFLAGS = ${PLAN_STDLIBS}
rts2_plan_LDADD = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBXML_LIBS@ @LIBPG_CFLAGS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

rts2_scriptexec_LDFLAGS = libscript.a ../writers/librts2image.a ../utils/librts2block.a @LIBXML_LIBS@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@
rts2_scriptor_LDADD = libscript.a ../writers/librts2image.a ../utils/librts2block.a @LIBXML_LIBS@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_imgproc_SOURCES = imgproc.cpp 
rts2_imgproc_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
rts2_imgproc_LDADD = libscript.a ../writers/librts2image.a ../utils/librts2block.a @LIBXML_LIBS@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

EXTRA_DIST += excutor.cpp executorque.cpp selector.cpp seltest.cpp targetinfo.cpp marchive.cpp planapp.cpp queue-selector.cpp

endif
