bin_PROGRAMS = rts2-logger rts2-logd

noinst_HEADERS = rts2loggerbase.h

EXTRA_DIST = rts2loggerbase.cpp

AM_CXXFLAGS=@NOVA_CFLAGS@

LDADD = rts2loggerbase.o ../utils/librts2block.a @LIB_NOVA@ @LIB_M@

rts2_logger_SOURCES = logger.cpp

rts2_logd_SOURCES = logd.cpp
