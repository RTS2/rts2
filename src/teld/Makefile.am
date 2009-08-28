SUBDIRS = model

noinst_HEADERS = teld.h rts2devclicop.h hms.h gem.h fork.h tgdrive.h

noinst_LIBRARIES = librts2teld.a

EXTRA_DIST =

bin_PROGRAMS = rts2-teld-gemini rts2-teld-dummy rts2-teld-mm2 rts2-teld-lx200 \
	rts2-teld-d50 rts2-teld-kolonica rts2-teld-hlohovec \
	rts2-teld-opentpl rts2-opentpl-errors rts2-teld-indi-bridge

librts2teld_a_SOURCES = teld.cpp gem.cpp fork.cpp rts2devclicop.cpp tgdrive.cpp
librts2teld_a_CXXFLAGS = @NOVA_CFLAGS@

LDADD = librts2teld.a ../utils/librts2block.a model/librts2model.a @LIB_M@ @LIB_NOVA@

AM_CXXFLAGS=@NOVA_CFLAGS@

rts2_teld_gemini_SOURCES = gemini.cpp hms.c

rts2_teld_dummy_SOURCES = dummy.cpp

rts2_teld_mm2_SOURCES = mm2.cpp hms.c

rts2_teld_lx200_SOURCES = lx200.cpp hms.c

rts2_teld_d50_SOURCES = d50.cpp

rts2_teld_kolonica_SOURCES = kolonica.cpp

rts2_teld_hlohovec_SOURCES = hlohovec.cpp

rts2_teld_opentpl_SOURCES = opentpl.cpp 
rts2_opentpl_errors_SOURCES = opentpl-errors.cpp

rts2_teld_indi_bridge_SOURCES = indibridge.cpp hms.c
rts2_teld_indi_bridge_LDADD = -L../libindi -lindi -lz @LIB_PTHREAD@ ${LDADD}
rts2_teld_indi_bridge_CPPFLAGS = -I../libindi

if PGSQL
bin_PROGRAMS += rts2-telmodeltest

rts2_telmodeltest_SOURCES = telmodeltest.cpp
rts2_telmodeltest_LDADD = ../writers/librts2imagedb.a ../utilsdb/librts2blockdb.a ${LDADD} @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CFITSIO@
else
EXTRA_DIST += telmodeltest.cpp
endif

if PARALIB
bin_PROGRAMS += rts2-teld-paramount
rts2_teld_paramount_SOURCES = paramount.cpp
rts2_teld_paramount_LDFLAGS = @LIB_PARA@
rts2_teld_paramount_CPPFLAGS = @INC_PARA@
else
EXTRA_DIST += paramount.cpp
endif
