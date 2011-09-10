LDADD = -L../plan -lscript -L../../lib/xmlrpc++ -lrts2xmlrpc -L../../lib/rts2db -lrts2db -L../../lib/rts2fits -lrts2image -lrts2imagedb -L../../lib/rts2 -lrts2 @LIBXML_LIBS@ @LIBPG_LIBS@ @LIB_ECPG@ @LIB_NOVA@ @LIB_CFITSIO@ @LIB_JPEG@
AM_CXXFLAGS = @LIBXML_CFLAGS@ @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@

bin_PROGRAMS = rts2-image

rts2_image_SOURCES = appimagemanip.cpp

EXTRA_DIST = airmasscale.ec
CLEANFILES = airmasscale.cpp

if PGSQL

bin_PROGRAMS += rts2-nightreport rts2-targetlist rts2-target rts2-tpm rts2-obsinfo rts2-user rts2-newtarget rts2-simbadinfo rts2-plan rts2-airmasscale

noinst_HEADERS = rts2targetapp.h

rts2_nightreport_SOURCES = nightreport.cpp

rts2_targetlist_SOURCES = targetlist.cpp

rts2_target_SOURCES = target.cpp

rts2_tpm_SOURCES = tpm.cpp

rts2_obsinfo_SOURCES = obsinfo.cpp

rts2_user_SOURCES = user.cpp

rts2_newtarget_SOURCES = newtarget.cpp rts2targetapp.cpp
rts2_newtarget_CXXFLAGS = @LIBXML_CFLAGS@ ${AM_CXXFLAGS}

rts2_simbadinfo_SOURCES = simbadinfo.cpp rts2targetapp.cpp
rts2_simbadinfo_CXXFLAGS = @LIBXML_CFLAGS@ ${AM_CXXFLAGS}

rts2_plan_SOURCES = planapp.cpp
rts2_plan_CXXFLAGS = @LIBPG_CFLAGS@ @NOVA_CFLAGS@ @CFITSIO_CFLAGS@ @JPEG_CFLAGS@ @LIBXML_CFLAGS@

nodist_rts2_airmasscale_SOURCES = airmasscale.cpp

%.cpp: %.ec
	@ECPG@ -o $@ $^

else

EXTRA_DIST += nightreport.cpp targetlist.cpp target.cpp tpm.cpp obsinfo.cpp user.cpp newtarget.cpp rts2targetapp.cpp simbadinfo.cpp planapp.cpp airmasscale.ec

endif
