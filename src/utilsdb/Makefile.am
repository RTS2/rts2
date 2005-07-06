# Process this file with automake to produce Makefile.in

noinst_LIBRARIES = librts2blockdb.a

librts2blockdb_a_SOURCES = rts2devicedb.cpp 
librts2blockdb_a_CXXFLAGS = @LIBPG_CFLAGS@

noinst_HEADERS = rts2devicedb.h

%.cpp: %.ec
	@ECPG@ -o $@ $^
