lib_LTLIBRARIES = libpluto.la

noinst_PROGRAMS = obs_test obs_test2 test_sat sat_id test2 out_comp test_out

noinst_HEADERS = norad.h norad_in.h observe.h

LDADD = -lpluto @LIB_M@

libpluto_la_SOURCES = sgp.cpp sgp4.cpp sgp8.cpp sdp4.cpp sdp8.cpp deep.cpp basics.cpp get_el.cpp common.cpp observe.cpp tle_out.cpp

obs_test_SOURCES = obs_test.cpp

obs_test2_SOURCES = obs_test2.cpp

test_sat_SOURCES = test_sat.cpp

sat_id_SOURCES = sat_id.cpp

test2_SOURCES = test2.cpp

out_comp_SOURCES = out_comp.cpp

test_out_SOURCES = test_out.cpp
