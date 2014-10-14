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

#include <9p-mixp/intmap.h>
#include <9p-mixp/threading.h>
#include <9p-mixp/msgs.h>
#include <9p-mixp/srv_addr.h>
#include <9p-mixp/qid.h>
#include <9p-mixp/stat.h>

#define IXP_VERSION	"9P2000"
#define IXP_NOTAG	((unsigned short)~0)	/* Dummy tag */
#define IXP_NOFID	(~0U)

enum {
	IXP_MAX_VERSION = 32,
	IXP_MAX_MSG = 8192,
	IXP_MAX_ERROR = 128,
	IXP_MAX_CACHE = 32,
	IXP_MAX_FLEN = 128,
	IXP_MAX_ULEN = 32,
	IXP_MAX_WELEM = 16,
};

/* from libc.h in p9p */
enum {	P9_OREAD	= 0,	/* open for read */
	P9_OWRITE	= 1,	/* write */
	P9_ORDWR	= 2,	/* read and write */
	P9_OEXEC	= 3,	/* execute, == read but check execute permission */
	P9_OTRUNC	= 16,	/* or'ed in (except for exec), truncate file first */
	P9_OCEXEC	= 32,	/* or'ed in, close on exec */
	P9_ORCLOSE	= 64,	/* or'ed in, remove on close */
	P9_ODIRECT	= 128,	/* or'ed in, direct access */
	P9_ONONBLOCK	= 256,	/* or'ed in, non-blocking call */
	P9_OEXCL	= 0x1000,	/* or'ed in, exclusive use (create only) */
	P9_OLOCK	= 0x2000,	/* or'ed in, lock after opening */
	P9_OAPPEND	= 0x4000	/* or'ed in, append only */
};

/* bits in Qid.type */
enum {	P9_QTDIR	= 0x80,	/* type bit for directories */
	P9_QTAPPEND	= 0x40,	/* type bit for append only files */
	P9_QTEXCL	= 0x20,	/* type bit for exclusive use files */
	P9_QTMOUNT	= 0x10,	/* type bit for mounted channel */
	P9_QTAUTH	= 0x08,	/* type bit for authentication file */
	P9_QTTMP	= 0x04,	/* type bit for non-backed-up file */
	P9_QTSYMLINK	= 0x02,	/* type bit for symbolic link */
	P9_QTFILE	= 0x00	/* type bits for plain file */
};

/* bits in Dir.mode */
enum {
	P9_DMEXEC	= 0x1,		/* mode bit for execute permission */
	P9_DMWRITE	= 0x2,		/* mode bit for write permission */
	P9_DMREAD	= 0x4,		/* mode bit for read permission */
};

/* Larger than int, can't be enum */
#define P9_DMDIR	0x80000000	/* mode bit for directories */
#define P9_DMAPPEND	0x40000000	/* mode bit for append only files */
#define P9_DMEXCL	0x20000000	/* mode bit for exclusive use files */
#define P9_DMMOUNT	0x10000000	/* mode bit for mounted channel */
#define P9_DMAUTH	0x08000000	/* mode bit for authentication file */
#define P9_DMTMP	0x04000000	/* mode bit for non-backed-up file */
#define P9_DMSYMLINK	0x02000000	/* mode bit for symbolic link (Unix, 9P2000.u) */
#define P9_DMDEVICE	0x00800000	/* mode bit for device file (Unix, 9P2000.u) */
#define P9_DMNAMEDPIPE	0x00200000	/* mode bit for named pipe (Unix, 9P2000.u) */
#define P9_DMSOCKET	0x00100000	/* mode bit for socket (Unix, 9P2000.u) */
#define P9_DMSETUID	0x00080000	/* mode bit for setuid (Unix, 9P2000.u) */
#define P9_DMSETGID	0x00040000	/* mode bit for setgid (Unix, 9P2000.u) */

typedef struct MIXP_9CONN MIXP_9CONN;
typedef struct MIXP_REQUEST MIXP_REQUEST;
typedef struct MIXP_SRV_OPS MIXP_SRV_OPS;
typedef struct MIXP_CFID MIXP_CFID;
typedef struct MIXP_CLIENT MIXP_CLIENT;
typedef struct MIXP_CONNECTION MIXP_CONNECTION;
typedef struct MIXP_FCALL MIXP_FCALL;
typedef struct MIXP_FID MIXP_FID;
typedef struct MIXP_RPC MIXP_RPC;
typedef struct MIXP_SERVER MIXP_SERVER;

