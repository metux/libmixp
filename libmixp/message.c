/* Copyright ©2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mixp_local.h"

#include <9p-mixp/convert.h>
#include <9p-mixp/fcall.h>
#include <9p-mixp/msgs.h>
#include <9p-mixp/stat.h>

#include "util.h"

enum {
	SByte = 1,
	SWord = 2,
	SDWord = 4,
	SQWord = 8,
};

#define SString(s) (SWord + strlen(s))
enum {
	SQid = SByte + SDWord + SQWord,
};

MIXP_MESSAGE
mixp_message(char *data, size_t length, unsigned int mode) {
	MIXP_MESSAGE m;

	m.data = data;
	m.pos = data;
	m.end = data + length;
	m.size = length;
	m.mode = mode;
	return m;
}

void
mixp_fcall_free(MIXP_FCALL *fcall)
{
	if (fcall == NULL)
	{
#ifdef DEBUG
		fprintf(mixp_debug_stream,"mixp_fcall_free() got an NULL pointer\n");
#endif
		return;
	}

	switch(fcall->type)
	{
		case P9_RStat:
			MIXP_FREE(fcall->Rstat.stat);
		break;
		case P9_RRead:
			MIXP_FREE(fcall->Rread.data);
		break;
		case P9_RVersion:
			MIXP_FREE(fcall->Rversion.version);
		break;
		case P9_RError:
			MIXP_FREE(fcall->Rerror.ename);
		break;
	}
#ifdef _DEBUG
	fprintf(mixp_debug_stream,"WARN: incomplete mixp_fcall_free()\n");
#endif
//    MIXP_FREE(fcall);
}

size_t
mixp_stat_sizeof(MIXP_STAT * stat) {
	return SWord /* size */
		+ SWord /* type */
		+ SDWord /* dev */
		+ SQid /* qid */
		+ 3 * SDWord /* mode, atime, mtime */
		+ SQWord /* length */
		+ SString(stat->name)
		+ SString(stat->uid)
		+ SString(stat->gid)
		+ SString(stat->muid);
}

const char* fcall_type2str(int type)
{
	switch (type)
	{
		case P9_TVersion:	return "[TVersion]";
		case P9_RVersion:	return "[RVersion]";
		case P9_TAuth:		return "[TAuth]   ";
		case P9_RAuth:		return "[RAuth]   ";
		case P9_TAttach:	return "[TAttach] ";
		case P9_RAttach:	return "[RAttach] ";
		case P9_TOpen:		return "[TOpen]   ";
		case P9_ROpen:		return "[ROpen]   ";
		case P9_TRead:		return "[TRead]   ";
		case P9_RRead:		return "[RRead]   ";
		case P9_TWrite:		return "[TWrite]  ";
		case P9_RWrite:		return "[RWrite]  ";
		case P9_TFlush:		return "[TFlush]  ";
		case P9_RError:		return "[RError]  ";
		case P9_TWalk:		return "[TWalk]   ";
		case P9_RWalk:		return "[RWalk]   ";
		case P9_TCreate:	return "[TCreate] ";
		case P9_RCreate:	return "[RCreate] ";
		case P9_TClunk:		return "[TClunk]  ";
		case P9_RClunk:         return "[RClunk]  ";
		case P9_TRemove:	return "[TRemove] ";
		case P9_TStat:		return "[TStat]   ";
		case P9_RStat:		return "[RStat]   ";
		case P9_TWStat:		return "[TWStat]  ";
		default:		return "(unknown) ";
	}
}

#define _MSGDUMP(fmt...)						\
	if (mixp_dump)							\
	{								\
		fprintf(mixp_debug_stream,"%s %s %-5ld tag=%-5d ",	\
			(msg->mode==MsgUnpack) ? ">>":"<<",		\
			fcall_type2str(fcall->type),			\
			(msg->end - msg->data),				\
			fcall->tag					\
		);							\
		fprintf(mixp_debug_stream,fmt);				\
		fprintf(mixp_debug_stream,"\n");			\
	}

