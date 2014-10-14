
#ifndef __MIXP_CONN_H
#define __MIXP_CONN_H

#include <9p-mixp/types.h>

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

#endif
