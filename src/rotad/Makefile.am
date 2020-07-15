bin_PROGRAMS = rts2-rotad-dummy rts2-rotad-pyxis rts2-rotad-sitech rts2-rotad-pwiirf90r

noinst_HEADERS = sitech-rotator.h sitech-multi.h

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

rts2_rotad_dummy_SOURCES = dummy.cpp
rts2_rotad_pyxis_SOURCES = pyxis.cpp
rts2_rotad_sitech_SOURCES = sitech-multi.cpp sitech-rotator.cpp
rts2_rotad_pwiirf90r_SOURCES = pwiirf90r.cpp

