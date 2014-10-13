/* This file is derived from src/lib9p/intmap.c from plan9port */
/* See LICENCE.p9p for terms of use */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <9p-mixp/intmap.h>

#include "mixp_local.h"
#include "util.h"

struct Intlist {
	unsigned long		id;
	void*		aux;
	Intlist*	link;
	int             magic;
};

static unsigned long
hashid(MIXP_INTMAP *map, unsigned long id) {
	return id%map->nhash;
}

static void
nop(void *v) {
	USED(v);
}

void mixp_intmap_init(MIXP_INTMAP *m, unsigned long nhash, void *hash, const char* name) 
{
	m->nhash = nhash;
	m->hash = hash;
	m->name = name;
	memset(hash,0,sizeof(void*)*nhash);
	mixp_thread->initrwlock(&m->lk);
}

void mixp_intmap_free(MIXP_INTMAP *map, void (*destroy)(void*)) 
{
	int i;
	Intlist *p, *nlink;

	if(destroy == NULL)
		destroy = nop;
	for(i=0; i<map->nhash; i++)
	{
		for(p=map->hash[i]; p; p=nlink)
		{
			nlink = p->link;
			destroy(p->aux);
			MIXP_FREE(p);
		}
	}

	mixp_thread->rwdestroy(&map->lk);
}

void mixp_intmap_exec(MIXP_INTMAP *map, void (*run)(void*)) 
{
	int i;
	Intlist *p, *nlink;

	mixp_thread->rlock(&map->lk);
	for(i=0; i<map->nhash; i++)
	{
		for(p=map->hash[i]; p; p=nlink)
		{
			mixp_thread->runlock(&map->lk);
			nlink = p->link;
#ifdef DEBUG
			fprintf(mixp_debug_stream,"execmap: [%s] hid=%ld id=%ld aux=%ld\n", map->name, (long)i, (long)p->id, (long)p->aux);
#endif
			run(p->aux);
			mixp_thread->rlock(&map->lk);
		}
	}
	mixp_thread->runlock(&map->lk);
}

static void * __real_lookupkey(MIXP_INTMAP* map, unsigned long id)
{
	int hid=hashid(map,id);
	
	Intlist * elem = map->hash[hid];
	while (elem)
	{
		if (elem->id == id)
			return elem->aux;
		elem = elem->link;
	}
	
	return NULL;
}

void * mixp_intmap_lookupkey(MIXP_INTMAP *map, unsigned long id) 
{
	void *v;

#ifdef DEBUG
	fprintf(mixp_debug_stream,"mixp_intmap_lookupkey [%s] id=%ld\n", map->name, (long)id);
#endif
	mixp_thread->rlock(&map->lk);
	v = __real_lookupkey(map,id);
	mixp_thread->runlock(&map->lk);
	return v;
}

static void * __real_insertkey(MIXP_INTMAP* map, unsigned long id, void* value)
{
	int hid = hashid(map,id);
	void* ov = NULL;
	Intlist* elem = map->hash[hid];

	while (elem)
	{
		if (elem->id==id)
		{
#ifdef DEBUG
			fprintf(mixp_debug_stream,"insertkey [%s] updating id=%ld addr=%ld value=%ld\n", map->name, (long)id, (long)elem, (long)value);
#endif
			ov = elem->aux;
			elem->aux=value;
			return ov;
		}
	}

	elem = calloc(1,sizeof(Intlist));
	elem->id   = id;
	elem->aux  = value;
	elem->link = map->hash[hid];
	elem->magic = 666777;
	map->hash[hid] = elem;

#ifdef DEBUG
	fprintf(mixp_debug_stream,"insertkey() [%s] added elem: id=%ld addr=%ld value=%ld next=%ld\n",
	    map->name, (long)id, (long)elem, (long)value, (long)elem->link);
#endif

	return NULL;
}

void * mixp_intmap_insertkey(MIXP_INTMAP *map, unsigned long id, void* value) 
{
#ifdef DEBUG
	fprintf(mixp_debug_stream,"insertkey [%s] id=%ld value=%ld\n", map->name, (long)id, (long)value);
#endif

	void* ov;
	mixp_thread->wlock(&map->lk);
	ov = __real_insertkey(map, id, value);
	mixp_thread->wunlock(&map->lk);
	return ov;	
}

static int __real_caninsertkey(MIXP_INTMAP* map, unsigned long id, void* value)
{
	int hid = hashid(map,id);
	Intlist* elem=map->hash[hid];

	while(elem)
	{	
		if (elem->id==id)
			return 0;
		elem = elem->link;
	}

	elem = calloc(1,sizeof(Intlist));
	elem->id       = id;
	elem->aux      = value;
	elem->link     = map->hash[hid];
	elem->magic    = 777666;
	map->hash[hid] = elem;

#ifdef DEBUG
	fprintf(mixp_debug_stream,"caninsertkey [%s] added: id=%ld elem=%ld nextelem=%ld nn=%ld\n", map->name, (long)id, (long)elem, (long)elem->link, (long)map->hash[hid]->link);
#endif
	return 1;
}

int mixp_intmap_caninsertkey(MIXP_INTMAP *map, unsigned long id, void *v) 
{
	mixp_thread->wlock(&map->lk);
	int rv = __real_caninsertkey(map,id,v);
	mixp_thread->wunlock(&map->lk);
	return rv;
}

static void* __real_deletekey(MIXP_INTMAP* map, unsigned long id)
{
	void *ov = NULL;
	Intlist* elem;

	int hid = hashid(map,id);

	if (!(elem = map->hash[hid]))
	{
#ifdef DEBUG
		fprintf(mixp_debug_stream,"branch %ld not existing. nothing to do\n", (long)hid);
#endif
		return NULL;
	}
	
	if (elem->id == id)
	{
		/* the first in line is our element */
		ov = elem->aux;
#ifdef DEBUG
		fprintf(mixp_debug_stream,"deletekey [%s] hid=%ld id=%ld\n", map->name, (long)hid, (long)id);
		fprintf(mixp_debug_stream,"  -> deleting first in line: %ld (magic %ld). next is: %ld\n", (long)elem, (long)elem->magic, (long)elem->link);
#endif
		map->hash[hid] = elem->link;
		elem->aux = NULL;
		MIXP_FREE(elem);
#ifdef DEBUG
		fprintf(mixp_debug_stream," --> now first: %ld\n", (long)map->hash[hid]);
#endif
		return ov;
	}

#ifdef DEBUG
	fprintf(mixp_debug_stream,"deletekey [%s] id=%ld hid=%ld firstelem=%ld magic=%ld\n", map->name, (long)id, (long)hid, (long)elem, (long)elem->magic);
#endif

	Intlist *next;
	while ((next = elem->link))
	{
#ifdef DEBUG
		fprintf(mixp_debug_stream,"trying next: %ld\n", (long)next);
		fprintf(mixp_debug_stream," -> next->id=%ld\n", (long)next->id);
#endif
		/* found it somewhere later in line */
		if (next->id == id)
		{
			ov = next->aux;
			elem->link = next->link;
#ifdef DEBUG
			fprintf(mixp_debug_stream,"deleting somewhere. next is %ld\n", (long)next->link);
#endif
			next->aux = NULL;
			MIXP_FREE(next);
		}
	}
	return ov;
}

void* mixp_intmap_deletekey(MIXP_INTMAP *map, unsigned long id) 
{
	void* ov;
	mixp_thread->wlock(&map->lk);
	ov = __real_deletekey(map,id);
	mixp_thread->wunlock(&map->lk);
	return ov;
}
