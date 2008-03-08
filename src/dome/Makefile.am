# $Id$

bin_PROGRAMS = rts2-dome-bart rts2-dome-fram rts2-dome-dcm rts2-dome-dublin \
	rts2-cupola-mark rts2-cupola-dummy rts2-weathertimeout rts2-dome-ir \
	rts2-dome-bootes1


noinst_HEADERS = dome.h udpweather.h cupola.h rts2connbufweather.h ford.h

EXTRA_DIST = udpweather.cpp rts2connbufweather.cpp dome.cpp dummy_cup.cpp ford.cpp

LDADD = dome.o ../utils/librts2block.a @LIB_NOVA@

rts2_dome_bart_SOURCES = bart.cpp
rts2_dome_bart_LDADD = udpweather.o rts2connbufweather.o ford.o ${LDADD}

rts2_dome_fram_SOURCES = fram.cpp
rts2_dome_fram_LDADD = udpweather.o ${LDADD}

rts2_cupola_mark_SOURCES = mark.cpp
rts2_cupola_mark_LDADD = cupola.o udpweather.o ${LDADD}

rts2_cupola_dummy_SOURCES = cupola.cpp
rts2_cupola_dummy_LDADD = dummy_cup.o ${LDADD}

rts2_dome_dublin_SOURCES = dublin.cpp
rts2_dome_dublin_LDADD = udpweather.o rts2connbufweather.o ${LDADD}

rts2_dome_dcm_SOURCES = dcm.cpp
rts2_dome_dcm_LDADD = udpweather.o ${LDADD}

rts2_dome_ir_SOURCES = ir.cpp
rts2_dome_ir_LDADD = ${LDADD}
rts2_dome_ir_LDFLAGS = @LIB_COMEDI@

rts2_dome_bootes1_SOURCES = bootes1.cpp
rts2_dome_bootes1_LDADD = udpweather.o rts2connbufweather.o ford.o ${LDADD}

rts2_weathertimeout_SOURCES = weathertimeout.cpp
