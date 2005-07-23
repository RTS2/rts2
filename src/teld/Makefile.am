SUBDIRS = 

bin_PROGRAMS = rts2-teld-gemini rts2-teld-bridge rts2-teld-dummy rts2-teld-mm2

noinst_HEADERS = telescope.h maps.h

LDADD = ../utils/librts2block.a ../utils/hms.o @LIB_M@

rts2_teld_gemini_SOURCES = teld.cpp tpmodel.c gemini.cpp
rts2_teld_gemini_LDFLAGS = @LIB_NOVA@

rts2_teld_bridge_SOURCES = teld.cpp tpmodel.c bridge.cpp
rts2_teld_bridge_LDFLAGS = @LIB_NOVA@

rts2_teld_dummy_SOURCES = teld.cpp dummy.cpp
rts2_teld_dummy_LDFLAGS = @LIB_NOVA@

rts2_teld_mm2_SOURCES = teld.cpp mm2.cpp
rts2_teld_mm2_LDFLAGS = @LIB_NOVA@

if OPENTPL
bin_PROGRAMS += rts2-teld-ir
rts2_teld_ir_SOURCES = teld.cpp tpmodel.c ir.cpp
rts2_teld_ir_LDFLAGS = @LIB_NOVA@ @LIB_OPENTPL@
rts2_teld_ir_CPPFLAGS = -D HAVE_SYS_IPC_H @INC_OPENTPL@
endif

if PARALIB
bin_PROGRAMS += rts2-teld-paramount
rts2_teld_paramount_SOURCES = teld.cpp tpmodel.c paramount.cpp
rts2_teld_paramount_LDFLAGS = @LIB_NOVA@ @LIB_PARA@
rts2_teld_paramount_CPPFLAGS = @INC_PARA@
endif
