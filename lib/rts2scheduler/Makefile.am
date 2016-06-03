if PGSQL

lib_LTLIBRARIES = librts2scheduler.la

librts2scheduler_la_SOURCES = schedbag.cpp schedule.cpp schedobs.cpp ticket.cpp ticketset.cpp utils.cpp
librts2scheduler_la_CXXFLAGS = @LIBPG_CFLAGS@ @CFITSIO_CFLAGS@ @MAGIC_CFLAGS@ -I../../include

.ec.cpp:
	@ECPG@ -o $@ $^

EXTRA_DIST = schedobs.ec ticket.ec ticketset.ec

else

EXTRA_DIST = schedule.cpp schedbag.cpp schedule.cpp schedobs.ec ticket.ec ticketset.ec utils.cpp

endif

CLEANFILES = schedobs.cpp ticket.cpp ticketset.cpp
