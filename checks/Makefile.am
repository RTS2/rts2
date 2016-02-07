LDADD = -L../lib/rts2tel -lrts2tel -L../lib/pluto -lpluto -L../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@

if LIBCHECK
TESTS = check_gem_hko check_gem_mlo
check_PROGRAMS = check_gem_hko check_gem_mlo

noinst_HEADERS = gemtest.h

check_gem_hko_SOURCES = check_gem_hko.cpp gemtest.cpp
check_gem_hko_CPPFLAGS = @CHECK_CFLAGS@ ${CPPFLAGS} -I../include
check_gem_hko_LDADD = @CHECK_LIBS@ ${LDADD}

check_gem_mlo_SOURCES = check_gem_mlo.cpp gemtest.cpp
check_gem_mlo_CPPFLAGS = @CHECK_CFLAGS@ ${CPPFLAGS} -I../include
check_gem_mlo_LDADD = @CHECK_LIBS@ ${LDADD}
else
EXTRA_DIST=gemtest.h gemtest.cpp check_gem_mlo.cpp check_gem_hko.cpp
endif
