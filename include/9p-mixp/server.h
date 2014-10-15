
#ifndef __MIXP_SERVER_H
#define __MIXP_SERVER_H

#include <9p-mixp/types.h>

struct MIXP_SERVER {
	MIXP_CONNECTION *conn;
	void (*preselect)(MIXP_SERVER*);
	void *aux;
	int running;
	int maxfd;
	fd_set rd;
};

#endif
