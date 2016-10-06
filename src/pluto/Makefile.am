noinst_PROGRAMS = obs_test obs_test2 test_sat sat_id test2 out_comp test_out

noinst_DATA = obs_test.txt

AM_CXXFLAGS=-I../../include @NOVA_CFLAGS@
LDADD = -L../../lib/pluto -lpluto @LIB_M@ @LIB_NOVA@

obs_test_SOURCES = obs_test.cpp

obs_test2_SOURCES = obs_test2.cpp

test_sat_SOURCES = test_sat.cpp

sat_id_SOURCES = sat_id.cpp

test2_SOURCES = test2.cpp

out_comp_SOURCES = out_comp.cpp

test_out_SOURCES = test_out.cpp
