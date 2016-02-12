# $Id$

SUBDIRS = urvc2 apogee edtsao si8821 sxccd

bin_PROGRAMS = rts2-camd-sx rts2-camd-dummy rts2-camd-sidecar rts2-camd-azcam rts2-camd-si8821

noinst_HEADERS = reflex.h

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

rts2_camd_sx_SOURCES = sx.cpp
rts2_camd_sx_LDADD = sxccd/libsxccdusb.a ${LDADD} -lusb-1.0

rts2_camd_dummy_SOURCES = dummy.cpp
rts2_camd_dummy_CXXFLAGS = ${AM_CXXFLAGS} @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ -I../../include
rts2_camd_dummy_LDADD = -L../../lib/rts2fits -lrts2image ${LDADD} @LIB_CFITSIO@ @LIB_JPEG@

rts2_camd_sidecar_SOURCES = sidecar.cpp

rts2_camd_azcam_SOURCES = azcam.cpp

rts2_camd_si8821_SOURCES = si8821.cpp

EXTRA_DIST = 

if SUNCYGMAC
EXTRA_DIST += urvc2.cpp apogee.cpp v4l.cpp
else
if ARMCPU
EXTRA_DIST += urvc2.cpp apogee.cpp

bin_PROGRAMS += rts2-camd-v4l

rts2_camd_v4l_SOURCES = v4l.cpp
else
bin_PROGRAMS += rts2-camd-urvc2 rts2-camd-apogee rts2-camd-v4l

rts2_camd_urvc2_SOURCES = urvc2.cpp
rts2_camd_urvc2_LDFLAGS = urvc2/liburvc2.a

rts2_camd_apogee_SOURCES = apogee.cpp
rts2_camd_apogee_LDFLAGS = apogee/libapogee.a

rts2_camd_v4l_SOURCES = v4l.cpp
endif
endif

if SBIG
bin_PROGRAMS += rts2-camd-sbig
rts2_camd_sbig_SOURCES = sbig.cpp
rts2_camd_sbig_LDFLAGS = @LIB_SBIG@ ${LDADD} @LIB_CFITSIO@
rts2_camd_sbig_CXXFLAGS = @INC_SBIG@ -I../../include @CFITSIO_CFLAGS@
else
EXTRA_DIST += sbig.cpp
endif

if ANDOR 
bin_PROGRAMS += rts2-camd-andor
rts2_camd_andor_SOURCES = andor.cpp
rts2_camd_andor_LDFLAGS = @LIB_ANDOR@ @LIB_M@ @LIB_PTHREAD@
else
EXTRA_DIST += andor.cpp
endif

if ALTA
bin_PROGRAMS += rts2-camd-alta rts2-camd-altanet
rts2_camd_alta_SOURCES = alta.cpp
rts2_camd_alta_LDADD = @LIB_ALTA_USB@ @LIB_M@ ${LDADD}
rts2_camd_alta_CXXFLAGS = @INC_ALTA@ -I../../include
rts2_camd_altanet_SOURCES = alta.cpp
rts2_camd_altanet_LDADD = @LIB_ALTA_NET@ @LIB_M@ ${LDADD} -lcurl
rts2_camd_altanet_CXXFLAGS = @INC_ALTA@ -I../../include
else
EXTRA_DIST += alta.cpp
endif

if FLI
bin_PROGRAMS += rts2-camd-fli
rts2_camd_fli_SOURCES = fli.cpp
rts2_camd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_camd_fli_CXXFLAGS = @INC_FLI@ -I../../include
else
EXTRA_DIST += fli.cpp
endif

if MICCD
bin_PROGRAMS += rts2-camd-miccd
rts2_camd_miccd_SOURCES = miccd.cpp
rts2_camd_miccd_LDFLAGS = @LIB_MICCD@ @LIB_K8055@
rts2_camd_miccd_CXXFLAGS = @INC_MICCD@ -I../../include
else
EXTRA_DIST += miccd.cpp
endif

if EDTPDV
bin_PROGRAMS += rts2-camd-edtsao rts2-camd-reflex

rts2_camd_edtsao_SOURCES = edtsao.cpp
rts2_camd_edtsao_LDFLAGS = edtsao/libedtsao.a @LIB_EDTPDV@ @LIB_PTHREAD@
rts2_camd_edtsao_CXXFLAGS = @INC_EDTPDV@ -I../../include

rts2_camd_reflex_SOURCES = reflex.cpp
rts2_camd_reflex_LDFLAGS = @LIB_EDTPDV@ @LIB_PTHREAD@
rts2_camd_reflex_CXXFLAGS = @INC_EDTPDV@ -I../../include

else
EXTRA_DIST += edtsao.cpp reflex.cpp
endif

if ARC
bin_PROGRAMS += rts2-camd-arc
rts2_camd_arc_SOURCES = arc.cpp
rts2_camd_arc_LDFLAGS = @LIB_ARC@
rts2_camd_arc_CXXFLAGS = @INC_ARC@ -I../../include
else
EXTRA_DIST += arc.cpp
endif

if SI3097
bin_PROGRAMS += rts2-camd-si3097
rts2_camd_si3097_SOURCES = si3097.cpp
rts2_camd_si3097_CXXFLAGS = @INC_SI3097@ -I../../include
else
EXTRA_DIST += si3097.cpp
endif
