bin_PROGRAMS = rts2-filterd-ifw rts2-filterd-dummy rts2-filterd-fw102c rts2-filterd-apm rts2-filterd-galil rts2-filterd-fliindi

EXTRA_DIST = 

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

rts2_filterd_ifw_SOURCES = ifw.cpp

rts2_filterd_dummy_SOURCES = dummy.cpp

rts2_filterd_fw102c_SOURCES = fw102c.cpp

rts2_filterd_apm_SOURCES = apm.cpp

rts2_filterd_galil_SOURCES = galil.cpp

rts2_filterd_fliindi_SOURCES = fliindi.cpp
rts2_filterd_fliindi_LDADD = -L../../lib/libindi -lindi -lz @LIB_PTHREAD@ ${LDADD}
rts2_filterd_fliindi_CPPFLAGS = -I../../lib/libindi ${CPPFLAGS} -I../../include

if FLI
bin_PROGRAMS += rts2-filterd-fli
rts2_filterd_fli_SOURCES = fli.cpp
rts2_filterd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_filterd_fli_CXXFLAGS = @INC_FLI@ -I../../include
else
EXTRA_DIST += fli.cpp
endif

if MICCD
bin_PROGRAMS += rts2-filterd-mi
rts2_filterd_mi_SOURCES = mi.cpp
rts2_filterd_mi_LDFLAGS = @LIB_MICCD@
rts2_filterd_mi_CXXFLAGS = @INC_MICCD@ -I../../include
else
EXTRA_DIST += mi.cpp
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
