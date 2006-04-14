bin_PROGRAMS = rts2-filterd-ifw rts2-filterd-dummy

noinst_HEADERS = filterd.h

LDADD = ../utils/librts2block.a

rts2_filterd_ifw_SOURCES = filterd.cpp ifw.cpp

rts2_filterd_dummy_SOURCES = filterd.cpp dummy.cpp

if FLI
bin_PROGRAMS += rts2-filterd-fli
rts2_filterd_fli_SOURCES = filterd.cpp fli.cpp
rts2_filterd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_filterd_fli_CXXFLAGS = @INC_FLI@
endif
