EXTRA_DIST = 

SUBDIRS = irait

bin_PROGRAMS = rts2-teld-gemini rts2-teld-dummy rts2-teld-mm2 rts2-teld-lx200 \
	rts2-teld-d50 rts2-teld-kolonica rts2-teld-hlohovec \
	rts2-teld-opentpl rts2-opentpl-errors \
	rts2-teld-trencin rts2-teld-apgto rts2-teld-apgto-pk rts2-teld-lx200test rts2-teld-nexstar \
	rts2-teld-lx200gps rts2-teld-lx200focgps rts2-teld-meade rts2-teld-indi rts2-teld-sitech-gem \
	rts2-teld-irait

noinst_HEADERS = sitech.h

TESTS = check_gem_hko
check_PROGRAMS = check_gem_hko

LDADD = -L../../lib/rts2tel -lrts2tel -L../../lib/pluto -lpluto -L../../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@

AM_CXXFLAGS=@NOVA_CFLAGS@ -I../../include

rts2_teld_gemini_SOURCES = gemini.cpp
rts2_teld_gemini_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 ${LDADD}

rts2_teld_dummy_SOURCES = dummy.cpp

rts2_teld_mm2_SOURCES = mm2.cpp
rts2_teld_mm2_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 ${LDADD}

rts2_teld_lx200_SOURCES = lx200.cpp
rts2_teld_lx200_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 ${LDADD}

rts2_teld_meade_SOURCES = meade.cpp
rts2_teld_meade_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 ${LDADD}

rts2_teld_indi_SOURCES = indibridge.cpp
rts2_teld_indi_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 -L../../lib/libindi -lindi -lz @LIB_PTHREAD@ ${LDADD}
rts2_teld_indi_CPPFLAGS = -I../../lib/libindi ${CPPFLAGS} -I../../include

rts2_teld_lx200gps_SOURCES = lx200gps.cpp
rts2_teld_lx200gps_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 ${LDADD}

rts2_teld_lx200focgps_SOURCES = lx200focgps.cpp
rts2_teld_lx200focgps_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 ${LDADD}

rts2_teld_lx200test_SOURCES = lx200test.cpp
rts2_teld_lx200test_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 -lrts2pier-collision ${LDADD}

# comment this and uncomment next for alternative APT-GOTO driver

rts2_teld_apgto_SOURCES = apgto.cpp
rts2_teld_apgto_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 -lrts2pier-collision ${LDADD}

rts2_teld_apgto_pk_SOURCES = apgto_pk.cpp
rts2_teld_apgto_pk_LDADD = -L../../lib/rts2lx200 -lrts2tellx200 -lrts2pier-collision ${LDADD}

rts2_teld_d50_SOURCES = d50.cpp

rts2_teld_kolonica_SOURCES = kolonica.cpp

rts2_teld_hlohovec_SOURCES = hlohovec.cpp

rts2_teld_opentpl_SOURCES = opentpl.cpp 
rts2_opentpl_errors_SOURCES = opentpl-errors.cpp

rts2_teld_trencin_SOURCES = trencin.cpp

rts2_teld_nexstar_SOURCES = nexstar.cpp

rts2_teld_sitech_gem_SOURCES = sitech_gem.cpp
rts2_teld_sitech_gem_LDADD = -L../../lib/rts2tel -lrts2sitech ${LDADD}

rts2_teld_irait_SOURCES = irait.cpp
rts2_teld_irait_LDADD = -Lirait -lirait ${LIB_PTHREAD} ${LDADD}

if PGSQL
bin_PROGRAMS += rts2-telmodeltest

rts2_telmodeltest_SOURCES = telmodeltest.cpp
rts2_telmodeltest_LDADD = -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2db -lrts2db -L../../lib/xmlrpc++ -lrts2xmlrpc ${LDADD} @LIBPG_LIBS@ @LIB_ECPG@ @LIB_JPEG@ @LIB_CFITSIO@ @LIBXML_LIBS@ @LIB_CRYPT@
rts2_telmodeltest_CPPFLAGS = @JPEG_CFLAGS@ @CFITSIO_CFLAGS@ @LIBXML_CFLAGS@ -I../../include

else
EXTRA_DIST += telmodeltest.cpp
endif

if PARALIB
bin_PROGRAMS += rts2-teld-paramount
rts2_teld_paramount_SOURCES = paramount.cpp
rts2_teld_paramount_LDFLAGS = @LIB_PARA@
rts2_teld_paramount_CPPFLAGS = @INC_PARA@ -I../../include
else
EXTRA_DIST += paramount.cpp
endif

if MDMTCS
bin_PROGRAMS += rts2-teld-mdm rts2-teld-bait

rts2_teld_mdm_SOURCES = mdm.cpp
rts2_teld_mdm_LDFLAGS = @LIB_MDMTCS@
rts2_teld_mdm_CPPFLAGS = @INC_MDMTCS@ -I../../include

rts2_teld_bait_SOURCES = bait.cpp
rts2_teld_bait_LDFLAGS = @LIB_MDMTCS@
rts2_teld_bait_CPPFLAGS = @INC_MDMTCS@ -I../../include
else
EXTRA_DIST += mdm.cpp bait.cpp
endif

check_gem_hko_SOURCES = check_gem_hko.cpp
check_gem_hko_CPPFLAGS = @CHECK_CFLAGS@ ${CPPFLAGS} -I../../include
check_gem_hko_LDADD = @CHECK_LIBS@ ${LDADD}
