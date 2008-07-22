if INDI
bin_PROGRAMS = rts2-indi
rts2_indi_SOURCES = indi.cpp # ${INDI}/indidrivermain.c ${INDI}/eventloop.c
rts2_indi_LDFLAGS = @INDI@/libs/liblilxml.a @INDI@/libs/libindicom.a ../utils/librts2block.a 
rts2_indi_CXXFLAGS = @INC_INDI@
rts2_indi_CFLAGS = @INC_INDI@
else
EXTRA_DIST = indi.cpp
endif
