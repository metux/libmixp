/* Copyright ©2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "mixp_local.h"
#include <9p-mixp/stat.h>
#include <9p-mixp/err.h>
#include <9p-mixp/convert.h>

#include "util.h"

#define nelem(ary) (sizeof(ary) / sizeof(*ary))

enum {
	RootFid = 1,
};

static int
min(int a, int b) {
	if(a < b)
		return a;
	return b;
}

static MIXP_CFID *
getfid(MIXP_CLIENT *c) {
	MIXP_CFID *f;

	mixp_thread->lock(&c->lk);
	f = c->freefid;
	if(f != NULL)
		c->freefid = f->next;
	else {
		f = calloc(1,sizeof *f);
		f->client = c;
		f->fid = ++c->lastfid;
		mixp_thread->initmutex(&f->iolock);
	}
	f->next = NULL;
	f->open = 0;
	mixp_thread->unlock(&c->lk);
	return f;
}

static void
putfid(MIXP_CFID *f) {
	MIXP_CLIENT *c;

	c = f->client;
	mixp_thread->lock(&c->lk);
	if(f->fid == c->lastfid) {
		c->lastfid--;
		mixp_thread->mdestroy(&f->iolock);
		MIXP_FREE(f);
	}else {
		f->next = c->freefid;
		c->freefid = f;
	}
	mixp_thread->unlock(&c->lk);
}

static IxpFcall* do_fcall(MIXP_CLIENT* c, IxpFcall* fcall)
{
	IxpFcall *ret;
	__init_errstream();
	ret = muxrpc(c, fcall);
	if(ret == NULL)
	{
//		fprintf(mixp_error_stream,"MIXP: fcall without reply\n");
		return NULL;
	}

	if(ret->type == P9_RError) {
		mixp_werrstr("%s", ret->Rerror.ename);
		fprintf(mixp_error_stream, "MIXP: fcall returned error %s\n", ret->Rerror.ename);
		return NULL;
	}
	if(ret->type != (fcall->type^1)) {
		mixp_werrstr("received mismatched fcall");
		fprintf(mixp_error_stream, "MIXP: received mismatched fcall\n");
		return NULL;
	}
	return ret;
}

static int
dofcall(MIXP_CLIENT *c, IxpFcall *fcall) {
	IxpFcall *ret;
    
	ret = muxrpc(c, fcall);
	if(ret == NULL)
		return 0;
	if(ret->type == P9_RError) {
		mixp_werrstr("%s", ret->Rerror.ename);
		goto fail;
	}
	if(ret->type != (fcall->type^1)) {
		mixp_werrstr("received mismatched fcall");
		goto fail;
	}
	memcpy(fcall, ret, sizeof(IxpFcall));
	MIXP_FREE(ret);
	return 1;
fail:
	mixp_fcall_free(fcall);
	return 0;
}

void
mixp_unmount(MIXP_CLIENT *c) {
	MIXP_CFID *f;

	shutdown(c->fd, SHUT_RDWR);
	close(c->fd);

	muxfree(c);

	while((f = c->freefid)) {
		c->freefid = f->next;
		mixp_thread->mdestroy(&f->iolock);
		MIXP_FREE(f);
	}
	MIXP_FREE(c->rmsg.data);
	MIXP_FREE(c->wmsg.data);
	MIXP_FREE(c);
}

static void
allocmsg(MIXP_CLIENT *c, int n) {
	c->rmsg.size = n;
	c->wmsg.size = n;
	c->rmsg.data = ixp_erealloc(c->rmsg.data, n);
	c->wmsg.data = ixp_erealloc(c->wmsg.data, n);
}

MIXP_CLIENT *
mixp_mountfd(int fd) {
	MIXP_CLIENT *c;
	IxpFcall* fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
	IxpFcall* retfcall = NULL;

	c = calloc(1,sizeof(*c));
	c->fd = fd;

	muxinit(c);

	allocmsg(c, 256);
	c->lastfid = RootFid;
	/* Override tag matching on P9_TVersion */
	c->mintag = IXP_NOTAG;
	c->maxtag = IXP_NOTAG+1;

	fcall->type = P9_TVersion;
	fcall->Tversion.msize = IXP_MAX_MSG;
	fcall->Tversion.version = IXP_VERSION;

	if ((retfcall = do_fcall(c, fcall))==NULL)
	{
		mixp_unmount(c);
		mixp_fcall_free(fcall);
		return NULL;
	}

	if(strcmp(retfcall->Rversion.version, IXP_VERSION) || 
	          retfcall->Rversion.msize > IXP_MAX_MSG) {
		mixp_werrstr("bad 9P version response");
		mixp_unmount(c);
		mixp_fcall_free(fcall);
		mixp_fcall_free(retfcall);
		return NULL;
	}

	c->mintag = 0;
	c->maxtag = 255;

	allocmsg(c, retfcall->Tversion.msize);

	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	
	fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));

	fcall->type = P9_TAttach;
	fcall->fid = RootFid;
	fcall->Tattach.afid = IXP_NOFID;
	fcall->Tattach.uname = getenv("USER");
	fcall->Tattach.aname = "";
	if((retfcall = do_fcall(c, fcall)) == NULL) {
		mixp_unmount(c);
		mixp_fcall_free(fcall);
		return NULL;
	}

	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	return c;
}

