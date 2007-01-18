# $Id$

SUBDIRS = urvc2 apogee

bin_PROGRAMS = rts2-camd-miniccd rts2-camd-urvc2 rts2-camd-apogee rts2-camd-dummy

noinst_HEADERS = camd.h ccd_msg.h rts2devcliwheel.h filter.h rts2devclifocuser.h

LDADD = ../utils/librts2block.a

if SBIG
bin_PROGRAMS += rts2-camd-sbigusb
rts2_camd_sbigusb_SOURCES = camd.cpp sbigusb.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp
rts2_camd_sbigusb_LDFLAGS = @LIB_SBIG@
rts2_camd_sbigusb_CXXFLAGS = @INC_SBIG@
endif

if ANDOR 
bin_PROGRAMS += rts2-camd-andor
rts2_camd_andor_SOURCES = camd.cpp andor.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp
rts2_camd_andor_LDFLAGS = @LIB_ANDOR@ @LIB_M@ @LIB_PTHREAD@ @LIB_FLI@
rts2_camd_andor_CXXFLAGS = @INC_ANDOR@ @INC_FLI@
endif

rts2_camd_miniccd_SOURCES = camd.cpp miniccd.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp

rts2_camd_urvc2_SOURCES = camd.cpp urvc2.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp
rts2_camd_urvc2_LDFLAGS = urvc2/liburvc2.a

rts2_camd_apogee_SOURCES = camd.cpp apogee.cpp apogee/CameraIO_LinuxPCI.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp

rts2_camd_dummy_SOURCES = camd.cpp dummy.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp

if ALTA
bin_PROGRAMS += rts2-camd-alta
rts2_camd_alta_SOURCES = camd.cpp alta.cpp alta/ApnCamera_Linux.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp
rts2_camd_alta_LDFLAGS = @LIB_ALTA@ @LIB_M@
rts2_camd_alta_CXXFLAGS = @INC_ALTA@
endif

if FLI
bin_PROGRAMS += rts2-camd-fli
rts2_camd_fli_SOURCES = camd.cpp fli.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp
rts2_camd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_camd_fli_CXXFLAGS = @INC_FLI@
endif
