# Process this file with automake to produce Makefile.in

noinst_PROGRAMS = test

test_SOURCES = test.c
test_LDFLAGS = config.o hms.o mkpath.o

noinst_LIBRARIES = libdevdem.a libdevser.a libdevcli.a

libdevdem_a_SOURCES = devdem.c devser.c devcli.c devhnd.c hms.c param.c thread_attr.c config.c

libdevser_a_SOURCES = devdem.c devser.c hms.c param.c thread_attr.c config.c

libdevcli_a_SOURCES = devcli.c devhnd.c param.c hms.c mv.c mkpath.c thread_attr.c config.c

noinst_HEADERS = devcli.h devdem.h devser.h config.h

#libdevdem_la_LDFLAGS = \
#	-version-info $(LT_VERSION) \
#	-release $(LT_RELEASE) 
#	-export-dynamic

