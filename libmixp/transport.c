/* Copyright ©2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "mixp_local.h"

#include <9p-mixp/transport.h>
#include <9p-mixp/err.h>
#include <9p-mixp/convert.h>

static int
mread(int fd, MIXP_MESSAGE *msg, size_t count) {
	int r, n;

	n = msg->end - msg->pos;
	if(n <= 0) {
		mixp_werrstr("buffer full");
		return -1;
	}
	if(n > count)
		n = count;

	r = mixp_thread->read(fd, msg->pos, n);
	if(r > 0)
		msg->pos += r;
	return r;
}

static int
readn(int fd, MIXP_MESSAGE *msg, size_t count) {
	size_t  num;
	int r;

	num = count;
	while(num > 0) {
		r = mread(fd, msg, num);
		if(r == -1 && errno == EINTR)
			continue;
		if(r == 0) {
			mixp_werrstr("broken pipe");
			return count - num;
		}
		num -= r;
	}
	return count - num;
}

size_t mixp_sendmsg(int fd, MIXP_MESSAGE *msg) {
	int r;

	int _size = msg->end - msg->data;

#ifdef _DEBUG
	printf("sending: size=%d\n", _size);
	unsigned char* buf = (unsigned char*)msg->data;
	int x=0;
	for (x=0; x<_size; x++)
	    printf("%02X ", buf[x]);	
	printf("\n");
#endif

	msg->pos = msg->data;
	while(msg->pos < msg->end) {
		r = mixp_thread->write(fd, msg->pos, msg->end - msg->pos);
		if(r < 1) {
			if(errno == EINTR)
				continue;
			mixp_werrstr("broken pipe");
			return 0;
		}
		msg->pos += r;
	}

	return msg->pos - msg->data;
}

size_t
mixp_recvmsg(int fd, MIXP_MESSAGE *msg) {
	enum { SSize = 4 };
	unsigned int msize, size;

	msg->mode = MsgUnpack;
	msg->pos = msg->data;
	msg->end = msg->data + msg->size;
	if(readn(fd, msg, SSize) != SSize)
		return 0;

	msg->pos = msg->data;
	mixp_pu32(msg, &msize);

	size = msize - SSize;
	if(msg->pos + size >= msg->end) {
		mixp_werrstr("message too large");
		return 0;
	}
	if(readn(fd, msg, size) != size) {
		mixp_werrstr("message incomplete");
		return 0;
	}

	msg->end = msg->pos;

#ifdef _DEBUG	
	printf("received: size=%d msize=%d\n", size, msize);
	unsigned char* buf = (unsigned char*)msg->data;
	int x=0;
	for (x=0; x<size; x++)
	    printf("%02X ", buf[x]);	
	printf("\n");
#endif

	return msize;
}
