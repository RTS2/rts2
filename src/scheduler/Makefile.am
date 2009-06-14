noinst_HEADERS = rts2schedbag.h rts2schedule.h rts2schedobs.h ticket.h utils.h

if PGSQL

noinst_LIBRARIES = librts2scheduler.a

librts2scheduler_a_SOURCES = rts2schedbag.cpp rts2schedule.cpp rts2schedobs.cpp ticket.cpp ticketset.cpp utils.cpp
librts2scheduler_a_CXXFLAGS = @LIBPG_CFLAGS@

bin_PROGRAMS = rts2-scheduler

rts2_scheduler_SOURCES = scheduler.cpp
rts2_scheduler_CXXFLAGS = @LIBPG_CFLAGS@ @LIB_JPEG@
rts2_scheduler_LDFLAGS = librts2scheduler.a ../utilsdb/librts2blockdb.a ../writers/librts2imagedb.a ../utils/librts2block.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @JPEG_CFLAGS@

%.cpp: %.ec
	@ECPG@ -o $@ $^

EXTRA_DIST = rts2schedobs.ec ticket.ec ticketset.ec

else

EXTRA_DIST = scheduler.cpp rts2schedbag.cpp rts2schedule.cpp rts2schedobs.ec ticket.ec utils.cpp

endif

CLEANFILES = rts2schedobs.cpp ticket.cpp ticketset.cpp
