#!/bin/sh

AC_VERSION=1.8

aclocal-$AC_VERSION && libtoolize && automake-$AC_VERSION --add-missing && autoconf
