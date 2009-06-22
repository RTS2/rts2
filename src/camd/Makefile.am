# $Id$

SUBDIRS = urvc2 apogee edtsao

bin_PROGRAMS = rts2-camd-miniccd rts2-camd-miniccd-il rts2-camd-urvc2 rts2-camd-apogee rts2-camd-dummy

noinst_HEADERS = camd.h ccd_msg.h rts2devcliwheel.h filter.h rts2devclifocuser.h

noinst_LIBRARIES = librts2camd.a

librts2camd_a_SOURCES = camd.cpp rts2devcliwheel.cpp rts2devclifocuser.cpp

LDADD = librts2camd.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

if SBIG
bin_PROGRAMS += rts2-camd-sbigusb
rts2_camd_sbigusb_SOURCES = sbigusb.cpp
rts2_camd_sbigusb_LDFLAGS = @LIB_SBIG@
rts2_camd_sbigusb_CXXFLAGS = @INC_SBIG@
endif

if ANDOR 
bin_PROGRAMS += rts2-camd-andor
rts2_camd_andor_SOURCES = andor.cpp
rts2_camd_andor_LDFLAGS = @LIB_ANDOR@ @LIB_M@ @LIB_PTHREAD@
endif

rts2_camd_miniccd_SOURCES = miniccd.cpp

rts2_camd_miniccd_il_SOURCES = miniccd_il.cpp

rts2_camd_urvc2_SOURCES = urvc2.cpp
rts2_camd_urvc2_LDFLAGS = urvc2/liburvc2.a

rts2_camd_apogee_SOURCES = apogee.cpp apogee/CameraIO_LinuxPCI.cpp

rts2_camd_dummy_SOURCES = dummy.cpp

if ALTA
bin_PROGRAMS += rts2-camd-alta
rts2_camd_alta_SOURCES = alta.cpp
rts2_camd_alta_LDADD = @LIB_ALTA@ @LIB_M@ ${LDADD}
rts2_camd_alta_CXXFLAGS = @INC_ALTA@
endif

if FLI
bin_PROGRAMS += rts2-camd-fli
rts2_camd_fli_SOURCES = fli.cpp
rts2_camd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_camd_fli_CXXFLAGS = @INC_FLI@
endif

if EDTPDV
bin_PROGRAMS += rts2-camd-edtsao

rts2_camd_edtsao_SOURCES = edtsao.cpp
rts2_camd_edtsao_LDFLAGS = edtsao/libedtsao.a @LIB_PTHREAD@ @LIB_EDTPDV@ 
rts2_camd_edtsao_CXXFLAGS = @INC_EDTPDV@
endif
