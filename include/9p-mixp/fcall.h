
#ifndef __MIXP_FCALL_H
#define __MIXP_FCALL_H

#include <9p-mixp/bits.h>

struct MIXP_FCALL {
	unsigned char type;
	unsigned short tag;
	unsigned int fid;
	short _is_static;
	union {
		struct {
			unsigned int msize;
			char	*version;
		} Tversion;
		struct {
			unsigned int msize;
			char	*version;
		} Rversion;
		struct { 
			unsigned short oldtag;
		} Tflush;
		
		struct { /* Rerror */
			char *ename;
		} Rerror;
		struct { /* Ropen, Rcreate */
			MIXP_QID qid; /* +Rattach */
			size_t iounit;
		} Ropen;
		struct { /* Ropen, Rcreate */
			MIXP_QID qid; /* +Rattach */
			size_t iounit;
		} Rattach;
		struct { /* Ropen, Rcreate */
			MIXP_QID qid; /* +Rattach */
			size_t iounit;
		} Rcreate;
		struct { /* Rauth */
			MIXP_QID aqid;
		} Rauth;
		struct { /* Tauth, Tattach */
			unsigned int	afid;
			char		*uname;
			char		*aname;
		} Tauth;
		struct { /* Tauth, Tattach */
			unsigned int	afid;
			char		*uname;
			char		*aname;
		} Tattach;
		struct { /* Tcreate */
			unsigned int	perm;
			char		*name;
			unsigned char	mode; /* +Topen */
		} Tcreate;
		struct { /* Tcreate */
			unsigned int	perm;
			char		*name;
			unsigned char	mode; /* +Topen */
		} Topen;
		struct { /* Twalk */
			unsigned int	newfid;
			unsigned short	nwname;
			char	*wname[MIXP_MAX_WELEM];
		} Twalk;
		struct { /* Rwalk */
			unsigned short	nwqid;
			MIXP_QID	wqid[MIXP_MAX_WELEM];
		} Rwalk;

		struct { /* Twrite */
			uint64_t	offset; /* +Tread */
			/* +Rread */
			size_t		count; /* +Tread */
			char* 		data;
		} Twrite;

		struct { /* Twrite */
			uint64_t	offset; /* +Tread */
			/* +Rread */
			size_t		count; /* +Tread */
			char*		data;
		} Rwrite;

		struct { /* Twrite */
			uint64_t	offset; /* +Tread */
			/* +Rread */
			size_t		count; /* +Tread */
			char*		data;
		} Tread;

		/* Read reply */
		struct { 
			uint64_t	offset;
			size_t		count;
			char*		data;
		} Rread;
		
		struct { /* Twstat, Rstat */
			unsigned short	nstat;
			char	*stat;
		} Twstat;

		struct { /* Twstat, Rstat */
			unsigned short	nstat;
			char	*stat;
		} Rstat;
	};
};

#endif
