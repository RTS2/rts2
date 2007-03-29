# Process this file with automake to produce Makefile.in

noinst_HEADERS = rts2devicedb.h target.h rts2appdb.h rts2sqlcolumn.h rts2obs.h rts2obsset.h \
	rts2taruser.h rts2count.h rts2imgset.h rts2targetset.h rts2plan.h rts2planset.h rts2prop.h \
	rts2camlist.h target_auger.h rts2messagedb.h rts2targetplanet.h rts2targetgrb.h scriptcommands.h \
	rts2imgsetstat.h

EXTRA_DIST = rts2devicedb.ec target.ec sub_targets.ec rts2appdb.ec airmasscale.ec rts2sqlcolumn.ec rts2obs.ec \
	rts2obsset.ec rts2taruser.ec rts2count.ec rts2imgset.ec rts2targetset.ec rts2plan.ec rts2planset.ec rts2prop.ec \
	rts2camlist.ec target_auger.ec rts2messagedb.ec rts2targetplanet.ec rts2targetgrb.ec

if PGSQL

bin_PROGRAMS = rts2-airmasscale
noinst_LIBRARIES = librts2blockdb.a

rts2_airmasscale_SOURCES = airmasscale.cpp
rts2_airmasscale_CXXFLAGS = @LIBPG_CFLAGS@
rts2_airmasscale_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ librts2blockdb.a ../utils/librts2block.a

librts2blockdb_a_SOURCES = rts2devicedb.cpp target.cpp sub_targets.cpp rts2appdb.cpp rts2sqlcolumn.cpp rts2obs.cpp \
	rts2obsset.cpp rts2taruser.cpp rts2count.cpp rts2imgset.cpp rts2targetset.cpp rts2plan.cpp rts2planset.cpp \
	rts2prop.cpp rts2camlist.cpp target_auger.cpp rts2messagedb.cpp rts2targetplanet.cpp rts2targetgrb.cpp \
	rts2imgsetstat.cpp
librts2blockdb_a_CXXFLAGS = @LIBPG_CFLAGS@

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
