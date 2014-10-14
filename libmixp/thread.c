
#include <unistd.h>

#include <9p-mixp/bits.h>

#include "mixp_local.h"

static MIXP_THREAD ixp_nothread;
MIXP_THREAD *mixp_thread = &ixp_nothread;

static char*
errbuf() {
	static char errbuf[MIXP_MAX_ERROR];

	return errbuf;
}

static void
mvoid(MIXP_MUTEX *m) {
	USED(m);
	return;
}

static int
mtrue(MIXP_MUTEX *m) {
	USED(m);
	return 1;
}

static int
mfalse(MIXP_MUTEX *m) {
	USED(m);
	return 0;
}

static void
rwvoid(MIXP_RWLOCK *rw) {
	USED(rw);
	return;
}

static int
rwtrue(MIXP_RWLOCK *rw) {
	USED(rw);
	return 1;
}

static int
rwfalse(MIXP_RWLOCK *m) {
	USED(m);
	return 0;
}

static void
rvoid(MIXP_RENDEZ *r) {
	USED(r);
	return;
}

static int
rfalse(MIXP_RENDEZ *r) {
	USED(r);
	return 0;
}

static void
rsleep(MIXP_RENDEZ *r) {
	USED(r);
	fprintf(mixp_error_stream, "libmixp: rsleep() not implemented\n");
}

static MIXP_THREAD ixp_nothread = {
	/* RWLock */
	.initrwlock = rwfalse,
	.rlock = rwvoid,
	.runlock = rwvoid,
	.canrlock = rwtrue,
	.wlock = rwvoid,
	.wunlock = rwvoid,
	.canwlock = rwtrue,
	.rwdestroy = rwvoid,
	/* Mutex */
	.initmutex = mfalse,
	.lock = mvoid,
	.unlock = mvoid,
	.canlock = mtrue,
	.mdestroy = mvoid,
	/* Rendez */
	.initrendez = rfalse,
	.sleep = rsleep,
	.wake = rfalse,
	.wakeall = rfalse,
	.rdestroy = rvoid,
	/* Other */
	.errbuf = errbuf,
	.read = read,
	.write = write,
};

