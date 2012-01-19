dnl --------------------------------
dnl Package name and version number
dnl --------------------------------

# include version information
m4_include([version.m4])

dnl --------------------------------
dnl Initialization macros.
dnl --------------------------------

AC_PREREQ([2.68])
AC_INIT([dtndht], [PKG_FULL_VERSION], [lorentze@ibr.cs.tu-bs.de])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([dtndht/dtndht.h])
AC_CONFIG_HEADERS([config.h])
LT_INIT

dnl --------------------------------
dnl library version number
dnl --------------------------------

#API version
GENERIC_API_VERSION=PKG_MAJOR_VERSION.PKG_MINOR_VERSION

#shared library versioning
GENERIC_LIBRARY_VERSION=PKG_LIB_VERSION
#                       | | |
#                +------+ | +---+
#                |        |     |
#             current:revision:age
#                |        |     |
#                |        |     +- increment if interfaces have been added
#                |        |        set to zero if interfaces have been removed
#                                  or changed
#                |        +- increment if source code has changed
#                |           set to zero if current is incremented
#                +- increment if interfaces have been added, removed or changed

# define the library name
GENERIC_LIBRARY_NAME=$PACKAGE_NAME
GENERIC_VERSION=$PACKAGE_VERSION
GENERIC_RELEASE=PKG_MAJOR_VERSION.PKG_MINOR_VERSION

AC_SUBST(GENERIC_API_VERSION)
AC_SUBST(GENERIC_LIBRARY_VERSION)
AC_SUBST(GENERIC_LIBRARY_NAME)
AC_SUBST(GENERIC_RELEASE)
AC_SUBST(GENERIC_VERSION)


dnl -----------------------------------------------
dnl Checks for programs.
dnl -----------------------------------------------

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

# check for openssl
PKG_CHECK_MODULES(OPENSSL, [openssl],[
	use_openssl="yes"
    AC_SUBST(OPENSSL_CFLAGS)
    AC_SUBST(OPENSSL_LIBS)
    AC_DEFINE(HAVE_OPENSSL, [1], ["openssl library is available"])
    AC_CHECK_HEADERS([openssl/rand.h openssl/sha.h],[], [AC_MSG_ERROR([openssl extension requested, but no header files found. ssl extensions are disabled.])]) 
    ], [
    use_openssl="no"
    AC_MSG_ERROR([openssl extension requested, but no openssl library is found. ssl extensions are disabled.])
])

AM_CONDITIONAL(OPENSSL, test x$use_openssl = xyes)

AC_CONFIG_FILES([Makefile \
	dtndht/Makefile \
	test/Makefile \
	dtndht.pc])
AC_OUTPUT
