/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * See LICENSE file for license details.
 */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>

#include <9p-mixp/bits.h>

#include "mixp_local.h"

#include <9p-mixp/conn.h>
#include <9p-mixp/server.h>

int ixp_serversock_tcp(const char* addr, int port, char** errstr)
{
    int fd;
    struct sockaddr_in in_addr;

    signal(SIGPIPE,SIG_IGN);
    if ((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
	*errstr = "cannot open socket";
	return -1;
    }

    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(port);
    in_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(fd,(struct sockaddr*)&in_addr, sizeof(struct sockaddr_in))<0)
    {
	*errstr = "cannot bind socket";
	close(fd);
	return -1;
    }
    
    if (listen(fd, MIXP_MAX_CACHE)<0)
    {
	*errstr = "cannot listen on socket";
    	close(fd);
	return -1;
    }
    
    return fd;
}

MIXP_CONNECTION *
ixp_listen(MIXP_SERVER *s, int fd, void *aux,
		void (*read)(MIXP_CONNECTION *c),
		void (*close)(MIXP_CONNECTION *c)
		) {
	MIXP_CONNECTION *c;

	if (s==NULL)
	{
	    fprintf(mixp_error_stream,"ixp_listen() NULL server struct passed\n");
	    return NULL;
	}

	c = calloc(1,sizeof(MIXP_CONNECTION));
	c->fd = fd;
	c->aux = aux;
	c->srv = s;
	c->read = read;
	c->close = close;
	c->next = s->conn;
	s->conn = c;
	return c;
}

void
mixp_hangup(MIXP_CONNECTION *c) {
	MIXP_SERVER *s;
	MIXP_CONNECTION **tc;

	s = c->srv;
	for(tc=&s->conn; *tc; tc=&(*tc)->next)
		if(*tc == c) break;
	assert(*tc == c);

	*tc = c->next;
	c->closed = 1;
	if(c->close)
		c->close(c);
	else
		shutdown(c->fd, SHUT_RDWR);

	close(c->fd);
	free(c);
}

static void
prepare_select(MIXP_SERVER *s) {
	MIXP_CONNECTION *c;

	FD_ZERO(&s->rd);
	for(c = s->conn; c; c = c->next)
		if(c->read) {
			if(s->maxfd < c->fd)
				s->maxfd = c->fd;
			FD_SET(c->fd, &s->rd);
		}
}

static void
handle_conns(MIXP_SERVER *s) {
	MIXP_CONNECTION *c, *n;
	for(c = s->conn; c; c = n) {
		n = c->next;
		if(FD_ISSET(c->fd, &s->rd))
			c->read(c);
	}
}

int
mixp_server_loop(MIXP_SERVER *s) {
	int r;

	s->running = 1;
	while(s->running) {
		if(s->preselect)
			s->preselect(s);
		prepare_select(s);
		r = select(s->maxfd + 1, &s->rd, 0, 0, 0);
		if(r < 0) {
			if(errno == EINTR)
				continue;
			return -errno;
		}
		handle_conns(s);
	}
	return 0;
}

void
ixp_server_close(MIXP_SERVER *s) {
	MIXP_CONNECTION *c, *next;
	for(c = s->conn; c; c = next) {
		next = c->next;
		mixp_hangup(c);
	}
}
