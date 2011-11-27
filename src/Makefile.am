ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

h_sources = dht.h
c_sources = dht.c

if OPENSSL
c_sources += implFunction.c 
endif

library_includedir=$(includedir)/$(GENERIC_LIBRARY_NAME)-$(GENERIC_API_VERSION)/$(GENERIC_LIBRARY_NAME)
library_include_HEADERS = $(h_sources)

lib_LTLIBRARIES = libbtdht.la
libbtdht_la_SOURCES = $(h_sources) $(c_sources)
libbtdht_la_LDFLAGS= -version-info $(GENERIC_LIBRARY_VERSION) -release $(GENERIC_RELEASE)
