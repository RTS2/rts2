# That makefile is used for 2.6.x kernels. Should work on 2.4.x as
# well
obj-m := phot.o

all:
	make modules -C /lib/modules/`uname -r`/build SUBDIRS=`pwd`

clean:
	rm -f *.o *.ko
