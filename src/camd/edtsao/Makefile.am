noinst_HEADERS = edtswap.h sdvlib.h sdv_libpdv.h xos.h interface.h sdvpci.h edtreg.h

if EDTPDV
noinst_LIBRARIES = libedtsao.a

libedtsao_a_SOURCES = edtswap.c ccdpdv.c sleep.c saoprintblk.c
libedtsao_a_CFLAGS = @INC_EDTPDV@
endif
