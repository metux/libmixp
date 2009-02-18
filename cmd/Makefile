#
# Author(s): Enrico Weigelt <weigelt@metux.de>
#

include ../build.mk

CFLAGS += -DVERSION=\"${VERSION}\" -I../include
LIBMIXP=../libmixp/libmixp.a -lpthread

all:		ixpc
#all:		ixp_client	ixpc

#ixpc:		ixpc.o
#	$(CC) -L../libmixp -lixp -o $@ $^ -lpthread
#
#ixp_client:	ixp_client.o util.o
#	$(CC) -L../libmixp -lixp -o $@ $^ -lpthread

#ixp_client::	../libmixp/libmixp.a

ixp_client:	ixp_client.o util.o
	$(CC) -o $@ $^ $(LIBMIXP)

ixpc:		ixpc.o
	$(CC) -o $@ $^ $(LIBMIXP)
	
clean:
	rm -f *.o ixp_client ixpc
