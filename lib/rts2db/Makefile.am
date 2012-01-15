# Process this file with automake to produce Makefile.in

AM_CXXFLAGS=@NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @LIBPG_CFLAGS@ @LIBXML_CFLAGS@ @JPEG_CFLAGS@ -I../

noinst_HEADERS = sqlerror.h devicedb.h target.h appdb.h rts2sqlcolumn.h observation.h observationset.h \
	rts2taruser.h rts2count.h imageset.h targetset.h plan.h planset.h rts2prop.h \
	rts2camlist.h target_auger.h messagedb.h rts2targetplanet.h targetgrb.h scriptcommands.h \
	imagesetstat.h targetell.h user.h userset.h account.h accountset.h recvals.h records.h recordsavg.h \
	augerset.h timelog.h constraints.h labels.h labellist.h simbadtarget.h

EXTRA_DIST = sqlerror.ec devicedb.ec target.ec sub_targets.ec appdb.ec rts2sqlcolumn.ec observation.ec \
	observationset.ec rts2taruser.ec rts2count.ec imageset.ec targetset.ec plan.ec planset.ec rts2prop.ec \
	rts2camlist.ec target_auger.ec messagedb.ec rts2targetplanet.ec targetgrb.ec targetell.ec \
	user.ec userset.ec account.ec accountset.ec recvals.ec records.ec recordsavg.ec \
	augerset.ec labels.ec labellist.ec

CLEANFILES = sqlerror.cpp devicedb.cpp target.cpp sub_targets.cpp appdb.cpp rts2sqlcolumn.cpp observation.cpp \
	observationset.cpp rts2taruser.cpp rts2count.cpp imageset.cpp targetset.cpp plan.cpp planset.cpp rts2prop.cpp \
	rts2camlist.cpp target_auger.cpp messagedb.cpp rts2targetplanet.cpp targetgrb.cpp targetell.cpp \
	user.cpp userset.cpp account.cpp accountset.cpp recvals.cpp records.cpp recordsavg.cpp \
	augerset.cpp labels.cpp labellist.cpp

if PGSQL

lib_LTLIBRARIES = librts2db.la

nodist_librts2db_la_SOURCES = sqlerror.cpp devicedb.cpp target.cpp sub_targets.cpp appdb.cpp rts2sqlcolumn.cpp observation.cpp \
	observationset.cpp rts2taruser.cpp rts2count.cpp imageset.cpp targetset.cpp plan.cpp planset.cpp \
	rts2prop.cpp rts2camlist.cpp target_auger.cpp messagedb.cpp rts2targetplanet.cpp targetgrb.cpp \
	targetell.cpp user.cpp userset.cpp account.cpp accountset.cpp recvals.cpp records.cpp recordsavg.cpp \
	augerset.cpp labels.cpp labellist.cpp

librts2db_la_SOURCES = simbadtarget.cpp imagesetstat.cpp constraints.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
