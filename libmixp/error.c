#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <9p-mixp/err.h>
#include "mixp_local.h"

/* Approach to errno handling taken from Plan 9 Port. */
enum {
	EPLAN9 = 0x19283745,
};

FILE* mixp_error_stream = NULL;
FILE* mixp_debug_stream = NULL;

const char*
mixp_errbuf() {
	char *errbuf;

	errbuf = mixp_thread->errbuf();
	if(errno == EINTR)
		strncpy(errbuf, "interrupted", IXP_ERRMAX);
	else if(errno != EPLAN9)
		strncpy(errbuf, strerror(errno), IXP_ERRMAX);
	return errbuf;
}

void
mixp_errstr(char *buf, int n) {
	char tmp[IXP_ERRMAX];

	strncpy(tmp, buf, sizeof(tmp));
	mixp_rerrstr(buf, n);
	strncpy(mixp_thread->errbuf(), tmp, IXP_ERRMAX);
	errno = EPLAN9;
}

void
mixp_rerrstr(char *buf, int n) {
	strncpy(buf, mixp_errbuf(), n);
}

void
mixp_werrstr(char *fmt, ...) {
	char tmp[IXP_ERRMAX];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);
	strncpy(mixp_thread->errbuf(), tmp, IXP_ERRMAX);
	errno = EPLAN9;
}

