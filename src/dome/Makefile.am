bin_PROGRAMS = rts2-dome-bart rts2-dome-fram \
	rts2-cupola-mark rts2-cupola-dummy rts2-dome-bootes1a \
	rts2-dome-bootes1b rts2-dome-zelio rts2-dome-opentpl rts2-dome-flwo

SUBDIRS = zelio

EXTRA_DIST = 

LDADD = librts2dome.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@
AM_CFLAGS = @NOVA_CFLAGS@

noinst_HEADERS = dome.h cupola.h ford.h vermes.h door_vermes.h \
	dome-target-az.h  barcodereader_vermes.h \
	ssd650v_comm_vermes.h  bisync_vermes.h move-to-target-az_vermes.h \
	oak_comm_vermes.h move-door_vermes.h OakFeatureReports.h OakHidBase.h \
	util_vermes.h serial_vermes.h

noinst_LIBRARIES = librts2dome.a librts2dome-target-az.a

librts2dome_a_SOURCES = dome.cpp ford.cpp

librts2dome_target_az_a_SOURCES =  dome-target-az.c

rts2_dome_bart_SOURCES = bart.cpp

rts2_dome_fram_SOURCES = fram.cpp

rts2_cupola_mark_SOURCES = mark.cpp
rts2_cupola_mark_LDADD = cupola.o ${LDADD}

rts2_cupola_dummy_SOURCES = cupola.cpp dummy_cup.cpp

rts2_dome_bootes1a_SOURCES = bootes1a.cpp

rts2_dome_bootes1b_SOURCES = bootes1b.cpp

rts2_dome_zelio_SOURCES = zelio.cpp

rts2_dome_opentpl_SOURCES = opentpl.cpp

rts2_dome_flwo_SOURCES = flwo.cpp

if SUNOS
EXTRA_DIST += OakFeatureReports.c OakHidBase.c move-door_vermes.c move-to-target-az_vermes.c barcodereader_vermes.c ssd650v_comm_vermes.c bisync_vermes.c util_vermes.c serial_vermes.c oak_comm_vermes.c watcher.cpp
else
bin_PROGRAMS += rts2-cupola-vermes rts2-dome-door-vermes rts2-dome-watcher

noinst_LIBRARIES += librts2vermes.a librts2oak.a

librts2oak_a_SOURCES =  OakFeatureReports.c OakHidBase.c

librts2vermes_a_SOURCES = move-door_vermes.c move-to-target-az_vermes.c barcodereader_vermes.c ssd650v_comm_vermes.c bisync_vermes.c util_vermes.c serial_vermes.c oak_comm_vermes.c

rts2_cupola_vermes_SOURCES = cupola.cpp vermes.cpp
rts2_cupola_vermes_LDADD =  librts2vermes.a  librts2dome-target-az.a @LIB_PTHREAD@  ${LDADD} 

rts2_dome_door_vermes_SOURCES = door_vermes.cpp
rts2_dome_door_vermes_LDADD = librts2vermes.a  librts2oak.a librts2dome-target-az.a  @LIB_PTHREAD@  ${LDADD} 

rts2_dome_watcher_SOURCES = watcher.cpp
endif

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
