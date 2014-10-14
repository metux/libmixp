
#ifndef __MIXP_BITS_H
#define __MIXP_BITS_H

#define MIXP_VERSION	"9P2000"
#define MIXP_NOTAG	((unsigned short)~0)	/* Dummy tag */
#define MIXP_NOFID	(~0U)

enum {
	MIXP_MAX_VERSION = 32,
	MIXP_MAX_MSG = 8192,
	MIXP_MAX_ERROR = 128,
	MIXP_MAX_CACHE = 32,
	MIXP_MAX_FLEN = 128,
	MIXP_MAX_ULEN = 32,
	MIXP_MAX_WELEM = 16,
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

#endif
