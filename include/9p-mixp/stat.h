/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * Copyright ©2006-2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */

#ifndef __MIXP_STAT_H
#define __MIXP_STAT_H

// FIXME
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <inttypes.h>
#include <9p-mixp/qid.h>

/* stat structure */
typedef struct MIXP_STAT {
	unsigned short type;
	unsigned int dev;
	MIXP_QID qid;
	unsigned int mode;
	unsigned int atime;
	unsigned int mtime;
	uint64_t length;
	char *name;
	char *uid;
	char *gid;
	char *muid;
} MIXP_STAT;

void mixp_stat_free(MIXP_STAT* stat);

#endif
