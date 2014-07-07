bin_PROGRAMS = rts2-dome-bart rts2-dome-esa rts2-dome-fram \
	rts2-cupola-mark rts2-cupola-dummy rts2-dome-bootes1a \
	rts2-dome-bootes1b rts2-dome-zelio rts2-dome-opentpl \
	rts2-cupola-maxdomeii

SUBDIRS = zelio

EXTRA_DIST = 

LDADD = -lrts2 -L../../lib/rts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include
AM_CFLAGS = @NOVA_CFLAGS@ -I../../include

noinst_HEADERS =  ahe.h

rts2_dome_bart_SOURCES = bart.cpp

rts2_dome_esa_SOURCES = esa.cpp

rts2_dome_fram_SOURCES = fram.cpp

rts2_cupola_mark_SOURCES = mark.cpp

rts2_cupola_dummy_SOURCES = dummy_cup.cpp

rts2_dome_bootes1a_SOURCES = bootes1a.cpp

rts2_dome_bootes1b_SOURCES = bootes1b.cpp

rts2_dome_zelio_SOURCES = zelio.cpp

rts2_dome_opentpl_SOURCES = opentpl.cpp

rts2_dome_ahe_SOURCES = ahe.cpp 

rts2_cupola_maxdomeii_SOURCES = maxdomeii.cpp

if SUNCYG
EXTRA_DIST +=  watcher.cpp serial_ahe.c
else
bin_PROGRAMS += rts2-cupola-vermes rts2-dome-door-vermes rts2-dome-watcher rts2-dome-ahe

noinst_LTLIBRARIES = 

rts2_cupola_vermes_SOURCES = vermes.cpp
rts2_cupola_vermes_LDADD = -L../../lib/rts2/vermes/.libs -lrts2vermes  @LIB_PTHREAD@  ${LDADD} 

rts2_dome_door_vermes_SOURCES =  door_vermes.cpp
rts2_dome_door_vermes_LDADD =  -L../../lib/rts2/vermes/.libs -lrts2vermes   @LIB_PTHREAD@  ${LDADD} 

rts2_dome_watcher_SOURCES = watcher.cpp


endif

if COMEDI
bin_PROGRAMS += rts2-dome-bootes2 rts2-dome-ieec

rts2_dome_bootes2_SOURCES = bootes2.cpp
rts2_dome_bootes2_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_bootes2_CPPFLAGS = @INC_COMEDI@ -I../../include

rts2_dome_ieec_SOURCES = ieec.cpp
rts2_dome_ieec_LDFLAGS = @LIB_COMEDI@ @LIB_M@
rts2_dome_ieec_CPPFLAGS = @INC_COMEDI@ -I../../include

else
EXTRA_DIST += bootes2.cpp ieec.cpp
endif
