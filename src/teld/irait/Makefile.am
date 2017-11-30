AM_CXXFLAGS=@NOVA_CFLAGS@ -I../../../include

if MACOSX
AM_CXXFLAGS += -include ../../../include/compat/osx/compat.h
endif

lib_LTLIBRARIES = libirait.la

libirait_la_SOURCES = galil.cpp iraitenc.cpp

noinst_HEADERS = galil.h iraitenc.h
