# Process this file with automake to produce Makefile.in

AM_CXXFLAGS=@NOVA_CFLAGS@ @CFITSIO_CFLAGS@

noinst_HEADERS = sqlerror.h rts2devicedb.h target.h rts2appdb.h rts2sqlcolumn.h observation.h observationset.h \
	rts2taruser.h rts2count.h imageset.h targetset.h rts2plan.h rts2planset.h rts2prop.h \
	rts2camlist.h target_auger.h messagedb.h rts2targetplanet.h rts2targetgrb.h scriptcommands.h \
	imagesetstat.h rts2targetell.h user.h userset.h account.h accountset.h recvals.h records.h recordsavg.h \
	augerset.h timelog.h constraints.h

EXTRA_DIST = sqlerror.ec rts2devicedb.ec target.ec sub_targets.ec rts2appdb.ec airmasscale.ec rts2sqlcolumn.ec observation.ec \
	observationset.ec rts2taruser.ec rts2count.ec imageset.ec targetset.ec rts2plan.ec rts2planset.ec rts2prop.ec \
	rts2camlist.ec target_auger.ec messagedb.ec rts2targetplanet.ec rts2targetgrb.ec rts2targetell.ec \
	user.ec userset.ec account.ec accountset.ec recvals.ec records.ec recordsavg.ec \
	augerset.ec

CLEANFILES = sqlerror.cpp rts2devicedb.cpp target.cpp sub_targets.cpp rts2appdb.cpp airmasscale.cpp rts2sqlcolumn.cpp observation.cpp \
	observationset.cpp rts2taruser.cpp rts2count.cpp imageset.cpp targetset.cpp rts2plan.cpp rts2planset.cpp rts2prop.cpp \
	rts2camlist.cpp target_auger.cpp messagedb.cpp rts2targetplanet.cpp rts2targetgrb.cpp rts2targetell.cpp \
	user.cpp userset.cpp account.cpp accountset.cpp recvals.cpp records.cpp recordsavg.cpp \
	augerset.cpp

if PGSQL

bin_PROGRAMS = rts2-airmasscale
noinst_LIBRARIES = librts2blockdb.a

nodist_rts2_airmasscale_SOURCES = airmasscale.cpp
rts2_airmasscale_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@
rts2_airmasscale_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ librts2blockdb.a ../utils/librts2block.a

nodist_librts2blockdb_a_SOURCES = sqlerror.cpp rts2devicedb.cpp target.cpp sub_targets.cpp rts2appdb.cpp rts2sqlcolumn.cpp observation.cpp \
	observationset.cpp rts2taruser.cpp rts2count.cpp imageset.cpp targetset.cpp rts2plan.cpp rts2planset.cpp \
	rts2prop.cpp rts2camlist.cpp target_auger.cpp messagedb.cpp rts2targetplanet.cpp rts2targetgrb.cpp \
	imagesetstat.cpp rts2targetell.cpp user.cpp userset.cpp account.cpp accountset.cpp recvals.cpp records.cpp recordsavg.cpp \
	augerset.cpp constraints.cpp
librts2blockdb_a_SOURCES = imagesetstat.cpp
librts2blockdb_a_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
