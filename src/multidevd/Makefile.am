bin_PROGRAMS = rts2-sitech-focmirror rts2-apm-multidev

noinst_HEADERS = sitech-multidev.h sitech-focuser.h sitech-mirror.h apm-filter.h apm-aux.h apm-multidev.h

LDADD = -L../../lib/rts2 -lrts2 @LIB_NOVA@
AM_CXXFLAGS = @NOVA_CFLAGS@ -I../../include

rts2_sitech_focmirror_SOURCES = sitech-multidev.cpp sitech-focuser.cpp sitech-mirror.cpp

rts2_apm_multidev_SOURCES = apm-multidev.cpp apm-filter.cpp apm-aux.cpp
