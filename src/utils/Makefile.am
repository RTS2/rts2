# Process this file with automake to produce Makefile.in

noinst_PROGRAMS = test

test_SOURCES = test.cpp objectcheck.cpp mkpath.c
test_LDFLAGS = config.o 

noinst_LIBRARIES = librts2block.a

librts2block_a_SOURCES = rts2block.cpp rts2device.cpp config.c rts2option.cpp \
	rts2address.cpp rts2user.cpp rts2client.cpp rts2command.cpp rts2value.cpp \
	rts2devclient.cpp rts2dataconn.cpp config.c

noinst_HEADERS = devcli.h devdem.h devser.h config.h mkpath.h devconn.h thread_attr.h hms.h \
	rts2block.h rts2device.h rts2option.h rts2address.h rts2user.h rts2client.h rts2command.h rts2value.h \
	rts2devclient.h rts2dataconn.h rts2event.h