/* Threading */
enum {
	IXP_ERRMAX = IXP_MAX_ERROR,
};

#include <9p-mixp/mixp_fcall.h>

struct MIXP_CONNECTION {
	MIXP_SERVER	*srv;
	void		*aux;
	int		fd;
	void		(*read)(MIXP_CONNECTION *);
	void		(*close)(MIXP_CONNECTION *);
	char		closed;

	/* Implementation details, do not use */
	MIXP_CONNECTION		*next;
};

struct MIXP_SERVER {
	MIXP_CONNECTION *conn;
	void (*preselect)(MIXP_SERVER*);
	void *aux;
	int running;
	int maxfd;
	fd_set rd;
};

struct MIXP_RPC {
	MIXP_CLIENT *mux;
	MIXP_RPC *next;
	MIXP_RPC *prev;
	MIXP_RENDEZ r;
	unsigned int tag;
	MIXP_FCALL	*p;
	int waiting;
	int async;
};

struct MIXP_CLIENT {
	int	fd;
	unsigned int	msize;
	unsigned int	lastfid;

	/* Implementation details */
	unsigned int nwait;
	unsigned int mwait;
	unsigned int freetag;
	MIXP_CFID	*freefid;
	MIXP_MESSAGE	rmsg;
	MIXP_MESSAGE	wmsg;
	MIXP_MUTEX lk;
	MIXP_MUTEX rlock;
	MIXP_MUTEX wlock;
	MIXP_RENDEZ tagrend;
	MIXP_RPC **wait;
	MIXP_RPC *muxer;
	MIXP_RPC sleep;
	int mintag;
	int maxtag;
};

struct MIXP_CFID {
	unsigned int	fid;
	MIXP_QID	qid;
	unsigned char	mode;
	unsigned int	open;
	size_t		iounit;
	uint64_t	offset;
	MIXP_CLIENT 	*client;
	/* internal use only */
	MIXP_CFID 	*next;
	MIXP_MUTEX 	iolock;
};

struct MIXP_FID {
	char		*uid;
	void		*aux;
	unsigned long	fid;
	MIXP_QID	qid;
	signed char	omode;

	/* Implementation details */
	MIXP_9CONN	*conn;
	MIXP_INTMAP	*map;
};

struct MIXP_REQUEST {
	MIXP_SRV_OPS	*srv;
	MIXP_FID	*fid;
	MIXP_FID	*newfid;
	MIXP_REQUEST	*oldreq;
	MIXP_FCALL	*ifcall;
	MIXP_FCALL	*ofcall;
	void	 *aux;

	/* Implementation details */
	MIXP_9CONN	*conn;
};

struct MIXP_SRV_OPS {
	void *aux;
	void (*attach)(MIXP_REQUEST *r);
	void (*clunk)(MIXP_REQUEST *r);
	void (*create)(MIXP_REQUEST *r);
	void (*flush)(MIXP_REQUEST *r);
	void (*open)(MIXP_REQUEST *r);
	void (*read)(MIXP_REQUEST *r);
	void (*remove)(MIXP_REQUEST *r);
	void (*stat)(MIXP_REQUEST *r);
	void (*walk)(MIXP_REQUEST *r);
	void (*write)(MIXP_REQUEST *r);
	void (*freefid)(MIXP_FID *f);
};

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
void serve_9pcon(MIXP_CONNECTION *c);

/* message.c */
size_t mixp_stat_sizeof(MIXP_STAT *stat);
MIXP_MESSAGE mixp_message(char *data, size_t length, unsigned int mode);
void mixp_fcall_free(MIXP_FCALL *fcall);
size_t ixp_msg2fcall(MIXP_MESSAGE *msg, MIXP_FCALL *fcall);
size_t ixp_fcall2msg(MIXP_MESSAGE *msg, MIXP_FCALL *fcall);

/* server.c */
MIXP_CONNECTION *ixp_listen(MIXP_SERVER *s, int fd, void *aux,
		void (*read)(MIXP_CONNECTION *c),
		void (*close)(MIXP_CONNECTION *c));
void ixp_hangup(MIXP_CONNECTION *c);
char *ixp_serverloop(MIXP_SERVER *s);
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
