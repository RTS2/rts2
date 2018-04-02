if LIBERFA

LDADD += -L../../lib/ucac5 -lrts2ucac5 -L../../lib/rts2 -lrts2 @ERFA_LIBS@ @LIB_NOVA@
AM_CXXFLAGS= @ERFA_CFLAGS@ @NOVA_CFLAGS@ -I../../include

bin_PROGRAMS = ucac5-idx ucac5-search

noinst_SCRIPTS = ucac5-download

ucac5_idx_SOURCES = idx.cpp

ucac5_search_SOURCES = search.cpp

endif
