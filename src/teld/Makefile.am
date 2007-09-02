SUBDIRS = model

bin_PROGRAMS = rts2-teld-gemini rts2-teld-dummy rts2-teld-mm2 rts2-teld-lx200

noinst_HEADERS = telescope.h maps.h rts2devclicop.h ir.h hms.h

EXTRA_DIST = teld.cpp rts2devclicop.cpp

LDADD = teld.o rts2devclicop.o ../utils/librts2block.a model/librts2model.a @LIB_M@ @LIB_NOVA@

rts2_teld_gemini_SOURCES = gemini.cpp hms.c

rts2_teld_dummy_SOURCES = dummy.cpp

rts2_teld_mm2_SOURCES = mm2.cpp hms.c

rts2_teld_lx200_SOURCES = lx200.cpp hms.c

if PGSQL
bin_PROGRAMS += rts2-telmodeltest

rts2_telmodeltest_SOURCES = telmodeltest.cpp
rts2_telmodeltest_LDADD = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CFITSIO@ ../writers/librts2imagedb.a ../utilsdb/librts2blockdb.a ${LDADD}
endif

if OPENTPL
bin_PROGRAMS += rts2-teld-ir rts2-ir-errors

rts2_teld_ir_SOURCES = ir-main.cpp ir.cpp
rts2_teld_ir_LDFLAGS = @LIB_OPENTPL@
rts2_teld_ir_CPPFLAGS = -D HAVE_SYS_IPC_H -fpermissive @INC_OPENTPL@

rts2_ir_errors_SOURCES = ir-errors.cpp ir.cpp
rts2_ir_errors_LDFLAGS = @LIB_OPENTPL@
rts2_ir_errors_CPPFLAGS = -D HAVE_SYS_IPC_H -fpermissive @INC_OPENTPL@
else
EXTRA_DIST += ir.cpp ir-main.cpp ir-errors.cpp
endif

if PARALIB
bin_PROGRAMS += rts2-teld-paramount
rts2_teld_paramount_SOURCES = paramount.cpp
rts2_teld_paramount_LDFLAGS = @LIB_PARA@
rts2_teld_paramount_CPPFLAGS = @INC_PARA@
endif
