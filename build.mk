#
# Author(s): Enrico Weigelt, metux IT services <weigelt@metux.de>
#

VERSION=3.6.2
PREFIX?=/usr
LIBDIR?=$(PREFIX)/lib
INCLUDEDIR?=$(PREFIX)/include
PKGCONFIGDIR?=$(LIBDIR)/pkgconfig
PKG_CONFIG?=pkg-config
AR?=ar
RANLIB?=ranlib
CFLAGS+=-Wall

%.pc:		%.pc.in
	cat $< | \
	    sed -e 's~@VERSION@~$(VERSION)~'       | \
	    sed -e 's~@PREFIX@~$(PREFIX)~'         | \
	    sed -e 's~@LIBDIR@~$(LIBDIR)~'         | \
	    sed -e 's~@INCLUDEDIR@~$(INCLUDEDIR)~' > $@

%.a:
	$(AR) cr $@ $^ && $(RANLIB) $@

%.so:
	$(LD) -o $@ -shared $^ -soname $(SONAME) -lc -no-undefined $(LIBS)

%.nopic.o:	%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.pic.o:	%.c
	$(CC) -fpic -o $@ -c $< $(CFLAGS)

