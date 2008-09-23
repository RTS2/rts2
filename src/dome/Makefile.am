bin_PROGRAMS = rts2-dome-bart rts2-dome-fram rts2-dome-watcher \
	rts2-cupola-mark rts2-cupola-dummy rts2-weathertimeout \
	rts2-dome-bootes1a rts2-dome-bootes1b


noinst_HEADERS = dome.h cupola.h ford.h

EXTRA_DIST = dome.cpp ford.cpp

LDADD = dome.o ../utils/librts2block.a @LIB_NOVA@

rts2_dome_bart_SOURCES = bart.cpp
rts2_dome_bart_LDADD = ford.o ${LDADD}

rts2_dome_fram_SOURCES = fram.cpp
rts2_dome_fram_LDADD = ford.o ${LDADD}

rts2_cupola_mark_SOURCES = mark.cpp
rts2_cupola_mark_LDADD = cupola.o ${LDADD}

rts2_cupola_dummy_SOURCES = cupola.cpp dummy_cup.cpp

rts2_dome_watcher_SOURCES = watcher.cpp

rts2_weathertimeout_SOURCES = weathertimeout.cpp

rts2_dome_bootes1a_SOURCES = bootes1a.cpp
rts2_dome_bootes1a_LDADD = ford.o ${LDADD}

rts2_dome_bootes1b_SOURCES = bootes1b.cpp
rts2_dome_bootes1b_LDADD = ford.o ${LDADD}

if OPENTPL
bin_PROGRAMS += rts2-dome-ir

rts2_dome_ir_SOURCES = ir.cpp
rts2_dome_ir_LDFLAGS = @LIB_OPENTPL@
rts2_dome_ir_CPPFLAGS = -D HAVE_SYS_IPC_H -fpermissive @INC_OPENTPL@

else
EXTRA_DIST += ir.cpp
endif


if COMEDI
bin_PROGRAMS += rts2-dome-bootes2

rts2_dome_bootes2_SOURCES = bootes2.cpp
rts2_dome_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_bootes2_CPPFLAGS = @INC_COMEDI@

else
EXTRA_DIST += bootes2.cpp
endif
