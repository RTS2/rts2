# Process this file with autoconf to produce a configure script.C
AC_INIT(rts2, 0.9.4, petr@kubanek.net)

AC_CANONICAL_BUILD
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([gnu dist-bzip2])

LT_INIT()

AC_PREREQ(2.50)

AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AM_PATH_PYTHON

# check for lang
AC_LANG_CPLUSPLUS

CFLAGS="-Wall -Wshadow -g"
AC_SUBST(CFLAGS)
CXXFLAGS="-Wall -Wshadow -g"
AC_SUBST(CXXFLAGS)

AC_CHECK_LIB([m], [sqrt], LIB_M="-lm";
LIBS="${LIBS} -lm"; AC_SUBST(LIB_M), [cat << EOF
**** You don't have math library.
**** Please install one and make sure, that it is available. 
EOF
exit 1])

AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 floor gethostbyname gettimeofday inet_ntoa memmove memset select socket strchr strdup strerror strtol bzero mkdir sqrt strcasecmp strncasecmp pow getaddrinfo getopt_long flock strtod isinf scandir alphasort isblank strcasestr trunc getline inotify_init inotify_add_watch inotify_init1 nftw round strtof isatty])

AC_FUNC_CHOWN 
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES 
AC_FUNC_STAT
AC_FUNC_STRFTIME 
AC_FUNC_VPRINTF 
AC_TYPE_MODE_T

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([limits.h sys/ioccom.h argz.h arpa/inet.h dirent.h fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h termios.h unistd.h sys/inotify.h curses.h ncurses/curses.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_ARG_ENABLE(focusing,
[  --disable-focusing      disables focusing in camd (=>camd doesn't need cfitsio to build)],
[case "${enableval}" in
	yes) focusing=yes ;;
	no)  focusing=no ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-focusing) ;;
esac],[focusing=yes])

AM_CONDITIONAL(FOCUSING, test x$focusing = xyes)
AC_ARG_WITH(x-focusc,
[  --without-x-focusc      disables xfocusc],
[case "${withval}" in
	yes) xfocusc=yes ;;
	no)  xfocusc=no ;;
	*) AC_MSG_ERROR(bad value ${enableval} for --enable-xfocusc) ;;
esac],[xfocusc=yes])

AM_CONDITIONAL(XFOCUSC, test x$xfocusc = xyes)

AC_ARG_WITH(libnova,
[  --with-libnova       path to libnova or "no" if libnova is not installed],
[case "${withval}" in
	yes) libnova=yes ;;
	no)  libnova=no ;;
	*) libnova=${withval} ;;
esac],[libnova=yes])

AM_CONDITIONAL(LIBNOVA, test x$libnova != xno)

AC_ARG_WITH(cfitsio,
[  --with-cfitsio       path to CFITSO library or "no" if cfitsio is not installed],
[case "${withval}" in
	yes) cfitsio=yes ;;
	no)  cfitsio=no ;;
	*) cfitsio=${withval} ;;
esac],[cfitsio=yes])

AM_CONDITIONAL(CFITSIO, test x$cfitsio != xno)

AC_ARG_WITH(ncurses,
[  --with-ncurses       path to ncurses library or "yes" if ncurses is in standard path],
[case "${withval}" in
	yes) ncurses=yes ;;
	*) ncurses=${withval} ;;
esac],[ncurses=yes])

dnl Check for GraphicsMagic++ library

AC_ARG_WITH(graphicsmagic,
[  --with-graphicsmagic    path to GraphicsMagic++ config or "no" if it is not installed],
[case "${withval}" in
	yes) graphicsmagic_config="GraphicsMagick++-config" ;;
	no)  graphicsmagic_config=no ;;
	*) graphicsmagic_config=${withval} ;;
esac],[graphicsmagic_config="GraphicsMagick++-config"])

AC_MSG_CHECKING(looking for GraphicsMagick++ library)
if ${graphicsmagic_config} --libs > /dev/null 2>&1
then
	GRAPHICSMAGICK_VERS=`${graphicsmagic_config} --version`
	AC_MSG_RESULT(${GRAPHICSMAGICK_VERS} found)
else
	GRAPHICSMAGICK_VERS="no"
	AC_MSG_RESULT(${graphicsmagic_config} not found)
	graphicsmagic_config="no"
fi

AH_TEMPLATE([JSONSOUP],[JSON and SOUP libraries are present])

dnl check for JSON Glib
PKG_CHECK_MODULES([JSONGLIB], [json-glib-1.0 >= 0.1 libsoup-2.4 >= 0.1], AC_DEFINE_UNQUOTED(JSONSOUP, "1", [JSON and SOUP libraries are present]),AC_MSG_RESULT(not found))

AM_CONDITIONAL(JSONSOUP, test x$JSONGLIB_CFLAGS != x)