MIXP_CLIENT *
mixp_mount(const char *address) {
	int fd;

	fd = mixp_dial(address);
	if(fd < 0)
		return NULL;
	return mixp_mountfd(fd);
}

MIXP_CLIENT * mixp_mount_addr(MIXP_SERVER_ADDRESS *addr)
{
	int fd;

	fd = mixp_dial_addr(addr);
	if(fd < 0)
		return NULL;
	return mixp_mountfd(fd);
}

static MIXP_CFID *
walk(MIXP_CLIENT *c, const char *pathname) {
	MIXP_CFID *f;
	int n;

	IxpFcall* fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
	IxpFcall* retfcall = NULL;

	char* path = strdup(pathname);
	n = ixp_tokenize(fcall->Twalk.wname, nelem(fcall->Twalk.wname), path, '/');

	f = getfid(c);

	fcall->type = P9_TWalk;
	fcall->fid = RootFid;
	fcall->Twalk.nwname = n;
	fcall->Twalk.newfid = f->fid;
	if((retfcall=do_fcall(c, fcall)) == NULL)
		goto fail;
	if(retfcall->Rwalk.nwqid < n)
		goto fail;

	f->qid = fcall->Rwalk.wqid[n-1];

	goto out;
fail:
	putfid(f);
	f = NULL;
out:
	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	free(path);
	return f;
}

static MIXP_CFID *
walkdir(MIXP_CLIENT *c, char *path, char **rest) {
	char *p;

	p = path + strlen(path) - 1;
	assert(p >= path);
	while(*p == '/')
		*p-- = '\0';

	while((p > path) && (*p != '/'))
		p--;
	if(*p != '/') {
		mixp_werrstr("bad path");
		return NULL;
	}

	*p++ = '\0';
	*rest = p;
	return walk(c, path);
}

static int
clunk(MIXP_CFID *f) {
	MIXP_CLIENT *c;
	IxpFcall* fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
	IxpFcall* retfcall = NULL;

	c = f->client;

	fcall->type = P9_TClunk;
	fcall->fid  = f->fid;
	if ((retfcall=do_fcall(c,fcall))==NULL)
	{
	    mixp_fcall_free(fcall);
	    return 0;
	}
	else
	{
	    putfid(f);
	    mixp_fcall_free(retfcall);
	    mixp_fcall_free(fcall);
	    return 1;
	}
}

int
mixp_remove(MIXP_CLIENT *c, const char *path) 
{
	MIXP_CFID *f;
	if((f = walk(c, path)) == NULL)
		return 0;

	IxpFcall *fcall = calloc(1,sizeof(IxpFcall));
	fcall->type = P9_TRemove;
	fcall->fid  = f->fid;

	IxpFcall *retfcall = NULL;
	if ((retfcall = do_fcall(c, fcall))==NULL)
	{
	    mixp_fcall_free(fcall);
	    return 0;
	}
	else
	{
	    putfid(f);
	    mixp_fcall_free(fcall);
	    mixp_fcall_free(retfcall);
	    return 1;
	}
}

static void
initfid(MIXP_CFID *f, IxpFcall *fcall) 
{
	__init_errstream();

	f->open = 1;
	f->offset = 0;
	f->iounit = min(fcall->Ropen.iounit, IXP_MAX_MSG-17);
	f->qid = fcall->Ropen.qid;
	
	if (!(f->iounit))
	{
//	    fprintf(mixp_error_stream,"initfid() iounit missing. fixing to %d\n", DEFAULT_IOUNIT);
	    f->iounit=DEFAULT_IOUNIT;
	}
	
	if (f->iounit < 0)
	{
//	    fprintf(mixp_error_stream,"initfid() iounit <0: %d .. fixing to %d\n", f->iounit, DEFAULT_IOUNIT);
	    f->iounit=DEFAULT_IOUNIT;
	}    
}

MIXP_CFID*
mixp_create(MIXP_CLIENT *c, const char *filename, unsigned int perm, unsigned char mode) 
{
	MIXP_CFID *f;
	char *path = strdup(filename);
	char *name = strdup(filename);	// FIXME: correct ?!

	if ((f = walkdir(c, path, &name))==NULL)
	{
		MIXP_FREE(path);
		MIXP_FREE(name);
	}

	IxpFcall *fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
	fcall->type = P9_TCreate;
	fcall->fid = f->fid;
	fcall->Tcreate.name = name;
	fcall->Tcreate.perm = perm;
	fcall->Tcreate.mode = mode;

	IxpFcall* retfcall;
	if ((retfcall = do_fcall(c, fcall)) == NULL) 
	{
		clunk(f);
		f = NULL;
	}
	else
	{
		initfid(f, retfcall);
		f->mode = mode;
		mixp_fcall_free(retfcall);
	}
	return f;
}

