bin_PROGRAMS = rts2-logger rts2-logd

noinst_HEADERS = rts2loggerbase.h

LDADD = rts2loggerbase.o

rts2_logger_SOURCES = logger.cpp
rts2_logger_LDFLAGS = @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a
rts2_logger_CXXFLAGS = @NOVA_CFLAGS@

rts2_logd_SOURCES = logd.cpp
rts2_logd_LDFLAGS = @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a
rts2_logd_CXXFLAGS = @NOVA_CFLAGS@
