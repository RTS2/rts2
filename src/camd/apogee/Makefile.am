noinst_HEADERS = Apogee.h ApogeeLinux.h CameraIO_Linux.h

if ARMCPU
EXTRA_DIST = CameraIO_LinuxPCI.cpp
else

noinst_LIBRARIES = libapogee.a

libapogee_a_SOURCES = CameraIO_LinuxPCI.cpp
endif
