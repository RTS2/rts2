LDADD = @THRIFT_LIBS@

CLEANFILES = ObservatoryService.cpp rts2_types.cpp rts2_constants.cpp

if THRIFT

bin_PROGRAMS = rts2-thriftd rts2-thrift
rts2_thriftd_SOURCES = ObservatoryService.cpp rts2_types.cpp rts2_constants.cpp thriftd.cpp
rts2_thriftd_CXXFLAGS = -Iinterface -I../../include @NOVA_CFLAGS@ @THRIFT_CFLAGS@
rts2_thriftd_LDFLAGS = -L../../lib/rts2 -lrts2 -lpthread @LIB_NOVA@

rts2_thrift_SOURCES = ObservatoryService.cpp rts2_types.cpp rts2_constants.cpp thrift.cpp
rts2_thrift_CXXFLAGS = -Iinterface -I../../include @NOVA_CFLAGS@ @THRIFT_CFLAGS@
rts2_thrift_LDFLAGS = -L../../lib/rts2 -lrts2 -lpthread @LIB_NOVA@

ObservatoryService.cpp: rts2.thrift
	$(RM) -rf interface
	mkdir interface
	thrift -out interface --gen cpp rts2.thrift
	cp interface/ObservatoryService.cpp interface/rts2_types.cpp interface/rts2_constants.cpp .

else

EXTRA_DIST = thriftd.cpp thrift.cpp

endif
