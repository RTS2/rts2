bin_PROGRAMS = rts2-filterd-ifw rts2-filterd-dummy rts2-filterd-fw102c

EXTRA_DIST = 

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

rts2_filterd_ifw_SOURCES = ifw.cpp

rts2_filterd_dummy_SOURCES = dummy.cpp

rts2_filterd_fw102c_SOURCES = fw102c.cpp

if FLI
bin_PROGRAMS += rts2-filterd-fli
rts2_filterd_fli_SOURCES = fli.cpp
rts2_filterd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_filterd_fli_CXXFLAGS = @INC_FLI@ -I../../include
else
EXTRA_DIST += fli.cpp
endif

if MICCD
bin_PROGRAMS += rts2-filterd-miccd
rts2_filterd_miccd_SOURCES = miccd.cpp
rts2_filterd_miccd_LDFLAGS = @LIB_MICCD@
rts2_filterd_miccd_CXXFLAGS = @INC_MICCD@ -I../../include
else
EXTRA_DIST += miccd.cpp
endif

if ALTA
bin_PROGRAMS += rts2-filterd-alta
rts2_filterd_alta_SOURCES = alta.cpp
rts2_filterd_alta_LDADD = @LIB_ALTA_USB@ @LIB_M@ ${LDADD}
rts2_filterd_alta_CXXFLAGS = @INC_ALTA@ -I../../include
else
EXTRA_DIST += alta.cpp
endif

if MDMTCS
bin_PROGRAMS += rts2-filterd-mdm
rts2_filterd_mdm_SOURCES = mdm.cpp
rts2_filterd_mdm_LDFLAGS = @LIB_MDMTCS@
rts2_filterd_mdm_CPPFLAGS = @INC_MDMTCS@ -I../../include
else
EXTRA_DIST += mdm.cpp
endif
