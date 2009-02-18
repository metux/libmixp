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
//			printf("execmap: [%s] hid=%d id=%d aux=%ld\n", map->name, i, p->id, p->aux);
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

//	printf("mixp_intmap_lookupkey [%s] id=%d\n", map->name, id);
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
//			printf("insertkey [%s] updating id=%d addr=%ld value=%ld\n", map->name, id, elem, value);
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

//	printf("insertkey() [%s] added elem: id=%d addr=%ld value=%ld next=%ld\n",
//	    map->name, id, elem, value, elem->link);	
	return NULL;
}

void * mixp_intmap_insertkey(MIXP_INTMAP *map, unsigned long id, void* value) 
{
//	printf("insertkey [%s] id=%d value=%ld\n", map->name, id, value);

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
    
//	printf("caninsertkey [%s] added: id=%d elem=%ld nextelem=%ld nn=%ld\n", map->name, id, elem, elem->link,map->hash[hid]->link);
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
//		printf("branch %d not existing. nothing to do\n",hid);
		return NULL;
	}
	
	if (elem->id == id)
	{
		/* the first in line is our element */
		ov = elem->aux;
//		printf("deletekey [%s] hid=%d id=%d\n", map->name, hid, id);
//		printf("  -> deleting first in line: %ld (magic %d). next is: %ld\n", elem, elem->magic, elem->link);
		map->hash[hid] = elem->link;
		elem->aux = NULL;
		MIXP_FREE(elem);
//		printf(" --> now first: %ld\n", map->hash[hid]);
		return ov;
	}

//	printf("deletekey [%s] id=%d hid=%d firstelem=%ld magic=%d\n", map->name,id,hid,elem, elem->magic);

	Intlist *next;
	while ((next = elem->link))
	{
//		printf("trying next: %ld\n", next);
//		printf(" -> next->id=%d\n", next->id);

		/* found it somewhere later in line */
		if (next->id == id)
		{
			ov = next->aux;
			elem->link = next->link;
//			printf("deleting somewhere. next is %ld\n", next->link);
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