void
ixp_pfcall(MIXP_MESSAGE *msg, MIXP_FCALL *fcall)
{
	mixp_pu8(msg, &fcall->type);
	mixp_pu16(msg, &fcall->tag);

#ifdef DEBUG
	fprintf(mixp_debug_stream, "ixp_pfcall() %s tag=%d type=%d %s\n",
		((msg->mode==MsgPack) ? "PACK" : ((msg->mode==MsgUnpack) ? "UNPACK" : "???")),
		fcall->tag,
		fcall->type,
		fcall_type2str(fcall->type));
#endif

	switch (fcall->type) {
		case P9_TVersion:
			mixp_pu32(msg, &fcall->Tversion.msize);
			mixp_pstring(msg, &fcall->Tversion.version);
			_MSGDUMP("msize=%d version=\"%s\"", fcall->Tversion.msize, fcall->Tversion.version);
		break;
		case P9_RVersion:
			mixp_pu32(msg, &fcall->Rversion.msize);
			mixp_pstring(msg, &fcall->Rversion.version);
			_MSGDUMP("msize=%d version=\"%s\"", fcall->Tversion.msize, fcall->Tversion.version);
		break;
		case P9_TAuth:
			mixp_pu32(msg, &fcall->Tauth.afid);
			mixp_pstring(msg, &fcall->Tauth.uname);
			mixp_pstring(msg, &fcall->Tauth.aname);
			_MSGDUMP("afid=%d uname=\"%s\" aname=\"%s\"", fcall->Tauth.afid, fcall->Tauth.uname, fcall->Tauth.aname);
		break;
		case P9_RAuth:
			mixp_pqid(msg, &fcall->Rauth.aqid);
			_MSGDUMP(" ");
		break;
		case P9_RAttach:
			mixp_pqid(msg, &fcall->Rattach.qid);
			_MSGDUMP(" ");
		break;
		case P9_TAttach:
			mixp_pu32(msg, &fcall->fid);
			mixp_pu32(msg, &fcall->Tattach.afid);
			mixp_pstring(msg, &fcall->Tattach.uname);
			mixp_pstring(msg, &fcall->Tattach.aname);
			_MSGDUMP("fid=%d afid=%d uname=\"%s\" aname=\"%s\"", fcall->fid, fcall->Tattach.afid, fcall->Tattach.uname, fcall->Tattach.aname);
		break;
		case P9_RError:
			mixp_pstring(msg, &fcall->Rerror.ename);
			_MSGDUMP("ename=\"%s\"", fcall->Rerror.ename);
		break;
		case P9_TFlush:
			mixp_pu16(msg, &fcall->Tflush.oldtag);
			_MSGDUMP("oldtag=%d", fcall->Tflush.oldtag);
		break;
		case P9_TWalk:
			mixp_pu32(msg, &fcall->fid);
			mixp_pu32(msg, &fcall->Twalk.newfid);
			mixp_pstrings(msg, &fcall->Twalk.nwname, fcall->Twalk.wname);
			{
				char _dumpbuf[65535] = {0};
				int _x;
				for (_x=0; _x<fcall->Twalk.nwname; _x++)
				{
					if (_x) strcat(_dumpbuf,", ");
					strcat(_dumpbuf,"\"");
					strcat(_dumpbuf,fcall->Twalk.wname[_x]);
					strcat(_dumpbuf,"\"");
				}
				_MSGDUMP("fid=%d newfid=%d nwname[]={ %s }", fcall->fid, fcall->Twalk.newfid, _dumpbuf);
			}
		break;
		case P9_RWalk:
			mixp_pqids(msg, &fcall->Rwalk.nwqid, fcall->Rwalk.wqid);
			_MSGDUMP("nwqid=%d", fcall->Rwalk.nwqid);
		break;
		case P9_TOpen:
			mixp_pu32(msg, &fcall->fid);
			mixp_pu8(msg, &fcall->Topen.mode);
			_MSGDUMP("fid=%d mode=%d", fcall->fid, fcall->Topen.mode);
		break;
		case P9_ROpen:
			mixp_pqid(msg, &fcall->Ropen.qid);
			mixp_psize(msg, &fcall->Ropen.iounit);
			_MSGDUMP("iounit=%ld", (long)fcall->Ropen.iounit);
		break;
		case P9_RCreate:
			mixp_pqid(msg, &fcall->Rcreate.qid);
			mixp_psize(msg, &fcall->Rcreate.iounit);
			_MSGDUMP("iounit=%ld", (long)fcall->Rcreate.iounit);
		break;
		case P9_TCreate:
			mixp_pu32(msg, &fcall->fid);
			mixp_pstring(msg, &fcall->Tcreate.name);
			mixp_pu32(msg, &fcall->Tcreate.perm);
			mixp_pu8(msg, &fcall->Tcreate.mode);
			_MSGDUMP("fid=%ld name=\"%s\" perm=%ld mode=%ld", (long)fcall->fid, fcall->Tcreate.name, (long)fcall->Tcreate.perm, (long)fcall->Tcreate.mode);
		break;
		case P9_TRead:
			mixp_pu32(msg, &fcall->fid);
			mixp_pu64(msg, &fcall->Tread.offset);
			mixp_psize(msg, &fcall->Tread.count);
			_MSGDUMP("fid=%ld offset=%llu count=%ld", (long)fcall->fid, (long long unsigned int)fcall->Tread.offset, (long)fcall->Tread.count);
		break;
		case P9_RRead:
			mixp_psize(msg, &fcall->Rread.count);
			mixp_pdata(msg, &fcall->Rread.data, fcall->Rread.count);
			_MSGDUMP("count=%ld", (long)fcall->Rread.count);
		break;
		case P9_TWrite:
			mixp_pu32(msg, &fcall->fid);
			mixp_pu64(msg, &fcall->Twrite.offset);
			mixp_psize(msg, &fcall->Twrite.count);
			mixp_pdata(msg, &fcall->Twrite.data, fcall->Twrite.count);
			_MSGDUMP("fid=%u offset=%llu count=%ld", fcall->fid, (long long unsigned int)fcall->Twrite.offset, (long)fcall->Twrite.count);
		break;
		case P9_RWrite:
			mixp_psize(msg, &fcall->Rwrite.count);
			_MSGDUMP("count=%ld", (long)fcall->Rwrite.count);
		break;
		case P9_RClunk:
			mixp_pu32(msg, &fcall->fid);
			_MSGDUMP("fid=%ld", (long)fcall->fid);
		break;
		case P9_TClunk:
			mixp_pu32(msg, &fcall->fid);
			_MSGDUMP("fid=%ld", (long)fcall->fid);
		break;
		case P9_TRemove:
			mixp_pu32(msg, &fcall->fid);
			_MSGDUMP("fid=%ld", (long)fcall->fid);
		break;
		case P9_TStat:
			mixp_pu32(msg, &fcall->fid);
			_MSGDUMP("fid=%d", fcall->fid);
		break;
		case P9_RStat:
			mixp_pu16(msg, &fcall->Rstat.nstat);
			mixp_pdata(msg, (char**)&fcall->Rstat.stat, fcall->Rstat.nstat);
			_MSGDUMP("nstat=%ld", (long)fcall->Rstat.nstat);
		break;
		case P9_TWStat:
			mixp_pu32(msg, &fcall->fid);
			mixp_pu16(msg, &fcall->Twstat.nstat);
			mixp_pdata(msg, (char**)&fcall->Twstat.stat, fcall->Twstat.nstat);
			_MSGDUMP("fid=%ld nstat=%ld", (long)fcall->fid, (long)fcall->Twstat.nstat);
		break;
		default:
			_MSGDUMP(" unhandled type %ld", (long)fcall->type);
	}
}

size_t
mixp_fcall2msg(MIXP_MESSAGE *msg, MIXP_FCALL *fcall)
{
	size_t size;

	msg->end = msg->data + msg->size;
	msg->pos = msg->data + SDWord;
	msg->mode = MsgPack;
	ixp_pfcall(msg, fcall);

	if(msg->pos > msg->end)
		return 0;

	msg->end = msg->pos;
	size = msg->end - msg->data;

	msg->pos = msg->data;
	mixp_psize(msg, &size);

	msg->pos = msg->data;
	return size;
}

size_t
mixp_msg2fcall(MIXP_MESSAGE *msg, MIXP_FCALL *fcall)
{
	msg->pos = msg->data + SDWord;
	msg->mode = MsgUnpack;
	ixp_pfcall(msg, fcall);

	if(msg->pos > msg->end)
		return 0;

	return msg->pos - msg->data;
}
