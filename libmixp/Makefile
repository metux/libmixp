
SONAME=mixp
THREADLIB=pthread

include ../build.mk

OBJNAMES=client convert intmap message request socket transport util error rpc thread addr thread_$(THREADLIB) server

NOPIC_OBJ = $(addsuffix .nopic.o,$(OBJNAMES))
PIC_OBJ   = $(addsuffix .pic.o,$(OBJNAMES))

CFLAGS+=-I../include

all:	lib$(SONAME).a lib$(SONAME).so

lib$(SONAME).a::	$(NOPIC_OBJ)

lib$(SONAME).so::	$(PIC_OBJ)

install:	lib$(SONAME).a	lib$(SONAME).so
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp lib$(SONAME).a lib$(SONAME).so $(DESTDIR)$(LIBDIR)
	chmod ugo+x $(DESTDIR)$(LIBDIR)/lib$(SONAME).so

clean:
	rm -f *.o *.a *.so
