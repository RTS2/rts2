noinst_HEADERS = pier-collision.h hms.h tellx200.h

lib_LTLIBRARIES = librts2pier-collision.la librts2tellx200.la

EXTRA_DIST = 

bin_PROGRAMS = rts2-teld-gemini rts2-teld-dummy rts2-teld-mm2 rts2-teld-lx200 \
	rts2-teld-d50 rts2-teld-kolonica rts2-teld-hlohovec \
	rts2-teld-opentpl rts2-opentpl-errors \
	rts2-teld-trencin rts2-teld-apgto rts2-teld-lx200test rts2-teld-nexstar \
	rts2-teld-lx200gps rts2-teld-meade rts2-teld-indi

librts2tellx200_la_SOURCES = tellx200.cpp hms.c
librts2tellx200_l_CXXFLAGS = @NOVA_CFLAGS@

librts2pier_collision_la_SOURCES = pier-collision.cpp
librts2pier_collision_la_CXXFLAGS = @NOVA_CFLAGS@

LDADD = -L../../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@

AM_CXXFLAGS=@NOVA_CFLAGS@

rts2_teld_gemini_SOURCES = gemini.cpp
rts2_teld_gemini_LDADD = -lrts2tellx200 ${LDADD}

rts2_teld_dummy_SOURCES = dummy.cpp

rts2_teld_mm2_SOURCES = mm2.cpp
rts2_teld_mm2_LDADD = -lrts2tellx200 ${LDADD}

rts2_teld_lx200_SOURCES = lx200.cpp
rts2_teld_lx200_LDADD = -lrts2tellx200 ${LDADD}

rts2_teld_meade_SOURCES = meade.cpp
rts2_teld_meade_LDADD = -lrts2tellx200 ${LDADD}

rts2_teld_indi_SOURCES = indibridge.cpp hms.c
rts2_teld_indi_LDADD = -L../libindi -lindi -lz @LIB_PTHREAD@ ${LDADD}
rts2_teld_indi_CPPFLAGS = -I../libindi ${CPPFLAGS}

rts2_teld_lx200gps_SOURCES = lx200gps.cpp
rts2_teld_lx200gps_LDADD = -lrts2tellx200 ${LDADD}

rts2_teld_lx200test_SOURCES = lx200test.cpp
rts2_teld_lx200test_LDADD = -lrts2tellx200 -lrts2pier-collision ${LDADD}

# comment this and uncomment next for alternative APT-GOTO driver

rts2_teld_apgto_SOURCES = apgto.cpp
rts2_teld_apgto_LDADD = -lrts2tellx200 -lrts2pier-collision ${LDADD}

#rts2_teld_apgto_SOURCES = apgto_pk.cpp
#rts2_teld_apgto_LDADD = -lrts2tellx200 -lrts2pier-collision ${LDADD}

rts2_teld_d50_SOURCES = d50.cpp

rts2_teld_kolonica_SOURCES = kolonica.cpp

rts2_teld_hlohovec_SOURCES = hlohovec.cpp

rts2_teld_opentpl_SOURCES = opentpl.cpp 
rts2_opentpl_errors_SOURCES = opentpl-errors.cpp

rts2_teld_trencin_SOURCES = trencin.cpp

rts2_teld_nexstar_SOURCES = nexstar.cpp

if PGSQL
bin_PROGRAMS += rts2-telmodeltest

rts2_telmodeltest_SOURCES = telmodeltest.cpp
rts2_telmodeltest_LDADD = -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2/model -lrts2model -L../../lib/rts2db -lrts2db -L../../lib/xmlrpc++ -lrts2xmlrpc ${LDADD} @LIBPG_LIBS@ @LIB_ECPG@ @LIB_JPEG@ @LIB_CFITSIO@ @LIBXML_LIBS@
rts2_telmodeltest_CPPFLAGS = @JPEG_CFLAGS@ @CFITSIO_CFLAGS@ @LIBXML_CFLAGS@

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

if MDMTCS
bin_PROGRAMS += rts2-teld-mdm rts2-teld-bait

rts2_teld_mdm_SOURCES = mdm.cpp
rts2_teld_mdm_LDFLAGS = @LIB_MDMTCS@
rts2_teld_mdm_CPPFLAGS = @INC_MDMTCS@

rts2_teld_bait_SOURCES = bait.cpp
rts2_teld_bait_LDFLAGS = @LIB_MDMTCS@
rts2_teld_bait_CPPFLAGS = @INC_MDMTCS@
else
EXTRA_DIST += mdm.cpp bait.cpp
endif
