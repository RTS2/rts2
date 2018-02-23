noinst_HEADERS = UCAC5Record.hpp

if LIBERFA

LDADD += -L../rts2 -lrts2 @ERFA_LIBS@
AM_CXXFLAGS= @ERFA_CFLAGS@ -I../../include

bin_PROGRAMS = ucac5-idx

lib_LTLIBRARIES = librts2ucac5.la

librts2ucac5_la_SOURCES = UCAC5Record.cpp

ucac5_idx_SOURCES = idx.cpp
ucac5_idx_LDADD = -lrts2ucac5 ${LDADD}

endif
