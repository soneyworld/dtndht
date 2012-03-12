ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

include_h_sources = dtndht.h
h_sources = $(include_h_sources) dht.h  bootstrapping.h utils.h list.h
c_sources = dht.c dtndht.c bootstrapping.c utils.c list.c

AM_CFLAGS = $(OPENSSL_CFLAGS)
AM_LDFLAGS = $(OPENSSL_LIBS)

if BLACKLIST_SUPPORT
h_sources += blacklist.h
c_sources += blacklist.c
endif

if RATING_SUPPORT
h_sources += bloom.h rating.h
c_sources += bloom.c rating.c
endif

if EVALUATION
h_sources += evaluation.h
c_sources += evaluation.c
endif

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(include_h_sources)

lib_LTLIBRARIES = libdtndht.la
libdtndht_la_SOURCES = $(h_sources) $(c_sources)
libdtndht_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)
