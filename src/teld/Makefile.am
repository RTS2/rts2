# $Id$

SUBDIRS = lx200 losmandy #para

bin_PROGRAMS = rts2-teld-lx200  rts2-teld-losmandy #rts2-teld-para

noinst_HEADERS = telescope.h

rts2_teld_lx200_SOURCES = teld.c
rts2_teld_lx200_LDFLAGS = @LIB_M@ @LIB_PTHREAD@ @LIB_NOVA@ lx200/liblx200.a ../utils/libdevdem.a

rts2_teld_losmandy_SOURCES = teld.c
rts2_teld_losmandy_LDFLAGS = @LIB_M@ @LIB_PTHREAD@ @LIB_NOVA@ losmandy/liblosmandy.a ../utils/libdevdem.a

#rts2_teld_para_SOURCES = teld.c
#rts2_teld_para_LDFLAGS = @LIB_NOVA@ @LIB_M@ @LIB_PTHREAD@ para/libmks3.o para/te.o ../utils/libdevdem.a
