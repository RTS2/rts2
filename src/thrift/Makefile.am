LDADD = @THRIFT_LIBS@
AM_CXXFLAGS = -Igen-cpp @THRIFT_CFLAGS@

CLEANFILES = MountService.cpp rts2_types.cpp rts2_constants.cpp

if THRIFT

bin_PROGRAMS = rts2-thriftd
rts2_thriftd_SOURCES = thriftd.cpp MountService.cpp rts2_types.cpp rts2_constants.cpp
rts2_thriftd_CXXFLAGS = -Iinterface -I../../include @NOVA_CFLAGS@
rts2_thriftd_LDFLAGS = -L../../lib/rts2 -lrts2 -lpthread @LIB_NOVA@

thriftd.cpp: rts2.thrift
	$(RM) -rf interface
	mkdir interface
	thrift -out interface --gen cpp rts2.thrift
	cp interface/MountService.cpp interface/rts2_types.cpp interface/rts2_constants.cpp .

else

EXTRA_DIST = thriftd.cpp

endif
