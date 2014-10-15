/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * Copyright ©2006-2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */

#ifndef __MIXP_H
#define __MIXP_H

// FIXME !
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <stdio.h>

#include <9p-mixp/types.h>
#include <9p-mixp/threading.h>
#include <9p-mixp/srv_addr.h>

/* client.c */
MIXP_CLIENT*  mixp_mount(const char *address);
MIXP_CLIENT*  mixp_mount_addr(MIXP_SERVER_ADDRESS*addr);
MIXP_CLIENT*  mixp_mountfd(int fd);
void        mixp_unmount(MIXP_CLIENT *c);
MIXP_CFID*    mixp_create(MIXP_CLIENT *c, const char *name, unsigned int perm, unsigned char mode);
MIXP_CFID*    mixp_open(MIXP_CLIENT *c, const char *name, unsigned char mode);
int         mixp_remove(MIXP_CLIENT *c, const char *path);
MIXP_STAT*  mixp_stat(MIXP_CLIENT *c, const char *path);
long        mixp_read(MIXP_CFID *f, void *buf, size_t count);
long        mixp_write(MIXP_CFID *f, const void *buf, long count);
long        mixp_pread(MIXP_CFID *f, void *buf, long count, int64_t offset);
long        mixp_pwrite(MIXP_CFID *f, const void *buf, long count, int64_t offset);
int         mixp_close(MIXP_CFID *f);

/* request.c */
void mixp_respond(MIXP_REQUEST *r, const char *error);
void mixp_serve_conn(MIXP_CONNECTION *c);

/* message.c */
size_t mixp_stat_sizeof(MIXP_STAT *stat);
MIXP_MESSAGE mixp_message(char *data, size_t length, unsigned int mode);

/* server.c */
MIXP_CONNECTION *ixp_listen(MIXP_SERVER *s, int fd, void *aux,
		void (*read)(MIXP_CONNECTION *c),
		void (*close)(MIXP_CONNECTION *c));
void mixp_hangup(MIXP_CONNECTION *c);
int  mixp_server_loop(MIXP_SERVER *s);
void ixp_server_close(MIXP_SERVER *s);
int ixp_serversock_tcp(const char* addr, int port, char** errstr);

/* socket.c */
int mixp_dial(const char *address);
int mixp_dial_addr(MIXP_SERVER_ADDRESS* addr);
int mixp_announce(const char *address);

/* util.c */
void *ixp_erealloc(void *ptr, unsigned int size);
unsigned int ixp_tokenize(char **result, unsigned int reslen, char *str, char delim);

extern int   mixp_dump;
extern FILE* mixp_error_stream;
extern FILE* mixp_debug_stream;

#endif
