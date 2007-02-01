# $Id$

bin_PROGRAMS = rts2-mon rts2-cmon

noinst_HEADERS = nmonitor.h rts2daemonwindow.h rts2nmenu.h rts2nmsgbox.h rts2nmsgwindow.h rts2nstatuswindow.h

rts2_mon_SOURCES = nmonitor.cpp rts2daemonwindow.cpp rts2nmenu.cpp rts2nmsgbox.cpp rts2nmsgwindow.cpp rts2nstatuswindow.cpp
rts2_mon_LDFLAGS = @LIB_NCURSES@ @LIB_M@ @LIB_NOVA@ @LIB_CFITSIO@ ../writers/librts2image.a ../utils/librts2block.a 

rts2_cmon_SOURCES = cmonitor.cpp
rts2_cmon_LDFLAGS = @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a
