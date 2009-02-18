/* Copyright ©2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <9p-mixp/mixp.h>
#include "util.h"

static MIXP_CLIENT *client;

static void
usage() {
	fprintf(stderr,
		   "usage: %1$s [-a <address>] {create | read | ls [-ld] | remove | write} <file>\n"
		   "       %1$s [-a <address>] xwrite <file> <data>\n"
		   "       %1$s -v\n", argv0);
	exit(1);
}

/* Utility Functions */
static void
write_data(MIXP_CFID *fid, char *name) {
	void *buf;
	unsigned int len;

	buf = malloc(fid->iounit);;
	do {
		len = read(0, buf, fid->iounit);
		if(len >= 0 && mixp_write(fid, buf, len) != len)
			fatal("cannot write file '%s': %s\n", name, mixp_errbuf());
	} while(len > 0);

	free(buf);
}

static int
comp_stat(const void *s1, const void *s2) {
	MIXP_STAT *st1, *st2;

	st1 = (MIXP_STAT*)s1;
	st2 = (MIXP_STAT*)s2;
	
	fprintf(stderr,"entering comp_stat()\n");
	if (s1==NULL)
	{
	    fprintf(stderr,"comp_stat() s1==NULL!\n");
	    return 0;
	}
	if (s2==NULL)
	{
	    fprintf(stderr,"comp_stat() s2==NULL\n");
	    return 0;
	}
	if (st1->name==NULL)
	{
	    fprintf(stderr,"comp_stat() s1->name==NULL\n");
	    return 0;
	}
	if (st2->name==NULL)
	{
	    fprintf(stderr,"comp_stat() s2->name==NULL\n");
	    return 0;
	}
	
	fprintf(stderr,"comp_stat() XXX\n");
	fprintf(stderr,"comp_stat() s1->name=\"%s\"\n", st1->name);
	fprintf(stderr,"comp_stat() s2->name=\"%s\"\n", st2->name);
	return strcmp(st1->name, st2->name);
}

static void
setrwx(long m, char *s) {
	static char *modes[] = {
		"---", "--x", "-w-",
		"-wx", "r--", "r-x",
		"rw-", "rwx",
	};
	strncpy(s, modes[m], 3);
}

static char *
str_of_mode(unsigned int mode) {
	static char buf[16];

	buf[0]='-';
	if(mode & P9_DMDIR)
		buf[0]='d';
	buf[1]='-';
	setrwx((mode >> 6) & 7, &buf[2]);
	setrwx((mode >> 3) & 7, &buf[5]);
	setrwx((mode >> 0) & 7, &buf[8]);
	buf[11] = 0;
	return buf;
}

static char *
str_of_time(unsigned int val) {
	static char buf[32];

	ctime_r((time_t*)&val, buf);
	buf[strlen(buf) - 1] = '\0';
	return buf;
}

static void
print_stat(MIXP_STAT *s, int lflag) {
	if (s==NULL)
	{
	    fprintf(stderr,"print_stat() s==NULL\n");
	    return;
	}

	if(lflag)
	{
		fprintf(stderr," LFLAG\n");
		fprintf(stdout, "%s %s %s %5llu %s %s\n", str_of_mode(s->mode),
				s->uid, s->gid, s->length, str_of_time(s->mtime), s->name);
	}				
	else {
		fprintf(stderr,"NORMAL: %s\n", s->name);
		if((s->mode&P9_DMDIR) && strcmp(s->name, "/"))
			fprintf(stdout, "%s/\n", s->name);
		else
			fprintf(stdout, "%s\n", s->name);
	}
}

/* Service Functions */
static int
xwrite(int argc, char *argv[]) {
	MIXP_CFID *fid;
	char *file;

	ARGBEGIN{
	default:
		usage();
	}ARGEND;

	file = EARGF(usage());
	fid = mixp_open(client, file, P9_OWRITE);
	if(fid == NULL)
		fatal("Can't open file '%s': %s\n", file, mixp_errbuf());

	write_data(fid, file);
	return 0;
}

static int
xawrite(int argc, char *argv[]) {
	MIXP_CFID *fid;
	char *file, *buf, *arg;
	int nbuf, mbuf, len;

	ARGBEGIN{
	default:
		usage();
	}ARGEND;

	file = EARGF(usage());
	fid = mixp_open(client, file, P9_OWRITE);
	if(fid == NULL)
		fatal("Can't open file '%s': %s\n", file, mixp_errbuf());

	nbuf = 0;
	mbuf = 128;
	buf = malloc(mbuf);
	while(argc) {
		arg = ARGF();
		len = strlen(arg);
		if(nbuf + len > mbuf) {
			mbuf <<= 1;
			buf = ixp_erealloc(buf, mbuf);
		}
		memcpy(buf+nbuf, arg, len);
		nbuf += len;
		if(argc)
			buf[nbuf++] = ' ';
	}

	if(mixp_write(fid, buf, nbuf) == -1)
		fatal("cannot write file '%s': %s\n", file, mixp_errbuf());
	return 0;
}

