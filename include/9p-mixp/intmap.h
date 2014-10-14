
#ifndef __IXP_LOCAL_INTMAP_H
#define __IXP_LOCAL_INTMAP_H

#include <9p-mixp/threading.h>

typedef struct MIXP_INTLIST MIXP_INTLIST;
typedef struct {
	unsigned long nhash;
	MIXP_INTLIST	**hash;
	MIXP_RWLOCK lk;
	const char* name;
} MIXP_INTMAP;

void  mixp_intmap_init(MIXP_INTMAP *m, unsigned long nhash, void *hash, const char* name);
void  mixp_intmap_unref(MIXP_INTMAP *m);
void  mixp_intmap_ref(MIXP_INTMAP *m);
void  mixp_intmap_free(MIXP_INTMAP *map, void (*destroy)(void*));
void  mixp_intmap_exec(MIXP_INTMAP *map, void (*destroy)(void*));
void* mixp_intmap_lookupkey(MIXP_INTMAP *map, unsigned long id);
void* mixp_intmap_insertkey(MIXP_INTMAP *map, unsigned long id, void *v);
void* mixp_intmap_deletekey(MIXP_INTMAP *map, unsigned long id);
int   mixp_intmap_caninsertkey(MIXP_INTMAP *map, unsigned long id, void *v);

#endif
