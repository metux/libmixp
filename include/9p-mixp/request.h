
#ifndef __MIXP_REQUEST_H
#define __MIXP_REQUEST_H

struct MIXP_REQUEST {
	MIXP_SRV_OPS	*srv;
	MIXP_FID	*fid;
	MIXP_FID	*newfid;
	MIXP_REQUEST	*oldreq;
	MIXP_FCALL	*ifcall;
	MIXP_FCALL	*ofcall;
	void		*aux;

	/* Implementation details */
	MIXP_9CONN	*conn;
};

#endif
