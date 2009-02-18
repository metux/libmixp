/* Written by Kris Maglione <fbsdaemon at gmail dot com> */
/* Public domain */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include "mixp_local.h"

void
ixp_eprint(const char *fmt, ...) {
	va_list ap;
	int err;

	err = errno;
	fprintf(stderr, "libixp: fatal: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if(fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s\n", strerror(err));
	else
		fprintf(stderr, "\n");

	exit(1);
}

/* Can't malloc */
static void
ixp_mfatal(char *name, unsigned int size) {
	const char
		couldnot[] = "libixp: fatal: Could not ",
		paren[] = "() ",
		bytes[] = " bytes\n";
	char sizestr[8];
	int i;
	
	i = sizeof(sizestr);
	do {
		sizestr[--i] = '0' + (size%10);
		size /= 10;
	} while(size > 0);

	write(1, couldnot, sizeof(couldnot)-1);
	write(1, name, strlen(name));
	write(1, paren, sizeof(paren)-1);
	write(1, sizestr+i, sizeof(sizestr)-i);
	write(1, bytes, sizeof(bytes)-1);

	exit(1);
}

void *
ixp_emalloc(unsigned int size) {
	void *ret = malloc(size);
	if(!ret)
		ixp_mfatal("malloc", size);
	return ret;
}

void *
ixp_erealloc(void *ptr, unsigned int size) {
	void *ret = realloc(ptr, size);
	if(!ret)
		ixp_mfatal("realloc", size);
	return ret;
}

unsigned int ixp_tokenize(char *res[], unsigned int reslen, char *str, char delim) {
	char *s;
	unsigned int i;

	i = 0;
	s = str;
	while(i < reslen && *s) {
		while(*s == delim)
			*(s++) = '\0';
		if(*s)
			res[i++] = s;
		while(*s && *s != delim)
			s++;
	}
	return i;
}
