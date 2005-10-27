SUBDIRS = 

bin_PROGRAMS = rts2-focusd-optec rts2-focusd-robofocus rts2-focusd-dummy

noinst_HEADERS = focuser.h

rts2_focusd_optec_SOURCES = focusd.cpp optec.cpp
rts2_focusd_optec_LDFLAGS = ../utils/librts2block.a

rts2_focusd_robofocus_SOURCES = focusd.cpp robofocus.cpp
rts2_focusd_robofocus_LDFLAGS = ../utils/librts2block.a

rts2_focusd_dummy_SOURCES = focusd.cpp dummy.cpp
rts2_focusd_dummy_LDFLAGS = ../utils/librts2block.a

if OPENTPL
bin_PROGRAMS += rts2-focusd-ir
rts2_focusd_ir_SOURCES = focusd.cpp ir.cpp
rts2_focusd_ir_LDFLAGS = @LIB_OPENTPL@ @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@ ../utils/librts2block.a
rts2_focusd_ir_CPPFLAGS = -D HAVE_SYS_IPC_H @INC_OPENTPL@
endif

if FLI
bin_PROGRAMS += rts2-focusd-fli
rts2_focusd_fli_SOURCES = focusd.cpp fli.cpp
rts2_focusd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@ ../utils/librts2block.a
rts2_focusd_fli_CPPFLAGS = @INC_FLI@
endif
