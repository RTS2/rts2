bin_PROGRAMS = rts2-dome-bart rts2-dome-fram rts2-dome-watcher \
	rts2-cupola-mark rts2-cupola-dummy rts2-dome-bootes1a \
	rts2-dome-bootes1b rts2-dome-zelio rts2-dome-opentpl \
	rts2-cupola-vermes

SUBDIRS = zelio

EXTRA_DIST = 

noinst_HEADERS = dome.h cupola.h ford.h vermes.h

noinst_LIBRARIES = librts2dome.a librts2dome-target-az.a librts2vermes.a

librts2dome_a_SOURCES = dome.cpp ford.cpp

librts2dome_target_az_a_SOURCES =  dome-target-az.c

librts2vermes_a_SOURCES =  barcodereader_vermes.c  ssd650v_comm_vermes.c bisync_vermes.c util_vermes.c serial_vermes.c

LDADD = librts2dome.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

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

rts2_cupola_vermes_SOURCES = cupola.cpp vermes.cpp
rts2_cupola_vermes_LDADD = librts2dome-target-az.a librts2vermes.a  @LIB_PTHREAD@  ${LDADD} 

if COMEDI
bin_PROGRAMS += rts2-dome-bootes2 rts2-dome-ieec

rts2_dome_bootes2_SOURCES = bootes2.cpp
rts2_dome_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_bootes2_CPPFLAGS = @INC_COMEDI@

rts2_dome_ieec_SOURCES = ieec.cpp
rts2_dome_ieec_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_ieec_CPPFLAGS = @INC_COMEDI@

else
EXTRA_DIST += bootes2.cpp ieec.cpp
endif
