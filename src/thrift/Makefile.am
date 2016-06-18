LDADD = @THRIFT_LIBS@
AM_CXXFLAGS = -Igen-cpp @THRIFT_CFLAGS@

if THRIFT

bin_PROGRAMS = rts2-thriftd
rts2_thriftd_SOURCES = mount.cpp

else

EXTRA_DIST = mount.cpp

endif
