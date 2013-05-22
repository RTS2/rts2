bin_PROGRAMS = rts2-scriptexec rts2-scriptor rts2-imgproc
noinst_HEADERS = rts2devcliphot.h scriptexec.h selector.h
		 

EXTRA_DIST = selector.ec rts2devcliphot.ec

CLEANFILES = selector.cpp rts2devcliphot.cpp

PLAN_STDLIBS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

rts2_scriptexec_SOURCES = scriptexec.cpp 
rts2_scriptexec_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ @LIBNCURSES_CFLAGS@ -I../../include

# LDFLAGS are defined in PGSQL/nonPGSQL

rts2_scriptor_SOURCES = scriptor.cpp 
rts2_scriptor_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ -I../../include

if PGSQL

PG_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CRYPT@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_scriptexec_CXXFLAGS += @LIBPG_CFLAGS@ -I../../include
rts2_scriptexec_LDADD = ${PG_LDADD}  @LIBNCURSES_LDFLAGS@
rts2_scriptor_CXXFLAGS += @LIBPG_CFLAGS@ -I../../include
rts2_scriptor_LDADD = ${PG_LDADD}

bin_PROGRAMS += rts2-executor rts2-selector rts2-seltest rts2-marchive 

nodist_rts2_executor_SOURCES = rts2devcliphot.cpp
rts2_executor_SOURCES = executor.cpp
rts2_executor_CXXFLAGS = ${PLAN_STDLIBS} -I../../include
rts2_executor_LDADD = ${PG_LDADD}

rts2_imgproc_SOURCES = imgproc.cpp 
rts2_imgproc_CXXFLAGS = ${PLAN_STDLIBS} -I../../include
rts2_imgproc_LDADD = ${PG_LDADD}

nodist_rts2_selector_SOURCES = selector.cpp
rts2_selector_SOURCES = selectordev.cpp
rts2_selector_CXXFLAGS = ${PLAN_STDLIBS} -I../../include
rts2_selector_LDADD = ${PG_LDADD}

nodist_rts2_seltest_SOURCES = selector.cpp
rts2_seltest_SOURCES = seltest.cpp
rts2_seltest_CXXFLAGS = @LIBNCURSES_CFLAGS@ ${PLAN_STDLIBS} -I../../include
rts2_seltest_LDADD = ${PG_LDADD}

rts2_marchive_SOURCES = marchive.cpp
rts2_marchive_CXXFLAGS = ${PLAN_STDLIBS} -I../../include
rts2_marchive_LDADD = ${PG_LDADD}

.ec.cpp:
	@ECPG@ -o $@ $^

else

rts2_scriptexec_LDFLAGS = -L../../lib/rts2script -lrts2script -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@ @LIBNCURSES_LDFLAGS@
rts2_scriptor_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

rts2_imgproc_SOURCES = imgproc.cpp 
rts2_imgproc_CXXFLAGS = @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ -I../../include
rts2_imgproc_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

EXTRA_DIST += executor.cpp selectordev.cpp seltest.cpp marchive.cpp

endif
