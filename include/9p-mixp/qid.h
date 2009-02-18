
#ifndef __MIXP_QID_H
#define __MIXP_QID_H

#include <inttypes.h>

typedef struct {
	unsigned char type;
	unsigned int version;
	uint64_t path;
	/* internal use only */
	unsigned char dir_type;
} MIXP_QID;

#endif
