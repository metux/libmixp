
SONAME=mixp
THREADLIB=pthread

include ../build.mk

OBJNAMES=client convert intmap message request socket transport util error rpc thread addr thread_$(THREADLIB) server

NOPIC_OBJ = $(addsuffix .nopic.o,$(OBJNAMES))
PIC_OBJ   = $(addsuffix .pic.o,$(OBJNAMES))

PTHREAD_LIBS=`$(PKG_CONFIG) --libs libc-pthread`
PTHREAD_CFLAGS=`$(PKG_CONFIG) --cflags libc-pthread`

CFLAGS+=-I../include $(PTHREAD_CFLAGS)
LIBS+=$(PTHREAD_LIBS) -L../libmixp -lmixp -static

all:	addr_parse

addr_parse:	addr_parse.c	../libmixp/libixp.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ $< $(LIBS)

xall:	lib$(SONAME).a lib$(SONAME).so

lib$(SONAME).a::	$(NOPIC_OBJ)

lib$(SONAME).so::	$(PIC_OBJ)

install:	lib$(SONAME).a	lib$(SONAME).so
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp lib$(SONAME).a lib$(SONAME).so $(DESTDIR)$(LIBDIR)
	chmod ugo+x $(DESTDIR)$(LIBDIR)/lib$(SONAME).so

clean:
	rm -f *.o *.a *.so addr_parse
