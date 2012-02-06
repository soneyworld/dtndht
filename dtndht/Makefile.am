ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

include_h_sources = dtndht.h dht.h
h_sources = $(include_h_sources) blacklist.h  bloom.h rating.h
c_sources = dht.c implFunction.c dtndht.c blacklist.c bloom.c rating.c

AM_CFLAGS = $(OPENSSL_CFLAGS)
AM_LDFLAGS = $(OPENSSL_LIBS)

if DOUBLELOOKUP
h_sources += doublelookup.h
c_sources += doublelookup.c
endif

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(include_h_sources)

lib_LTLIBRARIES = libdtndht.la
libdtndht_la_SOURCES = $(h_sources) $(c_sources)
libdtndht_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)
