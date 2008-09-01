noinst_HEADERS = rts2schedbag.h rts2schedule.h rts2schedobs.h

if PGSQL

bin_PROGRAMS = rts2-scheduler

rts2_scheduler_SOURCES = scheduler.cpp rts2schedbag.cpp rts2schedule.cpp rts2schedobs.cpp
rts2_scheduler_CXXFLAGS = @LIBPG_CFLAGS@
rts2_scheduler_LDFLAGS = ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@

%.cpp: %.ec
	@ECPG@ -o $@ $^

EXTRA_DIST = rts2schedobs.ec

else

EXTRA_DIST = scheduler.cpp rts2schedbag.cpp rts2schedule.cpp rts2schedobs.ec

endif

