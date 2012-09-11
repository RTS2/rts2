# Process this file with automake to produce Makefile.in

AM_CXXFLAGS=@NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @LIBPG_CFLAGS@ @LIBXML_CFLAGS@ @JPEG_CFLAGS@ -I../../include -I../

noinst_HEADERS = rts2prop.h \
	rts2targetplanet.h targetgrb.h \
	augerset.h labellist.h

EXTRA_DIST = sqlerror.ec devicedb.ec target.ec sub_targets.ec appdb.ec sqlcolumn.ec observation.ec \
	observationset.ec taruser.ec rts2count.ec imageset.ec targetset.ec plan.ec planset.ec rts2prop.ec \
	camlist.ec target_auger.ec messagedb.ec rts2targetplanet.ec targetgrb.ec targetell.ec \
	user.ec userset.ec account.ec accountset.ec recvals.ec records.ec recordsavg.ec \
	augerset.ec labels.ec labellist.ec

CLEANFILES = sqlerror.cpp devicedb.cpp target.cpp sub_targets.cpp appdb.cpp sqlcolumn.cpp observation.cpp \
	observationset.cpp taruser.cpp rts2count.cpp imageset.cpp targetset.cpp plan.cpp planset.cpp rts2prop.cpp \
	camlist.cpp target_auger.cpp messagedb.cpp rts2targetplanet.cpp targetgrb.cpp targetell.cpp \
	user.cpp userset.cpp account.cpp accountset.cpp recvals.cpp records.cpp recordsavg.cpp \
	augerset.cpp labels.cpp labellist.cpp

if PGSQL

lib_LTLIBRARIES = librts2db.la

nodist_librts2db_la_SOURCES = sqlerror.cpp devicedb.cpp target.cpp sub_targets.cpp appdb.cpp sqlcolumn.cpp observation.cpp \
	observationset.cpp taruser.cpp rts2count.cpp imageset.cpp targetset.cpp plan.cpp planset.cpp \
	rts2prop.cpp camlist.cpp target_auger.cpp messagedb.cpp rts2targetplanet.cpp targetgrb.cpp \
	targetell.cpp user.cpp userset.cpp account.cpp accountset.cpp recvals.cpp records.cpp recordsavg.cpp \
	augerset.cpp labels.cpp labellist.cpp

librts2db_la_SOURCES = simbadtarget.cpp mpectarget.cpp imagesetstat.cpp constraints.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

EXTRA_DIST += simbadtarget.cpp mpectarget.cpp imagesetstat.cpp constraints.cpp

endif
