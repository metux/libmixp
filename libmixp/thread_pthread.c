#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <9p-mixp/bits.h>
#include <9p-mixp/err.h>

#include "mixp_local.h"
#include "mixp_pthread.h"

static MIXP_THREAD ixp_pthread;
static pthread_key_t errstr_k;

int
mixp_pthread_init() {
	int ret;

	ret = pthread_key_create(&errstr_k, free);
	if(ret) {
		mixp_werrstr("can't create TLS value: %s", mixp_errbuf());
		return 1;
	}

	mixp_thread = &ixp_pthread;
	return 0;
}

static char*
errbuf(void) {
	char *ret;

	ret = pthread_getspecific(errstr_k);
	if(ret == NULL) {
		ret = calloc(1,MIXP_MAX_ERROR);
		pthread_setspecific(errstr_k, (void*)ret);
	}
	return ret;
}

static void
mlock(MIXP_MUTEX *m) {
	pthread_mutex_lock(m->aux);
}

static int
mcanlock(MIXP_MUTEX *m) {
	return !pthread_mutex_trylock(m->aux);
}

static void
munlock(MIXP_MUTEX *m) {
	pthread_mutex_unlock(m->aux);
}

static void
mdestroy(MIXP_MUTEX *m) {
	pthread_mutex_destroy(m->aux);
	free(m->aux);
}

static int
initmutex(MIXP_MUTEX *m) {
	pthread_mutex_t *mutex;

	mutex = calloc(1,sizeof *mutex);
	if(pthread_mutex_init(mutex, NULL)) {
		free(mutex);
		return 1;
	}

	m->aux = mutex;
	return 0;
}

static void
rlock(MIXP_RWLOCK *rw) {
	pthread_rwlock_rdlock(rw->aux);
}

static int
canrlock(MIXP_RWLOCK *rw) {
	return !pthread_rwlock_tryrdlock(rw->aux);
}

static void
wlock(MIXP_RWLOCK *rw) {
	pthread_rwlock_rdlock(rw->aux);
}

static int
canwlock(MIXP_RWLOCK *rw) {
	return !pthread_rwlock_tryrdlock(rw->aux);
}

static void
rwunlock(MIXP_RWLOCK *rw) {
	pthread_rwlock_unlock(rw->aux);
}

static void
rwdestroy(MIXP_RWLOCK *rw) {
	pthread_rwlock_destroy(rw->aux);
	free(rw->aux);
}

static int
initrwlock(MIXP_RWLOCK *rw) {
	pthread_rwlock_t *rwlock;

	rwlock = calloc(1,sizeof *rwlock);
	if(pthread_rwlock_init(rwlock, NULL)) {
		free(rwlock);
		return 1;
	}

	rw->aux = rwlock;
	return 0;
}

static void
rsleep(MIXP_RENDEZ *r) {
	pthread_cond_wait(r->aux, r->mutex->aux);
}

static int
rwake(MIXP_RENDEZ *r) {
	pthread_cond_signal(r->aux);
	return 0;
}

static int
rwakeall(MIXP_RENDEZ *r) {
	pthread_cond_broadcast(r->aux);
	return 0;
}

static void
rdestroy(MIXP_RENDEZ *r) {
	pthread_cond_destroy(r->aux);
	free(r->aux);
}

static int
initrendez(MIXP_RENDEZ *r) {
	pthread_cond_t *cond;

	cond = calloc(1,sizeof *cond);
	if(pthread_cond_init(cond, NULL)) {
		free(cond);
		return 1;
	}

	r->aux = cond;
	return 0;
}

static MIXP_THREAD ixp_pthread = {
	/* Mutex */
	.initmutex = initmutex,
	.lock = mlock,
	.canlock = mcanlock,
	.unlock = munlock,
	.mdestroy = mdestroy,
	/* RWLock */
	.initrwlock = initrwlock,
	.rlock = rlock,
	.canrlock = canrlock,
	.wlock = wlock,
	.canwlock = canwlock,
	.runlock = rwunlock,
	.wunlock = rwunlock,
	.rwdestroy = rwdestroy,
	/* Rendez */
	.initrendez = initrendez,
	.sleep = rsleep,
	.wake = rwake,
	.wakeall = rwakeall,
	.rdestroy = rdestroy,
	/* Other */
	.errbuf = errbuf,
	.read = read,
	.write = write,
};

