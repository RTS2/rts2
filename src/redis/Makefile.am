noinst_HEADERS = redis.h

if HIREDIS

bin_PROGRAMS = rts2-redis
AM_CXXFLAGS = -std=c++11 -I../../include @LIBXML_CFLAGS@ @MAGIC_CFLAGS@ @HIREDIS_CFLAGS@
rts2_redis_SOURCES = redis.cpp

if PGSQL
rts2_redis_LDADD = ../../lib/rts2fits/librts2imagedb.la ../../lib/rts2db/librts2db.la @HIREDIS_LIBS@
else
rts2_redis_LDADD = ../../lib/rts2fits/llibrts2image.la ../../lib/rts2/librts2.la @HIREDIS_LIBS@
endif

else
EXTRA_DIST = redis.cpp
endif
