noinst_HEADERS = rts2schedbag.h rts2schedule.h rts2schedobs.h ticket.h ticketset.h utils.h

if PGSQL

lib_LTLIBRARIES = librts2scheduler.la

librts2scheduler_la_SOURCES = rts2schedbag.cpp rts2schedule.cpp rts2schedobs.cpp ticket.cpp ticketset.cpp utils.cpp
librts2scheduler_la_CXXFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

bin_PROGRAMS = rts2-scheduler

rts2_scheduler_SOURCES = scheduler.cpp
rts2_scheduler_CXXFLAGS = @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @JPEG_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@
rts2_scheduler_LDADD = -lrts2scheduler -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CRYPT@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

%.cpp: %.ec
	@ECPG@ -o $@ $^

EXTRA_DIST = rts2schedobs.ec ticket.ec ticketset.ec

else

EXTRA_DIST = scheduler.cpp rts2schedbag.cpp rts2schedule.cpp rts2schedobs.ec ticket.ec utils.cpp

endif

CLEANFILES = rts2schedobs.cpp ticket.cpp ticketset.cpp
