noinst_HEADERS = UCAC5Record.hpp UCAC5Idx.hpp UCAC5Bands.hpp

if LIBERFA

LDADD += -L../rts2 -lrts2 @ERFA_LIBS@ @LIB_NOVA@
AM_CXXFLAGS= @ERFA_CFLAGS@ @NOVA_CFLAGS@ -I../../include

bin_PROGRAMS = ucac5-idx ucac5-search

noinst_SCRIPTS = ucac5-download

lib_LTLIBRARIES = librts2ucac5.la

librts2ucac5_la_SOURCES = UCAC5Record.cpp UCAC5Idx.cpp UCAC5Bands.cpp

ucac5_idx_SOURCES = idx.cpp
ucac5_idx_LDADD = -lrts2ucac5 ${LDADD}

ucac5_search_SOURCES = search.cpp
ucac5_search_LDADD = -lrts2ucac5 ${LDADD}

endif