dnl Check for ROOT (to build contrib scripts)

AC_ARG_WITH(cern-root,
[  --with-cern-root        path to CERN ROOT config or "no" if it is not installed],
[case "${withval}" in
	yes) root_config="root-config" ;;
	no)  root_config=no ;;
	*) root_config=${withval} ;;
esac],[root_config="root-config"])

AC_MSG_CHECKING(looking for ROOT libraries)
if ${root_config} --libs > /dev/null 2>&1
then
	ROOT_VERS=`${root_config} --version`
	AC_MSG_RESULT(${ROOT_VERS} found)
else
	ROOT_VERS="no"
	AC_MSG_RESULT(${root_config} not found)
	root_config="no"
fi

dnl Check for the XML library
AC_ARG_WITH(xml2,
[  --with-xml2             specify path for libxml2 config file],
[case "${withval}" in
	yes) XML_CONFIG=xml2-config ;;
	no) AC_MSG_ERROR(You really need to have libxml2-dev) ;;
	*) XML_CONFIG=${withval} ;;
esac],[XML_CONFIG=xml2-config])
AC_MSG_CHECKING(looking for xml2 library)
if ${XML_CONFIG} --libs print > /dev/null 2>&1
then
	XMLVERS=`${XML_CONFIG} --version`
	LIBXML_LIBS=`${XML_CONFIG} --libs`
	LIBXML_CFLAGS=`${XML_CONFIG} --cflags`
	AC_SUBST(LIBXML_LIBS)
	AC_SUBST(LIBXML_CFLAGS)
	AC_MSG_RESULT(${XMLVERS} found)
else
	AC_MSG_ERROR([Could not find ${XML_CONFIG} anywhere, apt-get install libxml2-dev or check ftp://xmlsoft.org/])
fi

AC_ARG_WITH(pgsql,
[  --without-pgsql         don't build pgsql dependent clients],
[case "${withval}" in
	yes) psql=yes ;;
	no)  psql=no ;;
	*) AC_MSG_ERROR(bad value ${withval} for --without-pgsql) ;;
esac],[psql=yes])

AC_ARG_WITH(wcs,
[  --with-wcs           path to WCS programs],
[case "${withval}" in
	yes) libwcs=yes ;;
	no)  libwcs=no ;;
	*) libwcs=${withval} ;;
esac],[libwcs=yes])


# check for EPICS
AC_ARG_WITH(epics,
[  --with-epics 	path to EPICS library, build epics dependent binaries],
[case "${withval}" in
	yes) epics=yes ;;
	no)  epics=no ;;
	*) epics=${withval} ;;
esac],[epics=no])

AM_CONDITIONAL(EPICS, test x$epics != xno)

AC_ARG_WITH(sbig,
[  --with-sbig             path to SBIG USB CCD driver, build SBIG USB CCD driver],
SBIG="${withval}";
LIB_SBIG="-L${SBIG} -lsbigudrv $SBIG/csbigcam.o $SBIG/csbigimg.o -lusb";
INC_SBIG="-I${SBIG}",
[SBIG=no])

AM_CONDITIONAL(SBIG, test x$SBIG != xno)
AC_SUBST(SBIG)
AC_SUBST(LIB_SBIG)
AC_SUBST(INC_SBIG)

AC_ARG_WITH(andor,
[  --with-andor            path to Andor CCD driver, build Andor CCD driver],
ANDOR="${withval}";
LIB_ANDOR="-landor",
[ANDOR=no])

AM_CONDITIONAL(ANDOR, test x$ANDOR != xno)
AC_SUBST(ANDOR)
AC_SUBST(LIB_ANDOR)
AC_SUBST(INC_ANDOR)

