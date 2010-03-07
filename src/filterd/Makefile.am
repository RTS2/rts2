bin_PROGRAMS = rts2-filterd-ifw rts2-filterd-dummy

noinst_HEADERS = filterd.h

noinst_LIBRARIES  = librts2filterd.a

EXTRA_DIST = 

librts2filterd_a_SOURCES = filterd.cpp

LDADD = librts2filterd.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

rts2_filterd_ifw_SOURCES = ifw.cpp

rts2_filterd_dummy_SOURCES = dummy.cpp

if FLI
bin_PROGRAMS += rts2-filterd-fli
rts2_filterd_fli_SOURCES = fli.cpp
rts2_filterd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_filterd_fli_CXXFLAGS = @INC_FLI@
endif

if MDMTCS
bin_PROGRAMS += rts2-filterd-mdm
rts2_filterd_mdm_SOURCES = mdm.cpp
rts2_filterd_mdm_LDFLAGS = @LIB_MDMTCS@
rts2_filterd_mdm_CPPFLAGS = @INC_MDMTCS@
else
EXTRA_DIST += mdm.cpp
endif
