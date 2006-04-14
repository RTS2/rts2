SUBDIRS = model

bin_PROGRAMS = rts2-teld-gemini rts2-teld-bridge rts2-teld-dummy rts2-teld-mm2 rts2-teld-lx200 rts2-telmodeltest

noinst_HEADERS = telescope.h maps.h rts2devclicop.h ir.h

LDADD = ../utils/librts2block.a model/librts2model.a @LIB_M@ @LIB_NOVA@

rts2_teld_gemini_SOURCES = teld.cpp rts2devclicop.cpp tpmodel.c gemini.cpp

rts2_teld_bridge_SOURCES = teld.cpp rts2devclicop.cpp tpmodel.c bridge.cpp

rts2_teld_dummy_SOURCES = teld.cpp rts2devclicop.cpp dummy.cpp

rts2_teld_mm2_SOURCES = teld.cpp rts2devclicop.cpp mm2.cpp

rts2_teld_lx200_SOURCES = teld.cpp rts2devclicop.cpp lx200.cpp

rts2_telmodeltest_SOURCES = teld.cpp rts2devclicop.cpp telmodeltest.cpp

if OPENTPL
bin_PROGRAMS += rts2-teld-ir rts2-ir-errors
rts2_teld_ir_SOURCES = teld.cpp rts2devclicop.cpp tpmodel.c ir.cpp ir-main.cpp
rts2_teld_ir_LDFLAGS = @LIB_OPENTPL@
rts2_teld_ir_CPPFLAGS = -D HAVE_SYS_IPC_H @INC_OPENTPL@

rts2_ir_errors_SOURCES = teld.cpp rts2devclicop.cpp ir.cpp ir-errors.cpp
rts2_ir_errors_LDFLAGS = @LIB_OPENTPL@
rts2_ir_errors_CPPFLAGS = -D HAVE_SYS_IPC_H @INC_OPENTPL@
endif

if PARALIB
bin_PROGRAMS += rts2-teld-paramount
rts2_teld_paramount_SOURCES = teld.cpp rts2devclicop.cpp tpmodel.c paramount.cpp
rts2_teld_paramount_LDFLAGS = @LIB_PARA@
rts2_teld_paramount_CPPFLAGS = @INC_PARA@
endif
