LDADD = @CHECK_LIBS@ -L../lib/rts2tel -lrts2tel -L../lib/sgp4 -lsgp4 -L../lib/pluto -lpluto -L../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@
AM_CXXFLAGS= @CHECK_CFLAGS@ ${CPPFLAGS} -I../include

if LIBCHECK
TESTS = check_tel_corr check_gem_hko check_gem_mlo check_altaz check_tle check_sgp4
check_PROGRAMS = check_tel_corr check_gem_hko check_gem_mlo check_altaz check_tle check_sgp4

noinst_HEADERS = check_utils.h gemtest.h altaztest.h

check_tel_corr_SOURCES = check_tel_corr.cpp gemtest.cpp
check_gem_hko_SOURCES = check_gem_hko.cpp gemtest.cpp
check_gem_mlo_SOURCES = check_gem_mlo.cpp gemtest.cpp
check_altaz_SOURCES = check_altaz.cpp altaztest.cpp
check_tle_SOURCES = check_tle.cpp
check_sgp4_SOURCES = check_sgp4.cpp

else
EXTRA_DIST=gemtest.h gemtest.cpp check_gem_mlo.cpp check_gem_hko.cpp check_altaz.cpp check_tle.cpp
endif
