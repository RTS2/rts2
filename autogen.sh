#!/bin/sh

rm -f aclocal.m4

#AC_VERSION=-1.7
AC_VERSION=""

aclocal$AC_VERSION && libtoolize && autoheader && automake$AC_VERSION --add-missing && autoconf
