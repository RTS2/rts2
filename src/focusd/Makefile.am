SUBDIRS = 

bin_PROGRAMS = rts2-focusd-optec rts2-focusd-robofocus rts2-focusd-dummy rts2-focusd-opentpl

noinst_HEADERS = focusd.h

noinst_LIBRARIES = librts2focusd.a

librts2focusd_a_SOURCES = focusd.cpp

LDADD = librts2focusd.a ../utils/librts2block.a

rts2_focusd_optec_SOURCES = optec.cpp

rts2_focusd_robofocus_SOURCES = robofocus.cpp

rts2_focusd_dummy_SOURCES = dummy.cpp

rts2_focusd_opentpl_SOURCES = opentpl.cpp
rts2_focusd_opentpl_LDFLAGS = @LIB_NOVA@

if FLI
bin_PROGRAMS += rts2-focusd-fli
rts2_focusd_fli_SOURCES = fli.cpp
rts2_focusd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_focusd_fli_CPPFLAGS = @INC_FLI@
endif
