# $Id$

bin_PROGRAMS = rts2-mon rts2-cmon

rts2_mon_SOURCES = nmonitor.c
rts2_mon_LDFLAGS = @LIB_NCURSES@ @LIB_PTHREAD@ @LIB_M@ @LIB_NOVA@ ../utils/libdevcli.a

rts2_cmon_SOURCES = monitor.c
rts2_cmon_LDFLAGS = ../utils/libdevcli.a
