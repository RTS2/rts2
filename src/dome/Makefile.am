bin_PROGRAMS = rts2-dome-bart rts2-dome-fram rts2-dome-watcher \
	rts2-cupola-mark rts2-cupola-dummy rts2-dome-bootes1a \
	rts2-dome-bootes1b rts2-dome-zelio rts2-dome-opentpl

EXTRA_DIST = 

noinst_HEADERS = dome.h cupola.h ford.h

noinst_LIBRARIES = librts2dome.a

librts2dome_a_SOURCES = dome.cpp ford.cpp

LDADD = librts2dome.a ../utils/librts2block.a @LIB_NOVA@

rts2_dome_bart_SOURCES = bart.cpp

rts2_dome_fram_SOURCES = fram.cpp

rts2_cupola_mark_SOURCES = mark.cpp
rts2_cupola_mark_LDADD = cupola.o ${LDADD}

rts2_cupola_dummy_SOURCES = cupola.cpp dummy_cup.cpp

rts2_dome_watcher_SOURCES = watcher.cpp

rts2_dome_bootes1a_SOURCES = bootes1a.cpp

rts2_dome_bootes1b_SOURCES = bootes1b.cpp

rts2_dome_zelio_SOURCES = zelio.cpp

rts2_dome_opentpl_SOURCES = opentpl.cpp

if COMEDI
bin_PROGRAMS += rts2-dome-bootes2 rts2-dome-ieec

rts2_dome_bootes2_SOURCES = bootes2.cpp
rts2_dome_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_bootes2_CPPFLAGS = @INC_COMEDI@

rts2_dome_bootes2_SOURCES = ieec.cpp
rts2_dome_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_bootes2_CPPFLAGS = @INC_COMEDI@

else
EXTRA_DIST += bootes2.cpp ieec.cpp
endif
