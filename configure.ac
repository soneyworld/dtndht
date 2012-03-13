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
AC_PROG_CXX
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h stddef.h])

AC_FUNC_MALLOC
AC_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa memmove memset select socket strtol memchr memmem])

AC_ARG_WITH([openssl],AS_HELP_STRING([--with-openssl], [build with openssl support instead of own implementation]),[
# check for openssl
	PKG_CHECK_MODULES(OPENSSL, [openssl],[
		use_openssl="yes"
    	AC_SUBST(OPENSSL_CFLAGS)
    	AC_SUBST(OPENSSL_LIBS)
    	AC_DEFINE(HAVE_OPENSSL, [1], ["openssl library is available"])
    	AC_CHECK_HEADERS([openssl/rand.h openssl/sha.h],[], [AC_MSG_ERROR([openssl extension requested, but no header files found. ssl extensions are disabled.])]) 
    	], [
    	use_openssl="no"
    	AC_MSG_WARN([openssl extension requested, but no openssl library is found. ssl extensions are disabled.])
	])
],[
	use_openssl="no"
])

AM_CONDITIONAL(OPENSSL, test x$use_openssl = xyes)

dnl -----------------------------------------------
dnl optional parameter: blacklist mechanism
dnl -----------------------------------------------

AC_ARG_ENABLE(blacklist,
	AS_HELP_STRING([--disable-blacklist], [build without blacklist support])
)

AS_IF([test "x$enable_blacklist" != "xno"], [
        AC_DEFINE(BLACKLIST_SUPPORT, [1], ["blacklist support enabled"])
])
	
AM_CONDITIONAL(BLACKLIST_SUPPORT, test x$enable_blacklist != xno)

dnl -----------------------------------------------
dnl optional parameter: rating mechanism
dnl -----------------------------------------------

AC_ARG_ENABLE([rating],
    AS_HELP_STRING([--disable-rating], [build without rating feature])
)

AS_IF([test "x$enable_rating" != "xno"], [
	AC_DEFINE(RATING_SUPPORT, [1], ["rating support enabled"])
        AC_CHECK_HEADERS([limits.h])
])
	
AM_CONDITIONAL(RATING_SUPPORT, test x$enable_rating != xno)

dnl -----------------------------------------------
dnl optional parameter: logging for dht evaluation 
dnl -----------------------------------------------

AC_ARG_ENABLE([evaluation],
    AS_HELP_STRING([--enable-evaluation], [build with evaluation logging feature])
)

AS_IF([test "x$enable_evaluation" == "xyes"], [
	AC_DEFINE(EVALUATION, [1], ["evaluation logging enabled"])
])
	
AM_CONDITIONAL(EVALUATION, test x$enable_evaluation == xyes)


AC_CONFIG_FILES([Makefile \
	dtndht/Makefile \
	dtndht.pc])
AC_OUTPUT
