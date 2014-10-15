
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <9p-mixp/stat.h>

#include "mixp_local.h"
#include "util.h"

void mixp_stat_clear(MIXP_STAT *st) {
	if (st == NULL)
		return;

	MIXP_FREE(st->name);
	MIXP_FREE(st->uid);
	MIXP_FREE(st->gid);
	MIXP_FREE(st->muid);
}

void mixp_stat_free(MIXP_STAT *st) {
	if (st == NULL)
		return;

	mixp_stat_clear(st);
	MIXP_FREE(st);
}
