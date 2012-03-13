dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(dtndht-tools, 0.0.1, [lorentze@ibr.cs.tu-bs.de])


AM_INIT_AUTOMAKE()
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/lookup.c])
AC_PROG_CC

AC_CHECK_FUNCS([select])

AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([sys/socket.h])

PKG_CHECK_MODULES([DTNDHT], dtndht >= 0.2.1)

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT

