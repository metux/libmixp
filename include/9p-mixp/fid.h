
#ifndef __MIXP_FID_H
#define __MIXP_FID_H

#include <9p-mixp/qid.h>

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

#endif
