# $Id$

SUBDIRS = urvc sbigusb

bin_PROGRAMS = rts2-camd-urvc

rts2_camd_urvc_SOURCES = camd.c
rts2_camd_urvc_LDFLAGS = urvc/liburvc.a ../utils/libdevdem.a @LIB_PTHREAD@

noinst_HEADERS = camera.h

#CC = $(CXX)

#rts2_camd_sbigusb_SOURCES = camd.c
#rst2_camd_sbigusb_CFLAGS = -I/home/petr/sbig/include
#rts2_camd_sbigusb_LDFLAGS = ../utils/libdevdem.a sbigusb/libsbigusb.a /home/petr/sbig/lib/libsbig.a 
