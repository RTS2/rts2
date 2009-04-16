bin_PROGRAMS = rts2-filterd-ifw rts2-filterd-dummy

noinst_HEADERS = filterd.h

noinst_LIBRARIES  = librts2filterd.a

librts2filterd_a_SOURCES = filterd.cpp

LDADD = librts2filterd.a ../utils/librts2block.a

rts2_filterd_ifw_SOURCES = ifw.cpp

rts2_filterd_dummy_SOURCES = dummy.cpp

if FLI
bin_PROGRAMS += rts2-filterd-fli
rts2_filterd_fli_SOURCES = fli.cpp
rts2_filterd_fli_LDFLAGS = @LIB_FLI@ @LIB_M@
rts2_filterd_fli_CXXFLAGS = @INC_FLI@
endif
