### Detect PostgreSQL

AC_DEFUN(AC_POSTGRESQL,
[
if test -z "$PG_CONFIG" ; then
    AC_PATH_PROG( PG_CONFIG, pg_config, no )
fi

if test "$PG_CONFIG" = "no" ; then
    echo "*** You must install PostgreSQL and pg_config. If pg_config is not"
    echo "*** in PATH. Try to setup PG_CONFIG variable"
fi

# FIXME: Replace `main' with a function in `-lpq':
AC_CHECK_LIB([pq], [main])

PG_CONFIG_LIBS=`$PG_CONFIG --libdir`
PG_CONFIG_INCLUDES=`$PG_CONFIG --includedir`
LIBPG_LIBS="-L$PG_CONFIG_LIBS"
LIBPG_CFLAGS="-I$PG_CONFIG_INCLUDES"
AC_SUBST(LIBPG_LIBS)
AC_SUBST(LIBPG_CFLAGS)

PG_CONFIG_SERVER_LIBS=`$PG_CONFIG --pkglibdir`
PG_CONFIG_SERVER_INCLUDES=`$PG_CONFIG --includedir-server`
LIBPG_SERVER_LIBS="-L$PG_CONFIG_SERVER_LIBS"
LIBPG_SERVER_CFLAGS="-I$PG_CONFIG_SERVER_INCLUDES"
AC_SUBST(PG_CONFIG_SERVER_LIBS)
AC_SUBST(LIBPG_SERVER_LIBS)
AC_SUBST(LIBPG_SERVER_CFLAGS)
])

AC_DEFUN(AC_ECPG, 
[
if test -z "$ECPG" ; then
    AC_PATH_PROG( ECPG, ecpg, no )
fi

if test "$ECPG" = "no" -o ! -x $ECPG ; then
    echo "*** You must install PostgreSQL and ecpg. If ecpg is not"
    echo "*** in PATH, try to setup ECPG variable. ecpg is ussually in"
    echo "*** Postgresql-devel package."
fi

# FIXME: Replace `main' with a function in `-lecpg':
AC_CHECK_LIB([ecpg], [main])

AC_SUBST(ECPG)
])
