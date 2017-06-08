noinst_HEADERS = wsd.h

if LIBWEBSOCKETS
bin_PROGRAMS = rts2-wsd

WSD_LDADD = @LIB_M@ @LIB_NOVA@ @LIBWEBSOCKETS_LIBS@
AM_CXXFLAGS = @NOVA_CFLAGS@ @MAGIC_CFLAGS@ @LIBARCHIVE_CFLAGS@ @LIBWEBSOCKETS_CFLAGS@ -I../../include

if PGSQL 

rts2_wsd_SOURCES = wsd.cpp http.c
rts2_wsd_CXXFLAGS = @LIBPG_CFLAGS@ ${AM_CXXFLAGS}
rts2_wsd_LDADD = -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2imagedb -L../../lib/pluto -lpluto -L../../lib/rts2 -lrts2 -L../../lib/xmlrpc++ -lrts2xmlrpc @LIBPG_LIBS@ @LIB_ECPG@ @LIBXML_LIBS@ @MAGIC_LIBS@ @CFITSIO_LIBS@ @LIB_CRYPT@ @LIBARCHIVE_LIBS@ ${WSD_LDADD}

else

rts2_wsd_SOURCES = wsd.cpp http.c
rts2_wsd_CXXFLAGS = ${AM_CXXFLAGS}
rts2_wsd_LDADD = ${WSD_LDADD}

endif

else
EXTRA_DIST=wsd.cpp http.c
endif
