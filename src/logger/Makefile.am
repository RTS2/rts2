bin_PROGRAMS = rts2-logger rts2-logd

noinst_HEADERS = loggerbase.h

EXTRA_DIST = loggerbase.cpp

AM_CXXFLAGS=@NOVA_CFLAGS@ -I../../include

LDADD = loggerbase.o -L../../lib/rts2 -lrts2 @LIB_NOVA@ @LIB_M@

rts2_logger_SOURCES = logger.cpp

rts2_logd_SOURCES = logd.cpp
