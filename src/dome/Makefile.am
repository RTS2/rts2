# $Id$

bin_PROGRAMS = rts2-dome-bart rts2-dome-fram rts2-dome-dcm

noinst_HEADERS = dome.h udpweather.h

LDADD = ../utils/librts2block.a @LIB_NOVA@

rts2_dome_bart_SOURCES = dome.cpp bart.cpp

rts2_dome_fram_SOURCES = dome.cpp fram.cpp 

rts2_dome_dcm_SOURCES = dome.cpp dcm.cpp udpweather.cpp
