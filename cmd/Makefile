#
# Author(s): Enrico Weigelt <weigelt@metux.de>
#

include ../build.mk

CFLAGS += -DVERSION=\"${VERSION}\" -I../include
LIBMIXP=../libmixp/libmixp.a -lpthread

all:		ixpc

ixp_client:	ixp_client.o util.o
	$(CC) -o $@ $^ $(LIBMIXP)

ixpc:		ixpc.nopic.o
	$(CC) -o $@ $^ $(LIBMIXP)

clean:
	rm -f *.o ixp_client ixpc
