/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * Copyright ©2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "mixp_local.h"

#include <9p-mixp/bits.h>
#include <9p-mixp/srv_addr.h>
#include <9p-mixp/err.h>

/* Note: These functions modify the strings that they are passed.
 *   The lookup function duplicates the original string, so it is
 *   not modified.
 */

typedef struct sockaddr sockaddr;
typedef struct sockaddr_un sockaddr_un;
typedef struct sockaddr_in sockaddr_in;

static int
get_port(const char *addr) {
	char *s, *end;
	int port;

	s = strchr(addr, '!');
	if(s == NULL) {
		mixp_werrstr("no port provided");
		return -1;
	}

	*s++ = '\0';
	port = strtol(s, &end, 10);
	if(*s == '\0' && *end != '\0') {
		mixp_werrstr("invalid port number");
		return -1;
	}
	return port;
}

static int
sock_unix(const char *address, sockaddr_un *sa, socklen_t *salen) {
	int fd;

	memset(sa, 0, sizeof(*sa));

	sa->sun_family = AF_UNIX;
	strncpy(sa->sun_path, address, sizeof(sa->sun_path));
	*salen = SUN_LEN(sa);

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd < 0)
		return -1;
	return fd;
}

static int
sock_tcp_2(const char *host, int port, sockaddr_in *sa) {
	struct hostent *he;
	int fd;

	if(port < 0)
		return -1;

	signal(SIGPIPE, SIG_IGN);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
		return -1;

	memset(sa, 0, sizeof(*sa));
	sa->sin_family = AF_INET;
	sa->sin_port = htons(port);

	if(strcmp(host, "*") == 0)
		sa->sin_addr.s_addr = htonl(INADDR_ANY);
	else if((he = gethostbyname(host)))
		memcpy(&sa->sin_addr, he->h_addr, he->h_length);
	else
		return -1;
	return fd;
}

static int
dial_unix(const char *address) {
	sockaddr_un sa;
	socklen_t salen;
	int fd;

	fd = sock_unix(address, &sa, &salen);
	if(fd == -1)
		return fd;

	if(connect(fd, (sockaddr*) &sa, salen)) {
		close(fd);
		return -1;
	}
	return fd;
}

static int
announce_unix(const char *file) {
	const int yes = 1;
	sockaddr_un sa;
	socklen_t salen;
	int fd;

	signal(SIGPIPE, SIG_IGN);

	fd = sock_unix(file, &sa, &salen);
	if(fd == -1)
		return fd;

	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&yes, sizeof(yes)) < 0)
		goto fail;

	unlink(file);
	if(bind(fd, (sockaddr*)&sa, salen) < 0)
		goto fail;

	chmod(file, S_IRWXU);
	if(listen(fd, MIXP_MAX_CACHE) < 0)
		goto fail;

	return fd;

fail:
	close(fd);
	return -1;
}

static int dial_tcp_2(const char *host, int port) 
{
	sockaddr_in sa;
	int fd;

	fd = sock_tcp_2(host, port, &sa);
	if(fd == -1)
		return fd;

	if(connect(fd, (sockaddr*)&sa, sizeof(sa))) {
		close(fd);
		return -1;
	}

	return fd;
}

static int dial_tcp(const char *host) 
{
	int port = get_port(host);
	return dial_tcp_2(host,port);
}

static int
announce_tcp(const char *host) {
	sockaddr_in sa;
	int fd;
	int port;

	port = get_port(host);

	fd = sock_tcp_2(host, port, &sa);
	if(fd == -1)
		return fd;

	if(bind(fd, (sockaddr*)&sa, sizeof(sa)) < 0)
		goto fail;

	if(listen(fd, MIXP_MAX_CACHE) < 0)
		goto fail;

	return fd;

fail:
	close(fd);
	return -1;
}

typedef struct addrtab addrtab;
struct addrtab {
	char *type;
	int (*fn)(const char*);
} dtab[] = {
	{"tcp", dial_tcp},
	{"unix", dial_unix},
	{0, 0}
}, atab[] = {
	{"tcp", announce_tcp},
	{"unix", announce_unix},
	{0, 0}
};

static int
lookup(const char *address, addrtab *tab) {
	char *addr, *type;
	int ret;

	ret = -1;
	type = strdup(address);

	addr = strchr(type, '!');
	if(addr == NULL)
		mixp_werrstr("no address type defined");
	else {
		*addr++ = '\0';
		for(; tab->type; tab++)
			if(strcmp(tab->type, type) == 0) break;
		if(tab->type == NULL)
			mixp_werrstr("unsupported address type");
		else
			ret = tab->fn(addr);
	}

	free(type);
	return ret;
}

int mixp_dial_addr(MIXP_SERVER_ADDRESS *addr)
{
	switch (addr->proto)
	{
		case P9_PROTO_TCP:
			return dial_tcp_2(addr->hostname, addr->port);
		case P9_PROTO_UNIX:
			return dial_unix(addr->path);
		default:
			return dial_tcp_2(addr->hostname, addr->port);
	}
}

int mixp_dial(const char *address) 
{
	int ret;
	MIXP_SERVER_ADDRESS* addr = mixp_srv_addr_parse(address);
	if (addr==NULL)
		return -1;
	ret = mixp_dial_addr(addr);
	mixp_srv_addr_free(addr);
	return ret;
}

int mixp_announce(const char *address) 
{
	return lookup(address, atab);
}
