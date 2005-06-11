# $Id$

bin_PROGRAMS = rts2-dome-bart rts2-dome-fram

noinst_HEADERS = dome.h

LDADD = ../utils/librts2block.a

rts2_dome_bart_SOURCES = dome.cpp bart.cpp

rts2_dome_fram_SOURCES = dome.cpp fram.cpp udpweather.cpp

if PCI8255
bin_PROGRAMS += rts2-dome-dublin

rts2_dome_dublin_SOURCES = dome.cpp dublin.cpp udpweather.cpp
rts2_dome_dublin_CXXFLAGS = @INC_PCI8255@
endif
