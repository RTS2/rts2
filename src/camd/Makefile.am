# $Id$

SUBDIRS = miniccd urvc

bin_PROGRAMS = rts2-camd-urvc rts2-camd-apogee rts2-camd-miniccd

if FOCUSING

rts2_camd_miniccd_SOURCES = camd.c focusing.c tcf.c sex_interface.c 
rts2_camd_miniccd_LDFLAGS = miniccd/libminiccd.a ../utils/libdevdem.a @LIB_PTHREAD@ @LIB_CFITSIO@
rts2_camd_miniccd_CFLAGS = -DFOCUSING

rts2_camd_urvc_SOURCES = camd.c focusing.c tcf.c sex_interface.c
rts2_camd_urvc_LDFLAGS = urvc/liburvc.a ../utils/libdevdem.a @LIB_PTHREAD@ @LIB_CFITSIO@
rts2_camd_urvc_CFLAGS = -DFOCUSING

rts2_camd_apogee_SOURCES = camd.c apogee/apogee.cpp apogee/CameraIO_LinuxPCI.cpp focusing.c tcf.c sex_interface.c
rts2_camd_apogee_LDFLAGS = ../utils/libdevdem.a @LIB_PTHREAD@ @LIB_CFITSIO@
rts2_camd_apogee_CFLAGS = -DFOCUSING

else

rts2_camd_miniccd_SOURCES = camd.c
rts2_camd_miniccd_LDFLAGS = miniccd/libminiccd.a ../utils/libdevdem.a @LIB_PTHREAD@

rts2_camd_urvc_SOURCES = camd.c
rts2_camd_urvc_LDFLAGS = urvc/liburvc.a ../utils/libdevdem.a @LIB_PTHREAD@

rts2_camd_apogee_SOURCES = camd.c apogee/apogee.cpp apogee/CameraIO_LinuxPCI.cpp
rts2_camd_apogee_LDFLAGS = ../utils/libdevdem.a @LIB_PTHREAD@

endif

noinst_HEADERS = camera.h

#CC = $(CXX)

#rts2_camd_sbigusb_SOURCES = camd.c
#rst2_camd_sbigusb_CFLAGS = -I/home/petr/sbig/include
#rts2_camd_sbigusb_LDFLAGS = ../utils/libdevdem.a sbigusb/libsbigusb.a /home/petr/sbig/lib/libsbig.a 
