if INDI
bin_PROGRAMS = rts2-indi
rts2_indi_SOURCES = indi.cpp 
rts2_indi_LDFLAGS = @INDI@/indidrivermain.o @INDI@/eventloop.o @INDI@/liblilxml.a @INDI@/libindicom.a ../utils/librts2block.a 
rts2_indi_CXXFLAGS = @INC_INDI@
else
EXTRA_DIST = indi.cpp
endif
