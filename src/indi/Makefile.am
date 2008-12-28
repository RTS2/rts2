bin_PROGRAMS = rts2-indi

noinst_HEADERS = indidrivermain.h eventloop.h

rts2_indi_SOURCES = indi.cpp indidrivermain.c eventloop.c
rts2_indi_LDFLAGS = -L../libindi -lindi -lz ../utils/librts2block.a 
rts2_indi_CXXFLAGS = -I../libindi
rts2_indi_CFLAGS = -I../libindi
