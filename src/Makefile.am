ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

include_h_sources = dtndht.h
h_sources = dht.h $(include_h_sources)
c_sources = dht.c implFunction.c dtndht.c

AM_CFLAGS = $(OPENSSL_CFLAGS)
AM_LDFLAGS = $(OPENSSL_LIBS)

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(include_h_sources)

lib_LTLIBRARIES = libdtndht.la
libdtndht_la_SOURCES = $(h_sources) $(c_sources)
libdtndht_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)
