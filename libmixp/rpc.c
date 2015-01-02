/* From Plan 9's libmux.
 * Copyright (c) 2003 Russ Cox, Massachusetts Institute of Technology
 * Distributed under the same terms as libixp.
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mixp_local.h"

#include <9p-mixp/fcall.h>
#include <9p-mixp/transport.h>
#include <9p-mixp/err.h>
#include <9p-mixp/client.h>
#include <9p-mixp/rpc.h>

#include "util.h"

static int gettag(MIXP_CLIENT*, MIXP_RPC*);
static void puttag(MIXP_CLIENT*, MIXP_RPC*);
static void enqueue(MIXP_CLIENT*, MIXP_RPC*);
static void dequeue(MIXP_CLIENT*, MIXP_RPC*);

void
muxinit(MIXP_CLIENT *mux)
{
	mux->tagrend.mutex = &mux->lk;
	mux->sleep.next = &mux->sleep;
	mux->sleep.prev = &mux->sleep;
	mixp_thread->initmutex(&mux->lk);
	mixp_thread->initmutex(&mux->rlock);
	mixp_thread->initmutex(&mux->wlock);
	mixp_thread->initrendez(&mux->tagrend);
}

void
muxfree(MIXP_CLIENT *mux)
{
	mixp_thread->mdestroy(&mux->lk);
	mixp_thread->mdestroy(&mux->rlock);
	mixp_thread->mdestroy(&mux->wlock);
	mixp_thread->rdestroy(&mux->tagrend);
	MIXP_FREE(mux->wait);
}

static void
initrpc(MIXP_CLIENT *mux, MIXP_RPC *r)
{
	r->mux = mux;
	r->waiting = 1;
	r->r.mutex = &mux->lk;
	r->p = NULL;
	mixp_thread->initrendez(&r->r);
}

static void
freemuxrpc(MIXP_RPC *r)
{
	mixp_thread->rdestroy(&r->r);
}

static int
sendrpc(MIXP_RPC *r, MIXP_FCALL *f)
{
	int ret;
	MIXP_CLIENT *mux;

	ret = 0;
	mux = r->mux;
	/* assign the tag, add selves to response queue */
	mixp_thread->lock(&mux->lk);
	r->tag = gettag(mux, r);
	f->tag = r->tag;
	enqueue(mux, r);
	mixp_thread->unlock(&mux->lk);

	mixp_thread->lock(&mux->wlock);
	if(!mixp_fcall2msg(&mux->wmsg, f) || !mixp_sendmsg(mux->fd, &mux->wmsg)) {
		/* mixp_werrstr("settag/send tag %d: %r", tag); fprint(2, "%r\n"); */
		mixp_thread->lock(&mux->lk);
		dequeue(mux, r);
		puttag(mux, r);
		mixp_thread->unlock(&mux->lk);
		ret = -1;
	}
	mixp_thread->unlock(&mux->wlock);
	return ret;
}

static MIXP_FCALL*
muxrecv(MIXP_CLIENT *mux)
{
	MIXP_FCALL *f;

	f = NULL;
	mixp_thread->lock(&mux->rlock);
	if(mixp_recvmsg(mux->fd, &mux->rmsg) == 0)
		goto fail;
	f = calloc(1,sizeof(MIXP_FCALL));
	if(mixp_msg2fcall(&mux->rmsg, f) == 0) {
		MIXP_FREE(f);
		f = NULL;
	}
fail:
	mixp_thread->unlock(&mux->rlock);
	return f;
}

static void
dispatchandqlock(MIXP_CLIENT *mux, MIXP_FCALL *f)
{
	int tag;
	MIXP_RPC *r2;

	tag = f->tag - mux->mintag;
	mixp_thread->lock(&mux->lk);
	/* hand packet to correct sleeper */
	if(tag < 0 || tag >= mux->mwait) {
		fprintf(mixp_error_stream, "libixp: recieved unfeasible tag: %d (min: %d, max: %d)\n", f->tag, mux->mintag, mux->mintag+mux->mwait);
		goto fail;
	}
	r2 = mux->wait[tag];
	if((r2 == NULL) || (r2->prev == NULL)) {
		fprintf(mixp_error_stream, "libixp: recieved message with bad tag\n");
		goto fail;
	}
	r2->p = f;
	dequeue(mux, r2);
	mixp_thread->wake(&r2->r);
	return;
fail:
	mixp_fcall_free(f);
	MIXP_FREE(f);
}

