ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
INCLUDES = -I@top_srcdir@
AM_CFLAGS = @OPENSSL_CFLAGS@
AM_LDFLAGS = @OPENSSL_LIBS@

noinst_PROGRAMS = blacklisttest dtndhttest

blacklisttest_SOURCES = blacklist_test.c
blacklisttest_LDADD = ../dtndht/blacklist.o

dtndhttest_SOURCES = dtndht_test.c
dtndhttest_LDADD = ../dtndht/dht.o ../dtndht/dtndht.o ../dtndht/implFunction.o ../dtndht/blacklist.o