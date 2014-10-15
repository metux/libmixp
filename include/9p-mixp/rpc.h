
#ifndef __MIXP_RPC_H
#define __MIXP_RPC_H

struct MIXP_RPC {
	MIXP_CLIENT	*mux;
	MIXP_RPC	*next;
	MIXP_RPC	*prev;
	MIXP_RENDEZ	r;
	unsigned int	tag;
	MIXP_FCALL	*p;
	int		waiting;
	int		async;
};

#endif
