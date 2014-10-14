
#ifndef __MIXP_SRV_OPS_H
#define __MIXP_SRV_OPS_H

#include <9p-mixp/types.h>

struct MIXP_SRV_OPS {
	void *aux;
	void (*attach)  (MIXP_REQUEST *r);
	void (*clunk)   (MIXP_REQUEST *r);
	void (*create)  (MIXP_REQUEST *r);
	void (*flush)   (MIXP_REQUEST *r);
	void (*open)    (MIXP_REQUEST *r);
	void (*read)    (MIXP_REQUEST *r);
	void (*remove)  (MIXP_REQUEST *r);
	void (*stat)    (MIXP_REQUEST *r);
	void (*walk)    (MIXP_REQUEST *r);
	void (*write)   (MIXP_REQUEST *r);
	void (*freefid) (MIXP_FID *f);
};

#endif
