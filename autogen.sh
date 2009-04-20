#!/bin/sh

#AC_VERSION=-1.7
AC_VERSION=""

aclocal$AC_VERSION && libtoolize && autoheader && automake$AC_VERSION --add-missing && autoconf
