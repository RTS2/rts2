# $Id$

SUBDIRS = urvc2 apogee edtsao

bin_PROGRAMS = rts2-camd-miniccd rts2-camd-miniccd-il rts2-camd-urvc2 rts2-camd-apogee rts2-camd-dummy

noinst_HEADERS = camd.h ccd_msg.h cliwheel.h filter.h clifocuser.h

noinst_LIBRARIES = librts2camd.a

librts2camd_a_SOURCES = camd.cpp cliwheel.cpp clifocuser.cpp

LDADD = librts2camd.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

rts2_camd_miniccd_SOURCES = miniccd.cpp

rts2_camd_miniccd_il_SOURCES = miniccd_il.cpp

rts2_camd_urvc2_SOURCES = urvc2.cpp
rts2_camd_urvc2_LDFLAGS = urvc2/liburvc2.a

rts2_camd_apogee_SOURCES = apogee.cpp apogee/CameraIO_LinuxPCI.cpp

rts2_camd_dummy_SOURCES = dummy.cpp

EXTRA_DIST = ""

if SBIG
bin_PROGRAMS += rts2-camd-sbigusb
rts2_camd_sbigusb_SOURCES = sbigusb.cpp
rts2_camd_sbigusb_LDFLAGS = @LIB_SBIG@
rts2_camd_sbigusb_CXXFLAGS = @INC_SBIG@
else
EXTRA_DIST += sbigusb.cpp
endif

if ANDOR 
bin_PROGRAMS += rts2-camd-andor
rts2_camd_andor_SOURCES = andor.cpp
rts2_camd_andor_LDFLAGS = @LIB_ANDOR@ @LIB_M@ @LIB_PTHREAD@
else
EXTRA_DIST += andor.cpp
endif

if ALTA
bin_PROGRAMS += rts2-camd-alta
rts2_camd_alta_SOURCES = alta.cpp
rts2_camd_alta_LDADD = @LIB_ALTA@ @LIB_M@ ${LDADD}
rts2_camd_alta_CXXFLAGS = @INC_ALTA@
else
EXTRA_DIST += alta.cpp
endif

if FLI
bin_PROGRAMS += rts2-camd-fli
rts2_camd_fli_SOURCES = fli.cpp
rts2_camd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_camd_fli_CXXFLAGS = @INC_FLI@
else
EXTRA_DIST += fli.cpp
endif

if MICCD
bin_PROGRAMS += rts2-camd-miccd
rts2_camd_miccd_SOURCES = miccd.cpp
rts2_camd_miccd_LDFLAGS = @LIB_MICCD@
rts2_camd_miccd_CXXFLAGS = @INC_MICCD@
else
EXTRA_DIST += miccd.cpp
endif

if EDTPDV
bin_PROGRAMS += rts2-camd-edtsao
rts2_camd_edtsao_SOURCES = edtsao.cpp
rts2_camd_edtsao_LDFLAGS = edtsao/libedtsao.a @LIB_PTHREAD@ @LIB_EDTPDV@ 
rts2_camd_edtsao_CXXFLAGS = @INC_EDTPDV@
else
EXTRA_DIST += edtsao.cpp
endif

if ARC
bin_PROGRAMS += rts2-camd-arc
rts2_camd_arc_SOURCES = arc.cpp
rts2_camd_arc_LDFLAGS = @LIB_ARC@
rts2_camd_arc_CXXFLAGS = @INC_ARC@
else
EXTRA_DIST += arc.pp
endif
