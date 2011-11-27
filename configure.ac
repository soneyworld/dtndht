AC_PREREQ([2.68])
AC_INIT([BtDHTLib], [0.1], [lorentze@ibr.cs.tu-bs.de])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcrypt':
AC_CHECK_LIB([crypt], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_RESTRICT
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa memmove memset select socket strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
