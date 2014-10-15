
#ifndef __MIXP_CLIENT_H
#define __MIXP_CLIENT_H

#include <9p-mixp/rpc.h>
#include <9p-mixp/qid.h>

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

#endif
