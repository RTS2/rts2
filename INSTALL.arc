Instructions to install Astronomical Research Cameras driver
============================================================

For ARC drivers, you will need to have:

- astropci driver running in your kernel. Source package is available at
  http://www.astro-cam.com/arcpage.php?txt=software.php, you will need to get
  driver acccess password from ARC. Look for AstroPCI - PCI board device driver

- ARC PCI API, version 2.0 (C++). You can download it from ARC pages mentioned
  above, look for AstroPCI Device Driver Application Interface (API) version
  2.0. Unpack it to a directory, let's say /usr/src/arcapi

During ./configure step, you will need to provide path for ARC library using
--with-arc parameter. The passed directory should point to path under which Release and CController directories are located.

user@host:$ cd ~/rts2
user@host:~/rts2$ ./configure --with-arc=/usr/src/arcapi/ARC_API/
