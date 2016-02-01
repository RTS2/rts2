TESTS = check_gem_hko
check_PROGRAMS = check_gem_hko

LDADD = -L../lib/rts2tel -lrts2tel -L../lib/pluto -lpluto -L../lib/rts2 -lrts2 @LIB_M@ @LIB_NOVA@

check_gem_hko_SOURCES = check_gem_hko.cpp
check_gem_hko_CPPFLAGS = @CHECK_CFLAGS@ ${CPPFLAGS} -I../include
check_gem_hko_LDADD = @CHECK_LIBS@ ${LDADD}
