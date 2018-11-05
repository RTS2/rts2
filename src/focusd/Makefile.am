SUBDIRS = 

bin_PROGRAMS = rts2-focusd-optec rts2-focusd-robofocus rts2-focusd-dummy rts2-focusd-opentpl \
	rts2-focusd-atc2 rts2-focusd-microfocuser rts2-focusd-nstep rts2-focusd-planewave rts2-focusd-apm \
	rts2-focusd-ng rts2-focusd-aaf

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

rts2_focusd_optec_SOURCES = optec.cpp

rts2_focusd_robofocus_SOURCES = robofocus.cpp

rts2_focusd_dummy_SOURCES = dummy.cpp

rts2_focusd_opentpl_SOURCES = opentpl.cpp

rts2_focusd_atc2_SOURCES = atc2.cpp

rts2_focusd_microfocuser_SOURCES = microfocuser.cpp

rts2_focusd_nstep_SOURCES = nstep.cpp

rts2_focusd_planewave_SOURCES = planewave.cpp

rts2_focusd_apm_SOURCES = apm.cpp

rts2_focusd_ng_SOURCES = ng.cpp

rts2_focusd_aaf_SOURCES = aaf.cpp

EXTRA_DIST = 

if FLI
bin_PROGRAMS += rts2-focusd-fli
bin_PROGRAMS += rts2-focusd-flitc
rts2_focusd_fli_SOURCES = fli.cpp
rts2_focusd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_focusd_fli_CPPFLAGS = @INC_FLI@ -I../../include
rts2_focusd_flitc_SOURCES = flitc.cpp
rts2_focusd_flitc_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_focusd_flitc_CPPFLAGS = @INC_FLI@ -I../../include
else
EXTRA_DIST += fli.cpp
EXTRA_DIST += flitc.cpp
endif

if MDMTCS
bin_PROGRAMS += rts2-focusd-mdm rts2-focusd-mdm-bait

rts2_focusd_mdm_SOURCES = mdm.cpp
rts2_focusd_mdm_LDFLAGS = @LIB_MDMTCS@
rts2_focusd_mdm_CPPFLAGS = @INC_MDMTCS@ -I../../include

rts2_focusd_mdm_bait_SOURCES = mdm_bait.cpp
rts2_focusd_mdm_bait_LDFLAGS = @LIB_MDMTCS@
rts2_focusd_mdm_bait_CPPFLAGS = @INC_MDMTCS@ -I../../include

else
EXTRA_DIST += mdm.cpp mdm_bait.cpp
endif
