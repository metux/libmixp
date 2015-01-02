#
# Author(s): Enrico Weigelt <weigelt@metux.de>
#

include ./build.mk

all:	lib	libmixp.pc	client

lib:
	make -C libmixp

client:
	make -C cmd

install-lib:
	make -C libmixp install

install-pkgconfig:	libmixp.pc
	mkdir -p $(DESTDIR)$(PKGCONFIGDIR)
	cp libmixp.pc $(DESTDIR)$(PKGCONFIGDIR)

install-includes:	include/9p-mixp/*.h
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/9p-mixp
	for i in include/9p-mixp/*.h ; do cp $$i $(DESTDIR)$(INCLUDEDIR)/9p-mixp ; done

install:	install-pkgconfig install-includes install-lib

clean:
	rm -f *.o *.a *.so *.pc
	make -C libmixp clean
	make -C cmd clean
