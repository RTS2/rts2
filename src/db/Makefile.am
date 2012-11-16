LDADD = -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2fits -lrts2image -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@
AM_CXXFLAGS = @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ -I../../include

bin_PROGRAMS = rts2-image rts2-user rts2-horizon

rts2_image_SOURCES = appimagemanip.cpp

rts2_horizon_SOURCES = horizonapp.cpp

EXTRA_DIST = airmasscale.ec
CLEANFILES = airmasscale.cpp

if PGSQL

bin_PROGRAMS += rts2-targetinfo rts2-nightreport rts2-targetlist rts2-target rts2-tpm rts2-obsinfo rts2-newtarget rts2-simbadinfo rts2-plan rts2-airmasscale
PG_LDADD = -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2imagedb @LIB_CRYPT@ ${LDADD} 

noinst_HEADERS = rts2targetapp.h

rts2_image_LDADD = ${PG_LDADD}

rts2_targetinfo_SOURCES = targetinfo.cpp
rts2_targetinfo_CXXFLAGS = ${AM_CXXFLAGS} @JPEG_CFLAGS@ -I../../include
rts2_targetinfo_LDADD = ${PG_LDADD} @LIB_JPEG@

rts2_nightreport_SOURCES = nightreport.cpp
rts2_nightreport_LDADD = ${PG_LDADD}

rts2_targetlist_SOURCES = targetlist.cpp
rts2_targetlist_LDADD = ${PG_LDADD}

rts2_target_SOURCES = target.cpp
rts2_target_LDADD = ${PG_LDADD}

rts2_tpm_SOURCES = tpm.cpp
rts2_tpm_LDADD = ${PG_LDADD}

rts2_obsinfo_SOURCES = obsinfo.cpp
rts2_obsinfo_LDADD = ${PG_LDADD}

rts2_user_SOURCES = user.cpp
rts2_user_LDADD = ${PG_LDADD}

rts2_newtarget_SOURCES = newtarget.cpp rts2targetapp.cpp
rts2_newtarget_CXXFLAGS = @LIBXML_CFLAGS@ ${AM_CXXFLAGS} -I../../include
rts2_newtarget_LDADD = ${PG_LDADD}

rts2_simbadinfo_SOURCES = simbadinfo.cpp rts2targetapp.cpp
rts2_simbadinfo_CXXFLAGS = @LIBXML_CFLAGS@ ${AM_CXXFLAGS} -I../../include
rts2_simbadinfo_LDADD = ${PG_LDADD}

rts2_plan_SOURCES = planapp.cpp
rts2_plan_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@ -I../../include
rts2_plan_LDADD = ${PG_LDADD}

nodist_rts2_airmasscale_SOURCES = airmasscale.cpp
rts2_airmasscale_LDADD = ${PG_LDADD}

%.cpp: %.ec
	@ECPG@ -o $@ $^

EXTRA_DIST += usernondb.cpp

else

rts2_image_LDADD = ${LDADD}

rts2_user_SOURCES = usernondb.cpp
rts2_user_LDADD = -lrts2users ${LDADD} @LIB_CRYPT@

EXTRA_DIST += targetinfo.cpp nightreport.cpp targetlist.cpp target.cpp tpm.cpp obsinfo.cpp user.cpp newtarget.cpp rts2targetapp.cpp simbadinfo.cpp planapp.cpp airmasscale.ec

endif
