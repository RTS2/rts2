SUBDIRS = 

bin_PROGRAMS = rts2-focusd-optec rts2-focusd-robofocus rts2-focusd-dummy rts2-focusd-opentpl rts2-focusd-atc2

noinst_HEADERS = focusd.h

noinst_LIBRARIES = librts2focusd.a

librts2focusd_a_SOURCES = focusd.cpp

LDADD = librts2focusd.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

rts2_focusd_optec_SOURCES = optec.cpp

rts2_focusd_robofocus_SOURCES = robofocus.cpp

rts2_focusd_dummy_SOURCES = dummy.cpp

rts2_focusd_opentpl_SOURCES = opentpl.cpp

rts2_focusd_atc2_SOURCES = atc2.cpp

EXTRA_DIST = 

if FLI
bin_PROGRAMS += rts2-focusd-fli
rts2_focusd_fli_SOURCES = fli.cpp
rts2_focusd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_focusd_fli_CPPFLAGS = @INC_FLI@
else
EXTRA_DIST += fli.cpp
endif

if MDMTCS
bin_PROGRAMS += rts2-focusd-mdm
rts2_focusd_mdm_SOURCES = mdm.cpp
rts2_focusd_mdm_LDFLAGS = @LIB_MDMTCS@
rts2_focusd_mdm_CPPFLAGS = @INC_MDMTCS@
else
EXTRA_DIST += mdm.cpp
endif
