/* Copyright ©2006-2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include "mixp_local.h"

#include <9p-mixp/transport.h>
#include <9p-mixp/intmap.h>

int mixp_dump = 0;

static void handlereq(Ixp9Req *r);

static int
min(int a, int b) {
	if(a < b)
		return a;
	return b;
}

static char
	Eduptag[] = "tag in use",
	Edupfid[] = "fid in use",
	Enofunc[] = "function not implemented",
	Ebotch[] = "9P protocol botch",
	Enofile[] = "file does not exist",
	Enofid[] = "fid does not exist",
	Enotag[] = "tag does not exist",
	Enotdir[] = "not a directory",
	Eintr[] = "interrupted",
	Eisdir[] = "cannot perform operation on a directory";

enum {
	TAG_BUCKETS = 61,
	FID_BUCKETS = 61,
};

struct MIXP_9CONN {
	MIXP_INTMAP	tagmap;
	MIXP_INTMAP	fidmap;
	Ixp9Srv		*srv;
	MIXP_CONNECTION	*conn;
	MIXP_MUTEX	rlock, wlock;
	MIXP_MESSAGE	rmsg;
	MIXP_MESSAGE	wmsg;
	int		ref;
	void		*taghash[TAG_BUCKETS];
	void		*fidhash[FID_BUCKETS];
};

static void
decref_p9conn(MIXP_9CONN *pc) {
	mixp_thread->lock(&pc->wlock);
	if(--pc->ref > 0) {
		mixp_thread->unlock(&pc->wlock);
		return;
	}
	mixp_thread->unlock(&pc->wlock);

	mixp_thread->mdestroy(&pc->rlock);
	mixp_thread->mdestroy(&pc->wlock);

	mixp_intmap_free(&pc->tagmap, NULL);
	mixp_intmap_free(&pc->fidmap, NULL);

	MIXP_FREE(pc->rmsg.data)
	MIXP_FREE(pc->wmsg.data)
	MIXP_FREE(pc)
}

static void *
createfid(MIXP_INTMAP *map, int fid, MIXP_9CONN *pc) {
	MIXP_FID *f;

	f = calloc(1,sizeof(MIXP_FID));
	pc->ref++;
	f->conn = pc;
	f->fid = fid;
	f->omode = -1;
	f->map = map;
	if(mixp_intmap_caninsertkey(map, fid, f))
		return f;
	fprintf(mixp_error_stream,"couldnt insert key (already exists): %d\n",fid);
	MIXP_FREE(f)
	return NULL;
}

static int
destroyfid(MIXP_9CONN *pc, unsigned long fid) {
	MIXP_FID *f;

	f = mixp_intmap_deletekey(&pc->fidmap, fid);
	if(f == NULL)
		return 0;

	if(pc->srv->freefid)
		pc->srv->freefid(f);

	decref_p9conn(pc);
	MIXP_FREE(f)
	return 1;
}

Ixp9Req* mixp_9req_alloc(MIXP_9CONN *conn)
{
	Ixp9Req * req = (Ixp9Req*) calloc(1,sizeof(Ixp9Req));
	req->ifcall   = (MIXP_FCALL*)calloc(1,sizeof(MIXP_FCALL));
	req->ofcall   = (MIXP_FCALL*)calloc(1,sizeof(MIXP_FCALL));
	
	if (conn)
	{
	    req->conn = conn;
	    req->srv  = conn->srv;
	    conn->ref++;
	}

	return req;
}

void mixp_9req_free(Ixp9Req* req)
{
	if (req==NULL);
	    return;
	mixp_fcall_free(req->ifcall);
	mixp_fcall_free(req->ofcall);
	
	if (req->conn)
	{
	    decref_p9conn(req->conn);
	    req->conn = NULL;
	    req->srv  = NULL;
	}
}

static void
handlefcall(MIXP_CONNECTION *c) 
{
	MIXP_9CONN *pc  = c->aux;
	Ixp9Req * req = mixp_9req_alloc(pc);

	// lock the connection
	mixp_thread->lock(&pc->rlock);

	// try to receive an packet - jump to Fail: if failed
	if(mixp_recvmsg(c->fd, &pc->rmsg) == 0)
		goto Fail;
		
	// decode incoming packet to to req->ifcall (MIXP_FCALL)
	if(ixp_msg2fcall(&pc->rmsg, req->ifcall) == 0)
		goto Fail;
	
	// unlock the connection 
	mixp_thread->unlock(&pc->rlock);

	pc->conn  = c;

	if(!mixp_intmap_caninsertkey(&pc->tagmap, req->ifcall->tag, req)) {
		fprintf(mixp_error_stream,"duplicate tag: %d\n", req->ifcall->tag);
		ixp_respond(req, Eduptag);
		decref_p9conn(pc);
		return;
	}

	handlereq(req);
	mixp_9req_free(req);
	return;

Fail:
	mixp_thread->unlock(&pc->rlock);
	ixp_hangup(c);
	mixp_9req_free(req);
	return;
}

static void
handlereq(Ixp9Req *r) 
{
	MIXP_9CONN *pc;
	Ixp9Srv *srv;

	pc = r->conn;
	srv = pc->srv;

	if (r->ifcall == NULL)
	{
	    fprintf(mixp_error_stream,"EMERG: handlereq() r->ifcall corrupt\n");
	    return;
	}

	switch(r->ifcall->type) 
	{
	    default:
		ixp_respond(r, Enofunc);
		break;
	    case P9_TVersion:
		if(!strcmp(r->ifcall->Tversion.version, "9P"))
			r->ofcall->Rversion.version = "9P";
		else if(!strcmp(r->ifcall->Tversion.version, "9P2000"))
			r->ofcall->Rversion.version = "9P2000";
		else
		{	
			fprintf(mixp_error_stream,"handlereq() TVersion: unknown version requested \"%s\"\n", r->ifcall->Tversion.version);
			r->ofcall->Rversion.version = "unknown";
		}
		r->ofcall->Rversion.msize = r->ifcall->Tversion.msize;
		if (r->ofcall->Rversion.msize == 0)
		{
		    fprintf(mixp_error_stream,"handlereq() TVersion: got zero msize. setting to default: %d\n", IXP_MAX_MSG);
		    r->ofcall->Rversion.msize = IXP_MAX_MSG;
		}
		ixp_respond(r, NULL);
		break;
	    case P9_TAttach:
		if(!(r->fid = createfid(&pc->fidmap, r->ifcall->fid, pc))) {
			fprintf(mixp_error_stream,"TAttach: duplicate fid\n");
			ixp_respond(r, Edupfid);
			return;
		}
		/* attach is a required function */
		srv->attach(r);
		break;
	    case P9_TClunk:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if(!srv->clunk) {
			ixp_respond(r, NULL);
			return;
		}
		srv->clunk(r);
		break;
	    case P9_TFlush:
		if(!(r->oldreq = mixp_intmap_lookupkey(&pc->tagmap, r->ifcall->Tflush.oldtag))) {
			ixp_respond(r, Enotag);
			return;
		}
		if(!srv->flush) {
			ixp_respond(r, Enofunc);
			return;
		}
		srv->flush(r);
		break;
	    case P9_TCreate:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if(r->fid->omode != -1) {
			ixp_respond(r, Ebotch);
			return;
		}
		if(!(r->fid->qid.type & P9_QTDIR)) {
			ixp_respond(r, Enotdir);
			return;
		}
		if(!pc->srv->create) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->create(r);
		break;
	    case P9_TOpen:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if((r->fid->qid.type & P9_QTDIR) && (r->ifcall->Topen.mode|P9_ORCLOSE) != (P9_OREAD|P9_ORCLOSE)) {
			ixp_respond(r, Eisdir);
			return;
		}
		r->ofcall->Ropen.qid = r->fid->qid;
		if(!pc->srv->open) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->open(r);
		break;
	    case P9_TRead:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if(r->fid->omode == -1 || r->fid->omode == P9_OWRITE) {
			ixp_respond(r, Ebotch);
			return;
		}
		if(!pc->srv->read) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->read(r);
		break;
	    case P9_TRemove:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if(!pc->srv->remove) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->remove(r);
		break;
	    case P9_TStat:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if(!pc->srv->stat) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->stat(r);
		break;
	    case P9_TWalk:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if(r->fid->omode != -1) {
			ixp_respond(r, "cannot walk from an open fid");
			return;
		}
		if(r->ifcall->Twalk.nwname && !(r->fid->qid.type & P9_QTDIR)) {
			ixp_respond(r, Enotdir);
			return;
		}
		if((r->ifcall->fid != r->ifcall->Twalk.newfid)) {
			if(!(r->newfid = createfid(&pc->fidmap, r->ifcall->Twalk.newfid, pc))) {
				ixp_respond(r, Edupfid);
				return;
			}
		}else
			r->newfid = r->fid;
		if(!pc->srv->walk) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->walk(r);
		break;
	    case P9_TWrite:
		if(!(r->fid = mixp_intmap_lookupkey(&pc->fidmap, r->ifcall->fid))) {
			ixp_respond(r, Enofid);
			return;
		}
		if((r->fid->omode&3) != P9_OWRITE && (r->fid->omode&3) != P9_ORDWR) {
			ixp_respond(r, "write on fid not opened for writing");
			return;
		}
		if(!pc->srv->write) {
			ixp_respond(r, Enofunc);
			return;
		}
		pc->srv->write(r);
		break;
	    /* Still to be implemented: wstat, auth */
	}
}

