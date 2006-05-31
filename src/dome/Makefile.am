# $Id$

bin_PROGRAMS = rts2-dome-bart rts2-dome-fram rts2-dome-dcm rts2-dome-dublin rts2-cupola-mark rts2-cupola-dummy rts2-weathertimeout

noinst_HEADERS = dome.h udpweather.h cupola.h rts2connbufweather.h

LDADD = ../utils/librts2block.a @LIB_NOVA@

rts2_dome_bart_SOURCES = dome.cpp bart.cpp udpweather.cpp rts2connbufweather.cpp

rts2_dome_fram_SOURCES = dome.cpp fram.cpp udpweather.cpp

rts2_cupola_mark_SOURCES = dome.cpp cupola.cpp mark.cpp udpweather.cpp

rts2_cupola_dummy_SOURCES = dome.cpp cupola.cpp dummy_cup.cpp

rts2_dome_dublin_SOURCES = dome.cpp dublin.cpp udpweather.cpp rts2connbufweather.cpp

rts2_dome_dcm_SOURCES = dome.cpp dcm.cpp udpweather.cpp

if COMEDI
bin_PROGRAMS += rts2-dome-ir
rts2_dome_ir_SOURCES = dome.cpp ir.cpp
rts2_dome_ir_LDFLAGS = @LIB_COMEDI@
endif

rts2_weathertimeout_SOURCES = weathertimeout.cpp
