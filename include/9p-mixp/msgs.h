/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * Copyright ©2006-2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */

#ifndef __MIXP_MSGS_H
#define __MIXP_MSGS_H

/* 9P message types */
enum {	P9_TVersion = 100,
	P9_RVersion,
	P9_TAuth = 102,
	P9_RAuth,
	P9_TAttach = 104,
	P9_RAttach,
	P9_TError = 106, /* illegal */
	P9_RError,
	P9_TFlush = 108,
	P9_RFlush,
	P9_TWalk = 110,
	P9_RWalk,
	P9_TOpen = 112,
	P9_ROpen,
	P9_TCreate = 114,
	P9_RCreate,
	P9_TRead = 116,
	P9_RRead,
	P9_TWrite = 118,
	P9_RWrite,
	P9_TClunk = 120,
	P9_RClunk,
	P9_TRemove = 122,
	P9_RRemove,
	P9_TStat = 124,
	P9_RStat,
	P9_TWStat = 126,
	P9_RWStat,
};

typedef struct MIXP_MESSAGE MIXP_MESSAGE;

enum { MsgPack, MsgUnpack, };
struct MIXP_MESSAGE {
	char *data;
	char *pos;
	char *end;
	unsigned int size;
	unsigned int mode;
};

#endif