static int
xcreate(int argc, char *argv[]) {
	MIXP_CFID *fid;
	char *file;

	ARGBEGIN{
	default:
		usage();
	}ARGEND;

	file = EARGF(usage());
	fid = mixp_create(client, file, 0777, P9_OWRITE);
	if(fid == NULL)
		fatal("Can't create file '%s': %s\n", file, mixp_errbuf());

	if((fid->qid.type&P9_DMDIR) == 0)
		write_data(fid, file);

	return 0;
}

static int
xremove(int argc, char *argv[]) {
	char *file;

	ARGBEGIN{
	default:
		usage();
	}ARGEND;

	file = EARGF(usage());
	if(ixp_remove(client, file) == 0)
		fatal("Can't remove file '%s': %s\n", file, mixp_errbuf());
	return 0;
}

static int
xread(int argc, char *argv[]) {
	MIXP_CFID *fid;
	char *file, *buf;
	int count;

	ARGBEGIN{
	default:
		usage();
	}ARGEND;

	file = EARGF(usage());
	fid = mixp_open(client, file, P9_OREAD);

	if(fid == NULL)
		fatal("Can't open file '%s': %s\n", file, mixp_errbuf());

	buf = malloc(fid->iounit);
	while((count = mixp_read(fid, buf, fid->iounit)) > 0)
		write(1, buf, count);

	if(count == -1)
		fatal("cannot read file/directory '%s': %s\n", file, mixp_errbuf());

	return 0;
}

static int
xls(int argc, char *argv[]) {
	MIXP_MESSAGE m;
	MIXP_STAT *stat;
	MIXP_CFID *fid;
	char *file, *buf;
	int lflag, dflag, nstat, mstat, i;
	unsigned int count;

	lflag = dflag = 0;

	ARGBEGIN{
	case 'l':
		lflag++;
		break;
	case 'd':
		dflag++;
		break;
	default:
		usage();
	}ARGEND;

	file = EARGF(usage());

	printf("0\n");
	
	stat = ixp_stat(client, file);
	if(stat == NULL)
		fatal("cannot stat file '%s': %s\n", file, mixp_errbuf());

	if(dflag || (stat->mode&P9_DMDIR) == 0) {
		print_stat(stat, lflag);
		mixp_stat_free(stat);
		return 0;
	}
	mixp_stat_free(stat);

	printf("1\n");
	fid = mixp_open(client, file, P9_OREAD);
	printf("2\n");
	if(fid == NULL)
		fatal("Can't open file '%s': %s\n", file, mixp_errbuf());

	nstat = 0;
	mstat = 1600;
	stat = malloc(sizeof(*stat) * mstat);
	buf = malloc(fid->iounit);
	printf("3\n");
	while((count = mixp_read(fid, buf, fid->iounit)) > 0) {
		m = ixp_message((unsigned char*)buf, count, MsgUnpack);
		while(m.pos < m.end) {
			if(nstat == mstat) {
				mstat <<= 1;
				stat = ixp_erealloc(stat, mstat);
				fprintf(stderr,"RELOCATE\n");
			}
			fprintf(stderr,"id=%d stat->name=%s -> uid=%s\n", nstat-1, stat[nstat-1].name, stat[nstat-1].uid);
			ixp_pstat(&m, &stat[nstat++]);
			fprintf(stderr,"id=%d stat->name=%s -> uid=%s\n", nstat-1, stat[nstat-1].name, stat[nstat-1].uid);
		}
	}
	
	fprintf(stderr,"YYY id=%d stat->name=%s -> uid=%s\n", nstat-1, stat[nstat-1].name, stat[nstat-1].uid);
//	fprintf(stderr,"AAA id=0 stat->name=%s -> uid=%s\n", 0, stat[1].name, stat[0].uid);
//	qsort(stat, nstat, sizeof(*stat), comp_stat);
	for(i = 0; i < nstat; i++) {
		fprintf(stderr,"1111 --> %s\n",stat[i].name);
//		print_stat(&stat[i], lflag);
		fprintf(stderr,"0000\n");
//		mixp_stat_free(&stat[i]);
	}
	free(stat);

	if(count == -1)
		fatal("cannot read directory '%s': %s\n", file, mixp_errbuf());
	return 0;
}

typedef struct exectab exectab;
struct exectab {
	char *cmd;
	int (*fn)(int, char**);
} etab[] = {
	{"write", xwrite},
	{"xwrite", xawrite},
	{"read", xread},
	{"create", xcreate},
	{"remove", xremove},
	{"ls", xls},
	{0, 0}
};

int
main(int argc, char *argv[]) {
	char *cmd, *address;
	exectab *tab;
	int ret;

	address = getenv("WMII_ADDRESS");

	ARGBEGIN{
	case 'v':
		printf("%s-" VERSION ", ©2007 Kris Maglione\n", argv0);
		exit(0);
	case 'a':
		address = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	cmd = EARGF(usage());

	if(!address)
		fatal("$WMII_ADDRESS not set\n");

	client = mixp_mount(address);
	if(client == NULL)
		fatal("%s\n", mixp_errbuf());
	
	for(tab = etab; tab->cmd; tab++)
		if(strcmp(cmd, tab->cmd) == 0) break;
	if(tab->cmd == 0)
		usage();

	ret = tab->fn(argc, argv);

	mixp_unmount(client);
	return ret;
}
