if PGSQL

bin_PROGRAMS = rts2-scheduler

rts2_scheduler_SOURCES = scheduler.cpp
rts2_scheduler_CXXFLAGS = @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @JPEG_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ -I../../include
rts2_scheduler_LDADD = -L../../lib/rts2scheduler -lrts2scheduler -L../../lib/rts2script -lrts2script -L../../lib/rts2db -lrts2db -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2fits -lrts2imagedb -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_CRYPT@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_M@ @LIB_JPEG@

endif