void
ixp_respond(Ixp9Req *r, const char *error) {
	MIXP_9CONN *pc;
	int msize;

	pc = r->conn;

	switch(r->ifcall->type) {
	default:
		if(!error)
			assert(!"ixp_respond called on unsupported fcall type");
		break;
	case P9_TVersion:
		assert(error == NULL);
		MIXP_FREE(r->ifcall->Tversion.version);		// move this to mixp_fcall_free()
		mixp_thread->lock(&pc->rlock);
		mixp_thread->lock(&pc->wlock);
		msize = min(r->ofcall->Rversion.msize, IXP_MAX_MSG);
		if (msize<1)
		{
		    fprintf(mixp_error_stream,"ixp_respond() P9_TVersion: msize<1 ! tweaking to IXP_MAX_MSG\n");
		    msize = IXP_MAX_MSG;
		}
		pc->rmsg.data = ixp_erealloc(pc->rmsg.data, msize);
		pc->wmsg.data = ixp_erealloc(pc->wmsg.data, msize);
		pc->rmsg.size = msize;
		pc->wmsg.size = msize;
		mixp_thread->unlock(&pc->wlock);
		mixp_thread->unlock(&pc->rlock);
		r->ofcall->Rversion.msize = msize;
		break;
	case P9_TAttach:
		if(error)
			destroyfid(pc, r->fid->fid);
		MIXP_FREE(r->ifcall->Tattach.uname)
		MIXP_FREE(r->ifcall->Tattach.aname)
		break;
	case P9_TOpen:
		if(!error) {
			r->fid->omode = r->ifcall->Tcreate.mode;
			r->fid->qid = r->ofcall->Rcreate.qid;
		}
		MIXP_FREE(r->ifcall->Tcreate.name)
		printf("P9_TOpen: rmsg.size=%ld\n", (long)pc->rmsg.size);
		r->ofcall->Rcreate.iounit = pc->rmsg.size - 16;
		printf("P9_TOpen: sending iounit: %ld\n", (long)r->ofcall->Rcreate.iounit);
		break;
	case P9_TCreate:
		if(!error) {
			r->fid->omode = r->ifcall->Tcreate.mode;
			r->fid->qid = r->ofcall->Rcreate.qid;
		}
		MIXP_FREE(r->ifcall->Tcreate.name)
		r->ofcall->Rcreate.iounit = pc->rmsg.size - 16;
		break;
	case P9_TWalk:
		if(error || r->ofcall->Rwalk.nwqid < r->ifcall->Twalk.nwname) {
			if(r->ifcall->fid != r->ifcall->Twalk.newfid && r->newfid)
				destroyfid(pc, r->newfid->fid);
			if(!error && r->ofcall->Rwalk.nwqid == 0)
				error = Enofile;
		}else{
			if(r->ofcall->Rwalk.nwqid == 0)
				r->newfid->qid = r->fid->qid;
			else
				r->newfid->qid = r->ofcall->Rwalk.wqid[r->ofcall->Rwalk.nwqid-1];
		}
		MIXP_FREE(*r->ifcall->Twalk.wname)
		break;
	case P9_TWrite:
		MIXP_FREE(r->ifcall->Twrite.data)
		break;
	case P9_TRemove:
		if(r->fid)
			destroyfid(pc, r->fid->fid);
		break;
	case P9_TClunk:
		if(r->fid)
			destroyfid(pc, r->fid->fid);
		r->ofcall->fid = r->ifcall->fid;
		break;
	case P9_TFlush:
		if((r->oldreq = mixp_intmap_lookupkey(&pc->tagmap, r->ifcall->Tflush.oldtag)))
			ixp_respond(r->oldreq, Eintr);
		break;
	case P9_TRead:
	case P9_TStat:
		break;
	/* Still to be implemented: wstat, auth */
	}

	r->ofcall->tag = r->ifcall->tag;

	if(error == NULL)
		r->ofcall->type = r->ifcall->type + 1;
	else {
		r->ofcall->type = P9_RError;
		r->ofcall->Rerror.ename = strdup(error);
	}

	mixp_intmap_deletekey(&pc->tagmap, r->ifcall->tag);;

	if(pc->conn) {
		mixp_thread->lock(&pc->wlock);
		msize = ixp_fcall2msg(&pc->wmsg, r->ofcall);
		if(mixp_sendmsg(pc->conn->fd, &pc->wmsg) != msize)
			ixp_hangup(pc->conn);
		mixp_thread->unlock(&pc->wlock);
	}

	switch(r->ofcall->type) {
	case P9_RStat:
		MIXP_FREE(r->ofcall->Rstat.stat)
		break;
	case P9_RRead:
//		MIXP_FREE(r->ofcall->Rread.data)
		break;
	}
}

