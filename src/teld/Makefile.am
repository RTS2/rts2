SUBDIRS = 

bin_PROGRAMS = rts2-teld-gemini rts2-teld-bridge rts2-teld-ir

noinst_HEADERS = telescope.h maps.h

LDADD = ../utils/librts2block.a ../utils/hms.o @LIB_M@

rts2_teld_gemini_SOURCES = teld.cpp tpmodel.c gemini.cpp
rts2_teld_gemini_LDFLAGS = @LIB_NOVA@

rts2_teld_bridge_SOURCES = teld.cpp tpmodel.c bridge.cpp
rts2_teld_bridge_LDFLAGS = @LIB_NOVA@

rts2_teld_ir_SOURCES = teld.cpp tpmodel.c ir.cpp
rts2_teld_ir_LDFLAGS = @LIB_NOVA@ -lssl -lopentpl -lttcommon
