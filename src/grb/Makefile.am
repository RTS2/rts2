bin_PROGRAMS = rts2-grbforward

noinst_HEADERS = grbd.h grbconst.h conngrb.h rts2grbfw.h connshooter.h augershooter.h

EXTRA_DIST = conngrb.ec connshooter.ec

CLEANFILES = conngrb.cpp connshooter.cpp

rts2_grbforward_SOURCES = forward.cpp rts2grbfw.cpp
rts2_grbforward_LDADD = ../utils/librts2block.a @LIB_M@ @LIB_NOVA@
rts2_grbforward_CXXFLAGS = @NOVA_CFLAGS@

if PGSQL

bin_PROGRAMS += rts2-grbd rts2-augershooter


nodist_rts2_grbd_SOURCES = conngrb.cpp
rts2_grbd_SOURCES = grbd.cpp rts2grbfw.cpp
rts2_grbd_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@
rts2_grbd_LDADD = ../utils/librts2block.a ../utilsdb/librts2blockdb.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@

nodist_rts2_augershooter_SOURCES = connshooter.cpp
rts2_augershooter_SOURCES = augershooter.cpp
rts2_augershooter_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@
rts2_augershooter_LDADD = ../utils/librts2block.a ../utilsdb/librts2blockdb.a @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_M@ 

%.cpp: %.ec
	@ECPG@ -o $@ $^

endif
