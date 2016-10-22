LDADD = @CHECK_LIBS@ -L../lib/rts2tel -lrts2tel -L../lib/sgp4 -lsgp4 -L../lib/pluto -lpluto -L../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@
AM_CXXFLAGS= @CHECK_CFLAGS@ ${CPPFLAGS} -I../include

TESTS = check_python_libnova check_python_gpoint check_python_rts2
EXTRA_DIST = gpoint_in_altaz

if LIBCHECK
TESTS += check_tel_corr check_gem_hko check_gem_mlo check_altaz check_tle check_sgp4 check_timestamp check_gpointmodel check_message
check_PROGRAMS = check_tel_corr check_gem_hko check_gem_mlo check_altaz check_tle check_sgp4 check_timestamp check_gpointmodel check_message

noinst_HEADERS = check_utils.h gemtest.h altaztest.h

check_tel_corr_SOURCES = check_tel_corr.cpp gemtest.cpp altaztest.cpp
check_gem_hko_SOURCES = check_gem_hko.cpp gemtest.cpp
check_gem_mlo_SOURCES = check_gem_mlo.cpp gemtest.cpp
check_altaz_SOURCES = check_altaz.cpp altaztest.cpp
check_tle_SOURCES = check_tle.cpp
check_sgp4_SOURCES = check_sgp4.cpp

check_timestamp_SOURCES = check_timestamp.cpp

check_gpointmodel_SOURCES = check_gpointmodel.cpp

check_message_SOURCES = check_message.cpp

else
EXTRA_DIST+=gemtest.h gemtest.cpp check_gem_mlo.cpp check_gem_hko.cpp check_altaz.cpp check_tle.cpp check_sgp4.cpp check_timestamp.cpp check_gpointmodel.cpp check_message.cpp
endif