MIXP_CFID*
mixp_open(MIXP_CLIENT *c, const char *name, unsigned char mode) 
{
	MIXP_CFID *f;

	f = walk(c, name);
	if(f == NULL)
		return NULL;

	IxpFcall* fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
	fcall->type       = P9_TOpen;
	fcall->fid        = f->fid;
	fcall->Topen.mode = mode;

	IxpFcall* retfcall;
	
	if ((retfcall=do_fcall(c, fcall)) == NULL) 
	{
		clunk(f);
		mixp_fcall_free(fcall);
		return NULL;
	}

	initfid(f, retfcall);
	f->mode = mode;
	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	return f;
}

int mixp_close(MIXP_CFID *f) 
{
	return clunk(f);
}

MIXP_STAT *
mixp_stat(MIXP_CLIENT *c, const char *path) 
{
	MIXP_MESSAGE msg;
	MIXP_STAT *stat;
	MIXP_CFID *f;

	stat = NULL;
	f = walk(c, path);
	if(f == NULL)
		return NULL;

	IxpFcall* fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
	fcall->type = P9_TStat;
	fcall->fid = f->fid;
	
	IxpFcall* retfcall;
	if((retfcall = do_fcall(c, fcall))==NULL)
		goto done;

	msg = mixp_message(retfcall->Rstat.stat, retfcall->Rstat.nstat, MsgUnpack);

	stat = malloc(sizeof(MIXP_STAT));
	mixp_pstat(&msg, stat);
	mixp_fcall_free(fcall);
	if(msg.pos > msg.end) {
		MIXP_FREE(stat);
		stat = 0;
	}

done:
	clunk(f);
	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	return stat;
}

static long _pread(MIXP_CFID *f, void *buf, size_t count, int64_t offset) 
{
	__init_errstream();

	IxpFcall* fcall = NULL;
	IxpFcall* retfcall = NULL;
	int len = 0;
	while(len < count)
	{
		mixp_fcall_free(fcall);		fcall = NULL;
		mixp_fcall_free(retfcall);	retfcall = NULL;

		fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
		int n = min(count-len, f->iounit);

		fcall->type = P9_TRead;
		fcall->fid  = f->fid;
		fcall->Tread.offset = offset;
		fcall->Tread.count = n;
		
		if ((retfcall = do_fcall(f->client, fcall)) == NULL)
		{
			fprintf(mixp_error_stream,"MIXP: _pread() fcall failed\n");
			goto err;
		}

		if (retfcall->Rread.count > n)
		{
			fprintf(mixp_error_stream,"MIXP: _pread() received more (%d) than requested (%d)\n", retfcall->Rread.count, n);
			goto err;
		}

		memcpy(buf+len, retfcall->Rread.data, retfcall->Rread.count);
		offset += retfcall->Rread.count;
		len += retfcall->Rread.count;

		if(retfcall->Rread.count < n)
			break;
	}
	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	return len;
err:
	mixp_fcall_free(fcall);
	mixp_fcall_free(retfcall);
	return -1;
}

long mixp_read(MIXP_CFID *f, void *buf, size_t count) 
{
	int n;

	mixp_thread->lock(&f->iolock);
	n = _pread(f, buf, count, f->offset);
	if(n > 0)
		f->offset += n;
	mixp_thread->unlock(&f->iolock);
	return n;
}

long mixp_pread(MIXP_CFID *f, void *buf, long count, int64_t offset) 
{
	int n;

	mixp_thread->lock(&f->iolock);
	n = _pread(f, buf, count, offset);
	mixp_thread->unlock(&f->iolock);
	return n;
}

static long _pwrite(MIXP_CFID *f, const void *buf, long count, int64_t offset) 
{
	int n, len;
	len = 0;
	do 
	{
		IxpFcall* fcall = (IxpFcall*)calloc(1,sizeof(IxpFcall));
		IxpFcall* retfcall = NULL;

		n = min(count-len, f->iounit);
		fcall->type = P9_TWrite;
		fcall->fid = f->fid;
		fcall->Twrite.offset = f->offset;
		fcall->Twrite.data = (char*)(buf + len);
		fcall->Twrite.count = n;
		if ((retfcall = do_fcall(f->client, fcall)) == NULL)
		{
		    mixp_fcall_free(fcall);
		    return -1;
		}

		f->offset += retfcall->Rwrite.count;
		len += retfcall->Rwrite.count;

		if(fcall->Rwrite.count < n)
		{
			mixp_fcall_free(fcall);
			mixp_fcall_free(retfcall);
			return len;
		}
		mixp_fcall_free(fcall);
		mixp_fcall_free(retfcall);
	} while(len < count);
	return len;
}

long
mixp_write(MIXP_CFID *f, const void *buf, long count) {
	int n;

	mixp_thread->lock(&f->iolock);
	n = _pwrite(f, buf, count, f->offset);
	if(n > 0)
		f->offset += n;
	mixp_thread->unlock(&f->iolock);
	return n;
}

long
mixp_pwrite(MIXP_CFID *f, const void *buf, long count, int64_t offset) {
	int n;

	mixp_thread->lock(&f->iolock);
	n = _pwrite(f, buf, count, offset);
	mixp_thread->unlock(&f->iolock);
	return n;
}