# test for EDT PDV boards
AC_ARG_WITH(edtpdv,
[  --with-edtpdv           path to EDT-PDV SDK files, needed to build EDT cameras (http://www.edt.com/software],
EDTPDV="${withval}";
LIB_EDTPDV="-L${EDTPDV} -lpdv -ldl";
INC_EDTPDV="-I${EDTPDV}",
[EDTPDV=no])

AM_CONDITIONAL(EDTPDV, test x$EDTPDV != xno)
AC_SUBST(EDTPDV)
AC_SUBST(LIB_EDTPDV)
AC_SUBST(INC_EDTPDV)

# Test for ARC/Leech controller headers
AC_ARG_WITH(arc,
[  --with-arc              path to Astronomical Research Cameras headers, version 2.0. You can download them from http://www.astro-cam.com/arcpage.php?txt=software.php (you will need to get password from ARC).],
ARC="${withval}";
if test x${target_os} == xcygwin; then
	LIB_ARC="${ARC}/CController/CController.a ${ARC}/CDeinterlace/CDeinterlace.o /lib/w32api/libsetupapi.a";
	INC_ARC="-I${ARC}"
else
	LIB_ARC="-L${ARC}/Release -lCController";
	INC_ARC="-I${ARC}"
fi,
[ARC=no])

AH_TEMPLATE([ARC_API_1_7],[If ARC version 1.7 is used])
AC_ARG_WITH(arc-1-7,
[  --with-arc-1-7          path to Astronomical Research Cameras headers, version 1.7. You can download them from http://www.astro-cam.com/arcpage.php?txt=software.php (you will need to get password from ARC).],
ARC_1_7="${withval}";
LIB_ARC="-lastropciAPI";
INC_ARC="-DARC_API_1_7 -I${ARC_1_7}/src",
[ARC_1_7=no])

if test x$ARC_1_7 != xno; then
	AC_DEFINE_UNQUOTED([ARC_API_1_7],1,[If ARC version 1.7 is used])
fi

AM_CONDITIONAL(ARC, test x$ARC != xno -o x$ARC_1_7 != xno)
AC_SUBST(ARC)
AC_SUBST(LIB_ARC)
AC_SUBST(INC_ARC)

# for Alta we need special library:(
AC_ARG_WITH(alta,
[  --with-alta             path to Apogee Alta CCD driver, build Apogee Alta CCD driver],
# checks for libusb-config
AC_CHECK_PROG(LIBUSB_CONFIG, libusb-config, yes, no)
if test "$LIBUSB_CONFIG" = "yes"; then
  LIBUSB_LDFLAGS=`libusb-config --libs`
  LIBUSB_CFLAGS=`libusb-config --cflags`
  LIBUSB_VERSION=`libusb-config --version`
else
  AC_MSG_ERROR([libusb not found!])
fi
ALTA="${withval}";
LIB_ALTA_USB="${ALTA}/apogee_USB.so ${LIBUSB_LDFLAGS}";
LIB_ALTA_NET="${ALTA}/apogee_NET.so ${LIBUSB_LDFLAGS}";
INC_ALTA="-I${ALTA} -I${ALTA}/FpgaRegs ${LIBUSB_CFLAGS}",
[ALTA=no])

AM_CONDITIONAL(ALTA, test x$ALTA != xno)
AC_SUBST(ALTA)
AC_SUBST(LIB_ALTA_USB)
AC_SUBST(LIB_ALTA_NET)
AC_SUBST(LIB_ALTA_FILTER)
AC_SUBST(INC_ALTA)

# FLI..
AC_ARG_WITH(fli,
[  --with-fli             path to libfli CCD driver, build FLI CCD driver],
FLI="${withval}";
LIB_FLI="-L${FLI} -lfli";
INC_FLI="-I${FLI}",
[FLI=no])

AM_CONDITIONAL(FLI, test x$FLI != xno)
AC_SUBST(FLI)
AC_SUBST(LIB_FLI)
AC_SUBST(INC_FLI)

AC_ARG_WITH(miccd,
[  --with-miccd           path to MI CCD driver, build MI CCD driver],
MICCD="${withval}";
LIB_MICCD="${MICCD}/lib/libmiccd.a";
INC_MICCD="-I${MICCD}/include",
[MICCD=no])

AM_CONDITIONAL(MICCD, test x$MICCD != xno)
AC_SUBST(MICCD)
AC_SUBST(LIB_MICCD)
AC_SUBST(INC_MICCD)

AC_ARG_WITH(k8055,
[  --with-k8055           path to Velleman k8055 library],
K8055="${withval}";
LIB_K8055="-lk8055 -lusb",
K8055=no;
LIB_K8055='')

AH_TEMPLATE([WITH_K8055],[K8055 library is not present])

if test x$K8055 != xno; then
	AC_DEFINE_UNQUOTED(WITH_K8055, "1", [K8055 library is present])
fi

AM_CONDITIONAL(K8055, test x$K8055 != xno)
AC_SUBST(K8055)
AC_SUBST(LIB_K8055)

# check for COMEDI
AC_ARG_WITH(comedi,
[  --with-comedi	path to COMEDI library, build comedi dependent binaries],
COMEDI="${withval}";
LIB_COMEDI="-lcomedi";
if [ "x${withval}" != "xyes" ]; then INC_COMEDI="-I${withval}"; else INC_COMEDI=""; fi,
[COMEDI=no])

AM_CONDITIONAL(COMEDI, test x$COMEDI != xno)
AC_SUBST(COMEDI)
AC_SUBST(LIB_COMEDI)
AC_SUBST(INC_COMEDI)

# check for paramount library 
AC_ARG_WITH(paralib,
[  --with-paralib          path to Paramount library, build Paramount driver],
PARALIB="${withval}";
LIB_PARA="${PARALIB}/libmks3.o";
INC_PARA="-I${PARALIB}",
[PARALIB=no])

AM_CONDITIONAL(PARALIB, test x$PARALIB != xno)
AC_SUBST(PARALIB)
AC_SUBST(LIB_PARA)
AC_SUBST(INC_PARA)

AC_ARG_WITH(mdm,
[  --with-mdm              path to MDM (Kitt Peak, Arizona) TCS library, build MDM driver],
MDMTCS="${withval}";
LIB_MDMTCS="${MDMTCS}/libtcs.a"
INC_MDMTCS="-I${MDMTCS}",
[MDMTCS=no])

AM_CONDITIONAL(MDMTCS, test x$MDMTCS != xno)
AC_SUBST(MDMTCS)
AC_SUBST(LIB_MDMTCS)
AC_SUBST(INC_MDMTCS)

# check for PCI8255
AC_ARG_WITH(pci8255,
[  --with-pic8255          path to PCI8255 sources, build Dublin dome],
PCI8255="${withval}";
LIB_PCI8255="-L${PCI8255}";
INC_PCI8255="-I${PCI8255}",
[PCI8255=no])

AM_CONDITIONAL(PCI8255, test x$PCI8255 != xno)
AC_SUBST(PCI8255)
AC_SUBST(LIB_PCI8255)
AC_SUBST(INC_PCI8255)

# check for GPIB
AC_ARG_WITH(gpib,
[  --with-gpib             build with support for GPIB devices],
GPIBLIB="${withval}";
LIB_GPIB="-lgpib",
[GPIBLIB=no])

if test "x$GPIBLIB" != "xno" ; then
  AC_CHECK_HEADERS([gpib/ib.h])
fi

AM_CONDITIONAL(GPIBLIB, test x$GPIBLIB != xno)
AC_SUBST(GPIBLIB)
AC_SUBST(LIB_GPIB)

# Check for A3200
AC_ARG_WITH(a3200,
[  --with-a3200           path for A3200 SDK],
A3200="${withval}";
LIB_A3200="${withval}/Lib/A32Cmplr.lib ${withval}/Lib/A32Sys.lib";
INC_A3200="-I${withval}/Include",
[A3200=no])

AM_CONDITIONAL(A3200, test x$A3200 != xno)
AC_SUBST(A3200)
AC_SUBST(LIB_A3200)
AC_SUBST(INC_A3200)

# Check for Boltwood
AC_ARG_WITH(boltwood,
[  --with-boltwood        path for Boltwood SDK],
BOLTWOOD="${withval}";
LIB_BOLTWOOD="${withval}/weather/libOCWeather.a ${withval}/common/libOCCdrivers.a ${withval}/util/libMTutil.a";
INC_BOLTWOOD="-I${withval}",
[BOLTWOOD=no])

AM_CONDITIONAL(BOLTWOOD, test x$BOLTWOOD != xno)
AC_SUBST(BOLTWOOD)
AC_SUBST(LIB_BOLTWOOD)
AC_SUBST(INC_BOLTWOOD)


# check for T-Balancer bigNG
AC_ARG_WITH(bigng,
[  --with-bigng            path for Tban SDK (libtan)],
BIGNG="${withval}";
LIB_BIGNG="${withval}/big_ng.o ${withval}/parser.o ${withval}/tban.o ${withval}/libtban.a";
INC_BIGNG="-I${withval}", 
[BIGNG=no])

AM_CONDITIONAL(BIGNG, test x$BIGNG != xno)
AC_SUBST(BIGNG)
AC_SUBST(LIB_BIGNG)
AC_SUBST(INC_BIGNG)

# Define prefix variable

AH_TEMPLATE([PREFIX],[prefix])
if test "x${prefix}" = "xNONE"; then
        AC_DEFINE_UNQUOTED(PREFIX, "", [prefix])
else
        AC_DEFINE_UNQUOTED(PREFIX, "$prefix", [prefix])
fi
AC_SUBST(PREFIX)
  
# Define lock file prefix
AC_ARG_WITH(lock,
[  --with-lock             specify lock file path prefix, default to /var/run/rts2_],
LOCK_PREFIX="${withval}",
LOCK_PREFIX="/var/run/rts2_")

AH_TEMPLATE([LOCK_PREFIX],[Lock file prefix path])

AC_DEFINE_UNQUOTED(LOCK_PREFIX, "$LOCK_PREFIX", [Lock file prefix path])

# Default centrald port
AC_ARG_WITH(port,
[  --with-port             default centrald port. If this value is not specified, 617 is used],
CENTRALD_PORT="${withval}",
CENTRALD_PORT="617")

AH_TEMPLATE([CENTRALD_PORT],[Default centrald port])

AC_DEFINE_UNQUOTED(CENTRALD_PORT, "$CENTRALD_PORT", [Default centrald port])


# Checks for libraries.

if test "x${cfitsio}" != "xno"; then
  if test "x${cfitsio}" != "xyes"; then
    LDFLAGS="${LDFLAGS} -L${cfitsio}"
  fi
  AC_CHECK_LIB([cfitsio], [ffinit], LIB_CFITSIO="-lcfitsio"; 
, [cat <<EOF
**** You don't have cfitsio installed.
**** Please download one from 
**** http://heasarc.gsfc.nasa.gov/docs/software/fitsio/fitsio.html
EOF
exit 1])
  if test "x${cfitsio}" != "xyes"; then
    LIB_CFITSIO="-L${cfitsio} ${LIB_CFITSIO}"
    CFITSIO_CFLAGS="-I${cfitsio}"
  else
    CFITSIO_CFLAGS=""
  fi
  AC_SUBST(CFITSIO_CFLAGS)
  AC_SUBST(LIB_CFITSIO)
fi

if test "x${libwcs}" != "xno"; then
  if test "x${libwcs}" != "xyes"; then
    LDFLAGS="${LDFLAGS} -L${libwcs}/libwcs"
  fi
  AC_CHECK_LIB([wcs], [wcs2pix], LIBWCS_LDFLAGS="-lwcs", 
  [cat <<EOF
**** You don't have libwcs installed.
**** You can download one from
**** http://tdc-www.harvard.edu/software/wcstools/.
**** Please note that there is no standart procedure to install
**** that library. Please see INSTALL file for instructions how to install
**** WCS library.
EOF
  exit 1])
  if test "x${libwcs}" != "xyes"; then
    LIBWCS_LDFLAGS="-L${libwcs}/libwcs -lwcs"
    LIBWCS_CFLAGS="-I${libwcs}"
  else
    LIBWCS_LDFLAGS="-lwcs"
  fi
fi

AM_CONDITIONAL(LIBWCS, test x$libwcs != xno)
AC_SUBST(LIBWCS_CFLAGS)
AC_SUBST(LIBWCS_LDFLAGS)

if test "x${ncurses}" != "xyes"; then
  LDFLAGS="${LDFLAGS} -L${ncurses}/lib"
fi     

AC_CHECK_LIB([ncurses], [mvwprintw], LIBNCURSES_LDFLAGS="-lncurses";
AC_SUBST(LIB_NCURSES), [cat << EOF
**** You don't have ncurser libraries.
**** Please install ncursers (you need devel package as well)
EOF
exit 1])

if test "x${ncurses}" != "xyes"; then
  LIBNCURSES_CFLAGS="-I${ncurses}/include"
  LIBNCURSES_LDFLAGS="-L${ncurses}/lib -lncurses"
fi

AC_SUBST(LIBNCURSES_CFLAGS)
AC_SUBST(LIBNCURSES_LDFLAGS)

if test "x${libnova}" != "xyes"; then
  LDFLAGS="${LDFLAGS} -L${libnova}/lib"
fi
AC_CHECK_LIB([nova], [ln_get_version], LIB_NOVA="-lnova", 
[cat << EOF
**** You don't have libnova library version >= 0.10.
**** Please get one from http://libnova.sourceforge.net.
EOF
exit 1])
if test "x${libnova}" != "xyes"; then
  LIB_NOVA="-L${libnova}/lib ${LIB_NOVA}"
  NOVA_CFLAGS="-I${libnova}/include"
else
  NOVA_CFLAGS=""
fi
AC_SUBST(LIB_NOVA)
AC_SUBST(NOVA_CFLAGS)

AC_CHECK_LIB([pthread], [pthread_exit], LIB_PTHREAD="-lpthread";
AC_SUBST(LIB_PTHREAD), [cat << EOF
**** You don't have pthread library installed.
**** Please try to get it somewhere.
EOF
exit 1])


AH_TEMPLATE([HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF],[If Libnova provides ln_get_heliocentric_time_diff function])

AC_CHECK_DECL(ln_get_heliocentric_time_diff, , ,[#include <libnova/libnova.h>])
if test "x"$ac_cv_have_decl_ln_get_heliocentric_time_diff = "xyes"; then
	AC_DEFINE_UNQUOTED(HAVE_DECL_LN_GET_HELIOCENTRIC_TIME_DIFF, 1, [If Libnova provides ln_get_heliocentric_time_diff function])
fi

AH_TEMPLATE([HAVE_DECL_LN_GET_ALT_FROM_AIRMASS],[If Libnova provides ln_get_alt_from_airmass function])

AC_CHECK_DECL(ln_get_alt_from_airmass, , ,[#include <libnova/libnova.h>])
if test "x"$ac_cv_have_decl_ln_get_alt_from_airmass= "xyes"; then
	AC_DEFINE_UNQUOTED(HAVE_DECL_LN_GET_ALT_FROM_AIRMASS, 1, [If Libnova provides ln_get_alt_from_airmass function])
fi

if test "x"$xfocusc = "xyes"; then
	AC_PATH_XTRA
	AC_SUBST([RTS2_XFOCUSC], ['rts2-xfocusc${EXEEXT}'])
fi

AH_TEMPLATE([HAVE_LIBJPEG],[If GraphicsMagicsk++ is installed])

if test "x${graphicsmagic_config}" != "xno"; then
  LIB_JPEG=`${graphicsmagic_config} --libs`
  JPEG_CFLAGS=`${graphicsmagic_config} --cppflags`
  AC_DEFINE_UNQUOTED(HAVE_LIBJPEG,1,[Whenever JPEG is installed])
else
  LIB_JPEG=""
  JPEG_CFLAGS=""
fi
AC_SUBST(LIB_JPEG)
AC_SUBST(JPEG_CFLAGS)

AM_CONDITIONAL(GRAPHICSMAGICK, test x$graphicsmagic_config != xno)

AH_TEMPLATE([HAVE_CERN_ROOT],[If CERN ROOT package is installed])

if test "x${root_config}" != "xno"; then
  LIB_ROOT=`${root_config} --libs`
  ROOT_CXXFLAGS="-I `${root_config} --incdir`"
  AC_DEFINE_UNQUOTED(HAVE_CERN_ROOT,1,[If CERN ROOT package is installed])
else
  LIB_ROOT=""
  ROOT_CXXFLAGS=""
fi
AC_SUBST(LIB_ROOT)
AC_SUBST(ROOT_CXXFLAGS)

AM_CONDITIONAL(CERN_ROOT, test x$root_config != xno)

AC_CHECK_LIB([archive],[archive_write_data], LIBARCHIVE_LIBS="-larchive", LIBARCHIVE_CFLAGS="")
AC_SUBST(LIBARCHIVE_LIBS)
AC_SUBST(LIBARCHIVE_CFLAGS)

AH_TEMPLATE([HAVE_LIBARCHIVE],[If libarchive is present])
 
if test "x$LIBARCHIVE_LIBS" != "x"; then
	AC_DEFINE_UNQUOTED([HAVE_LIBARCHIVE],1,[If libarchive is present])
	libarchive="yes"
else
	libarchive="no"
fi

if test "x$COMEDI" != "xno"; then
	AC_CHECK_LIB([comedi], [comedi_open], LIB_COMEDI="-lcomedi";
	AC_SUBST(LIB_COMEDI), [cat << EOF
#**** You don't have comedi library installed.
**** Please download it from http://www.comedi.org
EOF
	exit 1])
fi

if test "x$epics" != "xno"; then
	if test `uname` == 'Linux'; then
		AC_CHECK_LIB([readline],[readline],,[cat << EOF
**** You do not have readline library. For some reasons, it is
**** needed if you build with EPICS. Please install readline-dev package.
EOF
			exit 1])
	fi
  	if test "x$epics" != "xyes"; then
		if test "x$EPICS_HOST_ARCH" = "x"; then
		  	cat << EOF
**** Please define EPICS_HOST_ARCH varible. It content should be same as
**** the variable you used when you build EPICS library.
EOF
			exit 1
		fi
		LDFLAGS="${LDFLAGS} -L${epics}/lib/${EPICS_HOST_ARCH}"
		LIB_EPICS="-L${epics}/lib/${EPICS_HOST_ARCH}"
		if echo $EPICS_HOST_ARCH | grep ^linux; then
			INC_EPICS="-I${epics}/include -I${epics}/include/os/Linux"
		else
		if echo $EPICS_HOST_ARCH | grep ^solaris; then
			INC_EPICS="-I${epics}/include -I${epics}/include/os/solaris"
		else
		  	echo "**** Unsupported EPICS_HOST_ARCH argument ${EPICS_HOST_ARCH}"
			exit 1
		fi
		fi
	else
		LIB_EPICS=""
		INC_EPICS=""
	fi
	AC_CHECK_LIB([ca], [ca_context_create],
	LIB_EPICS="-lca ${LIB_EPICS}",
	[cat << EOF
**** You don't have epics library installed.
**** Please try to get it from http://www.aps.anl.gov/epics
EOF
	exit 1])

	AC_SUBST(LIB_EPICS)
	AC_SUBST(INC_EPICS)
fi

AH_TEMPLATE([HAVE_CRYPT],[If crypt library is installed])

AC_CHECK_LIB([crypt], [crypt],
[LIB_CRYPT="-lcrypt"
AC_SUBST(LIB_CRYPT)
AC_DEFINE_UNQUOTED([HAVE_CRYPT],1,[If crypt library is installed])],
[cat << EOF
**** You don't have crypt library.
**** All passwords will be stored in plaintext
EOF
LIB_CRYPT="no"
])

AC_MSG_CHECKING(for build date)
DATE=`date +%Y-%m-%d`
if test "z"$DATE = "z" ; then
	AC_MSG_RESULT(none found)
	DATE="unknow"
else
	AC_MSG_RESULT($DATE)
fi

LDADD=""
AC_SUBST(LDADD)
AC_SUBST(LDFLAGS)

AC_SUBST(DATE)

debug=""
AC_SUBST(debug)

AH_TEMPLATE([HAVE_PGSQL],[Whenever PGSQL is installed])
AH_TEMPLATE([HAVE_PGSQL_8], [If PGSQL version is 8 or higher])
AH_TEMPLATE([HAVE_PGSQL_9], [If PGSQL version is 9 or higher])
AH_TEMPLATE([HAVE_ECPG_9], [If ECPG version is 9 or higher])
AH_TEMPLATE([HAVE_PGSQL_SET_VARSIZE], [If PGSQL provides SET_VARSIZE function])

if test "x"$psql = xyes ; then
  AC_POSTGRESQL
  AC_ECPG

  PG_MAJOR=`$PG_CONFIG --version | sed 's/PostgreSQL //' | sed 's/\..*$//'`
  PG_MINOR=`$PG_CONFIG --version | sed 's/PostgreSQL [[0-9]]*\.//' | sed 's/\..*$//'`

  ECPG_MAJOR=`$ECPG --version | sed 's/ecpg (PostgreSQL //' | sed 's/\..*$//'`
  ECPG_MINOR=`$ECPG --version | sed 's/ecpg (PostgreSQL [[0-9]]*\.//' | sed 's/\..*$//'`

  AC_DEFINE_UNQUOTED(HAVE_PGSQL, 1, [Whenever PGSQL is installed])

  if test $PG_MAJOR -gt 7 ; then
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_8, 1, [If PGSQL version is 8 or higher])
  fi
  if test $PG_MAJOR -gt 8 ; then
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_9, 1, [If PGSQL version is 9 or higher])
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_SET_VARSIZE, 1, [If PGSQL provides SET_VARSIZE function])
  fi
  if test $ECPG_MAJOR -gt 8 ; then
    AC_DEFINE_UNQUOTED(HAVE_ECPG_9, 1, [If ECPG version is 9 or higher])
  fi
  if test $PG_MAJOR -eq 8 -a $PG_MINOR -gt 2 ; then
    AC_DEFINE_UNQUOTED(HAVE_PGSQL_SET_VARSIZE, 1, [If PGSQL provides SET_VARSIZE function])
  fi
fi

AM_CONDITIONAL(PGSQL, test x$psql = xyes)

DB2MAN=no
DB2FOP=no

AC_ARG_WITH(docbook,
[  --with-docbook         path DocBook stylesheets (needed for manual page generation)],
docbook="${withval}";
DB2MAN=$docbook/manpages/docbook.xsl;
DB2FOP=$docbook/fo/docbook.xsl,
[docbook=detect])

if test "x$docbook" == 'xdetect'; then
	# Ubuntu docbook localization
	AC_CHECK_FILE([/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl],DB2MAN=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl,[])
	AC_CHECK_FILE([/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/fo/docbook.xsl],DB2FOP=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/fo/docbook.xsl,[])

	# Fedore docbook localization
	AC_CHECK_FILE([/usr/share/sgml/docbook/xsl-stylesheets/manpages/docbook.xsl],DB2MAN=/usr/share/sgml/docbook/xsl-stylesheets/manpages/docbook.xsl,[])
	AC_CHECK_FILE([/usr/share/sgml/docbook/xsl-stylesheets/manpages/fo/docbook.xsl],DB2FOP=/usr/share/sgml/docbook/xsl-stylesheets/manpages/fo/docbook.xsl,[])

	# SUSE docbook localization
	AC_CHECK_FILE([/usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/docbook.xsl],DB2MAN=/usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/docbook.xsl,[])
	AC_CHECK_FILE([/usr/share/xml/docbook/stylesheet/nwalsh/current/fo/docbook.xsl],DB2FOP=/usr/share/xml/docbook/stylesheet/nwalsh/current/manpages/fo/docbook.xsl,[])
else
	if test "x$docbook" != "xno"; then
		if test x$DB2MAN != "xno"; then
			AC_CHECK_FILE([$DB2MAN],[],exit)
		fi

		if test x$DB2FOP != "xno"; then
			AC_CHECK_FILE([$DB2FOP],[],exit)
		fi
	fi
fi

AC_SUBST(DB2MAN)
AC_SUBST(DB2FOP)

AM_CONDITIONAL(HAVE_DB2MAN, test x$DB2MAN != xno)
AM_CONDITIONAL(HAVE_DB2FOP, test x$DB2FOP != xno)

AC_CONFIG_SRCDIR(src)
AM_CONFIG_HEADER(config.h)
AX_PREFIX_CONFIG_H(include/rts2-config.h)

AM_CONDITIONAL(NOT_GETADDRINFO, test x${ac_cv_func_getaddrinfo} = xno)
AM_CONDITIONAL(NOT_GETOPT_LONG, test x${ac_cv_func_getopt_long} = xno)
AM_CONDITIONAL(SUNOS, test x${target_os} = xsolaris2.8)
AM_CONDITIONAL(CYGWIN, test x${target_os} = xcygwin)
AM_CONDITIONAL(SUNCYG, test x${target_os} = xcygwin -o x${target_os} = xsolaris2.8)

AC_OUTPUT([Makefile
	   rts2.initd
	   man/Makefile
	   man/rts2.ent
	   include/Makefile
	   include/connection/Makefile
	   include/rts2db/Makefile
	   include/rts2fits/Makefile
	   include/rts2script/Makefile
	   include/rts2scheduler/Makefile
	   include/vermes/Makefile
	   conf/Makefile
	   conf/event.d/Makefile
	   contrib/Makefile
	   doc/Makefile
	   doc/doxyfile
	   lib/Makefile
	   lib/xmlrpc++/Makefile
	   lib/rts2/Makefile
	   lib/rts2/model/Makefile
	   lib/rts2/vermes/Makefile
	   lib/rts2fits/Makefile
	   lib/rts2db/Makefile
	   lib/rts2script/Makefile
	   lib/rts2scheduler/Makefile
	   lib/libindi/Makefile 
	   python/Makefile
	   python/lib/Makefile
	   scripts/Makefile
	   src/Makefile
	   src/bckimages/Makefile
	   src/camd/Makefile
	   src/camd/urvc2/Makefile
	   src/camd/apogee/Makefile
	   src/camd/edtsao/Makefile
	   src/dome/Makefile
	   src/dome/zelio/Makefile
	   src/filterd/Makefile
	   src/focusc/Makefile
	   src/focusd/Makefile
	   src/grb/Makefile
	   src/mirror/Makefile
	   src/monitor/Makefile
	   src/plan/Makefile
	   src/sql/Makefile
	   src/sql/auger/Makefile
	   src/sql/create/Makefile
	   src/sql/data/Makefile
	   src/sql/drop/Makefile
	   src/sql/update/Makefile
	   src/sql/telma/Makefile
	   src/sql/phot/Makefile
	   src/sql/bb/Makefile
	   src/pgsql/Makefile
	   src/phot/Makefile
	   src/centrald/Makefile
	   src/teld/Makefile
	   src/db/Makefile
	   src/sensord/Makefile
	   src/rotad/Makefile
	   src/logger/Makefile
	   src/xmlrpc/Makefile
	   src/scheduler/Makefile
	   src/bb/Makefile]
)

echo "
*********************************************************
 RTS2 build configuration for ${target_os} on ${build_os}

  focusing      ${focusing}
  xfocusc       ${xfocusc}
  psql          ${psql}
  libnova       ${libnova}
  cfitsio       ${cfitsio}
  libwcs        ${libwcs}
  pgsql		${psql}
  gpib          ${GPIBLIB}
  comedilib     ${COMEDI}
  epics         ${epics}
  libxml2       ${XMLVERS}
  graphicsmagic ${GRAPHICSMAGICK_VERS}
  CERN ROOT     ${ROOT_VERS}
  libarchive    ${libarchive}
  crypt         ${LIB_CRYPT}
  libgjson	${JSONGLIB_CFLAGS} ${JSONGLIB_LIBS}

CCDs:
  alta          ${ALTA}
  andor         ${ANDOR}
  arc           ${ARC}
  arc-1.7       ${ARC_1_7}
  edtpdv        ${EDTPDV}
  fli           ${FLI}
  miccd         ${MICCD}
  sbig          ${SBIG}

MOUNTs:
  paralib       ${PARALIB}
  mdmtcs        ${MDMTCS}

DOMEs:
  PCI8255       ${PCI8255}

SENSORs:
  A3200         ${A3200}
  BOLTWOOD      ${BOLTWOOD}
  BIGNG         ${BIGNG}
  K8055         ${K8055}

  lock prefix   ${LOCK_PREFIX}
  centrald port	${CENTRALD_PORT}

DocBook stylesheets:
  DB2MAN        ${DB2MAN}
  DB2FOP        ${DB2FOP}

***************************************

Now type 'make' followed by 'make install'
to install rts2 package.

Please note, that you should do 'make clean'
if you change any configuration option.

"
