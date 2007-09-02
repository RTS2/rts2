SUBDIRS = 

bin_PROGRAMS = rts2-focusd-optec rts2-focusd-robofocus rts2-focusd-dummy

noinst_HEADERS = focuser.h

EXTRA_DIST = focusd.cpp

LDADD = focusd.o ../utils/librts2block.a

rts2_focusd_optec_SOURCES = optec.cpp

rts2_focusd_robofocus_SOURCES = robofocus.cpp

rts2_focusd_dummy_SOURCES = dummy.cpp

if OPENTPL
bin_PROGRAMS += rts2-focusd-ir
rts2_focusd_ir_SOURCES = ir.cpp
rts2_focusd_ir_LDFLAGS = @LIB_OPENTPL@ @LIB_PTHREAD@ @LIB_M@ @LIB_CFITSIO@ @LIB_NOVA@
rts2_focusd_ir_CPPFLAGS = -D HAVE_SYS_IPC_H -fpermissive @INC_OPENTPL@
endif

if FLI
bin_PROGRAMS += rts2-focusd-fli
rts2_focusd_fli_SOURCES = fli.cpp
rts2_focusd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_focusd_fli_CPPFLAGS = @INC_FLI@
endif
