bin_PROGRAMS = rts2-rotad-dummy rts2-rotad-pyxis

noinst_HEADERS = rotad.h

noinst_LIBRARIES = librts2rotad.a

LDADD = librts2rotad.a ../utils/librts2block.a @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@

librts2rotad_a_SOURCES = rotad.cpp

rts2_rotad_dummy_SOURCES = dummy.cpp
rts2_rotad_pyxis_SOURCES = pyxis.cpp
