/* (C)opyright MMIV-MMVI Anselm R. Garbe <garbeam at gmail dot com>
 * Copyright ©2006-2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */

#ifndef __MIXP_THREADING_H
#define __MIXP_THREADING_H

typedef struct MIXP_MUTEX  MIXP_MUTEX;
typedef struct MIXP_RWLOCK MIXP_RWLOCK;
typedef struct MIXP_RENDEZ MIXP_RENDEZ;
typedef struct MIXP_THREAD MIXP_THREAD;

struct MIXP_MUTEX {
	void *aux;
};

struct MIXP_RWLOCK {
	void *aux;
};

struct MIXP_RENDEZ {
	MIXP_MUTEX *mutex;
	void *aux;
};

struct MIXP_THREAD {
	/* RWLock */
	int  (*initrwlock)(MIXP_RWLOCK*);
	void (*rlock)(MIXP_RWLOCK*);
	int  (*canrlock)(MIXP_RWLOCK*);
	void (*runlock)(MIXP_RWLOCK*);
	void (*wlock)(MIXP_RWLOCK*);
	int  (*canwlock)(MIXP_RWLOCK*);
	void (*wunlock)(MIXP_RWLOCK*);
	void (*rwdestroy)(MIXP_RWLOCK*);
	/* Mutex */
	int  (*initmutex)(MIXP_MUTEX*);
	void (*lock)(MIXP_MUTEX*);
	int  (*canlock)(MIXP_MUTEX*);
	void (*unlock)(MIXP_MUTEX*);
	void (*mdestroy)(MIXP_MUTEX*);
	/* Rendez */
	int  (*initrendez)(MIXP_RENDEZ*);
	void (*sleep)(MIXP_RENDEZ*);
	int  (*wake)(MIXP_RENDEZ*);
	int  (*wakeall)(MIXP_RENDEZ*);
	void (*rdestroy)(MIXP_RENDEZ*);
	/* Other */
	char *(*errbuf)(void);
	ssize_t (*read)(int, void*, size_t);
	ssize_t (*write)(int, const void*, size_t);
};

extern MIXP_THREAD *mixp_thread;

#endif
