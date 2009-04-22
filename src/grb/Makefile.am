bin_PROGRAMS = rts2-grbforward

noinst_HEADERS = grbd.h grbconst.h rts2conngrb.h rts2grbfw.h rts2grbexecconn.h connshooter.h augershooter.h

EXTRA_DIST = rts2conngrb.ec connshooter.ec

CLEANFILES = rts2conngrb.cpp rts2connshooter.cpp

rts2_grbforward_SOURCES = forward.cpp rts2grbfw.cpp
rts2_grbforward_LDFLAGS = @LIB_M@ ../utils/librts2block.a
rts2_grbforward_CXXFLAGS = @NOVA_CFLAGS@

if PGSQL

bin_PROGRAMS += rts2-grbd rts2-augershooter

rts2_grbd_SOURCES = grbd.cpp rts2grbfw.cpp rts2grbexecconn.cpp rts2conngrb.cpp
rts2_grbd_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@
rts2_grbd_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

rts2_augershooter_SOURCES = augershooter.cpp connshooter.cpp
rts2_augershooter_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@
rts2_augershooter_LDFLAGS = @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ ../utils/librts2block.a ../utilsdb/librts2blockdb.a

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