/* Flush a pending request */
static void
voidrequest(void *t) {
	Ixp9Req *r, *tr;
	MIXP_9CONN *pc;

#ifdef DEBUG
	fprintf(mixp_debug_stream, "voidrequest: t=%ld\n", (long)t);
#endif

	r = t;
	pc = r->conn;
	tr = mixp_9req_alloc(pc);
	tr->ifcall->type = P9_TFlush;
	tr->ifcall->tag = IXP_NOTAG;
	tr->ifcall->Tflush.oldtag = r->ifcall->tag;
	handlereq(tr);
}

/* Clunk an open Fid -- called by intmap (callback) */
static void
voidfid(void *t) {
	MIXP_FID* f = t;
	MIXP_9CONN *pc = f->conn;
	Ixp9Req* tr = mixp_9req_alloc(pc);
	tr->ifcall->type = P9_TClunk;
	tr->ifcall->tag = IXP_NOTAG;
	tr->ifcall->fid = f->fid;
	tr->fid = f;
	handlereq(tr);
	mixp_9req_free(tr);
}

static void
cleanupconn(MIXP_CONNECTION *c) {
	MIXP_9CONN *pc;

	pc = c->aux;
	pc->conn = NULL;
	if(pc->ref > 1) {
		mixp_intmap_exec(&pc->tagmap, voidrequest);
		mixp_intmap_exec(&pc->fidmap, voidfid);
	}
	decref_p9conn(pc);
}

/* Handle incoming 9P connections */
void
serve_9pcon(MIXP_CONNECTION *c) {
	MIXP_9CONN *pc;
	int fd;

	fd = accept(c->fd, NULL, NULL);
	if(fd < 0)
		return;

	pc = calloc(1,sizeof(MIXP_9CONN));
	pc->ref++;
	pc->srv = c->aux;
	pc->rmsg.size = 1024;
	pc->wmsg.size = 1024;
	pc->rmsg.data = malloc(pc->rmsg.size);
	pc->wmsg.data = malloc(pc->wmsg.size);

	mixp_intmap_init(&pc->tagmap, TAG_BUCKETS, &pc->taghash, "tags");
	mixp_intmap_init(&pc->fidmap, FID_BUCKETS, &pc->fidhash, "fids");
	mixp_thread->initmutex(&pc->rlock);
	mixp_thread->initmutex(&pc->wlock);

	ixp_listen(c->srv, fd, pc, handlefcall, cleanupconn);
}