static void
electmuxer(MIXP_CLIENT *mux)
{
	MIXP_RPC *rpc;

	/* if there is anyone else sleeping, wake them to mux */
	for(rpc=mux->sleep.next; rpc != &mux->sleep; rpc = rpc->next){
		if(!rpc->async){
			mux->muxer = rpc;
			mixp_thread->wake(&rpc->r);
			return;
		}
	}
	mux->muxer = NULL;
}

MIXP_FCALL*
muxrpc(MIXP_CLIENT *mux, MIXP_FCALL *tx)
{
	MIXP_RPC rpc;
	MIXP_FCALL *p;

	initrpc(mux, &rpc);
	if(sendrpc(&rpc, tx) < 0)
		return NULL;

	mixp_thread->lock(&mux->lk);
	/* wait for our packet */
	while(mux->muxer && mux->muxer != &rpc && !rpc.p)
		mixp_thread->sleep(&rpc.r);

	/* if not done, there's no muxer; start muxing */
	if(!rpc.p){
		assert((mux->muxer == NULL) || (mux->muxer == &rpc));
		mux->muxer = &rpc;
		while(!rpc.p){
			mixp_thread->unlock(&mux->lk);
			p = muxrecv(mux);
			if(p == NULL){
				/* eof -- just give up and pass the buck */
				mixp_thread->lock(&mux->lk);
				dequeue(mux, &rpc);
				break;
			}
			dispatchandqlock(mux, p);
		}
		electmuxer(mux);
	}
	p = rpc.p;

	puttag(mux, &rpc);
	mixp_thread->unlock(&mux->lk);
	if(p == NULL)
		mixp_werrstr("unexpected eof");
	return p;
}

static void
enqueue(MIXP_CLIENT *mux, MIXP_RPC *r)
{
	r->next = mux->sleep.next;
	r->prev = &mux->sleep;
	r->next->prev = r;
	r->prev->next = r;
}

static void
dequeue(MIXP_CLIENT *mux, MIXP_RPC *r)
{
	r->next->prev = r->prev;
	r->prev->next = r->next;
	r->prev = NULL;
	r->next = NULL;
}

static int 
gettag(MIXP_CLIENT *mux, MIXP_RPC *r)
{
	int i, mw;
	MIXP_RPC **w;

	for(;;){
		/* wait for a free tag */
		while(mux->nwait == mux->mwait){
			if(mux->mwait < mux->maxtag-mux->mintag){
				mw = mux->mwait;
				if(mw == 0)
					mw = 1;
				else
					mw <<= 1;
				w = realloc(mux->wait, mw*sizeof(w[0]));
				if(w == NULL)
					return -1;
				memset(w+mux->mwait, 0, (mw-mux->mwait)*sizeof(w[0]));
				mux->wait = w;
				mux->freetag = mux->mwait;
				mux->mwait = mw;
				break;
			}
			mixp_thread->sleep(&mux->tagrend);
		}

		i=mux->freetag;
		if(mux->wait[i] == 0)
			goto Found;
		for(; i<mux->mwait; i++)
			if(mux->wait[i] == 0)
				goto Found;
		for(i=0; i<mux->freetag; i++)
			if(mux->wait[i] == 0)
				goto Found;
		/* should not fall out of while without free tag */
		abort();
	}

Found:
	mux->nwait++;
	mux->wait[i] = r;
	r->tag = i+mux->mintag;
	return r->tag;
}

static void
puttag(MIXP_CLIENT *mux, MIXP_RPC *r)
{
	int i;

	i = r->tag - mux->mintag;
	assert(mux->wait[i] == r);
	mux->wait[i] = NULL;
	mux->nwait--;
	mux->freetag = i;
	mixp_thread->wake(&mux->tagrend);
	freemuxrpc(r);
}
