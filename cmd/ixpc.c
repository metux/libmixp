/* Copyright ©2007 Kris Maglione <fbsdaemon@gmail.com>
 * See LICENSE file for license details.
 */
#define IXP_NO_P9_
#define IXP_P9_STRUCTS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <9p-mixp/bits.h>
#include <9p-mixp/mixp.h>
#include <9p-mixp/msgs.h>
#include <9p-mixp/err.h>
#include <9p-mixp/stat.h>
#include <9p-mixp/convert.h>
#include <9p-mixp/client.h>

/* Temporary */
#define fatal(...) if (1) { fprintf(stderr, "ixpc: fatal: " __VA_ARGS__); } \

char *argv0;
#define ARGBEGIN int _argi=0, _argtmp=0, _inargv=0; char *_argv=NULL; \
		if(!argv0)argv0=ARGF(); _inargv=1; \
		while(argc && argv[0][0] == '-') { \
			_argi=1; _argv=*argv++; argc--; \
			while(_argv[_argi]) switch(_argv[_argi++])
#define ARGEND }_inargv=0;USED(_argtmp);USED(_argv);USED(_argi)
#define ARGF() ((_inargv && _argv[_argi]) ? \
		(_argtmp=_argi, _argi=strlen(_argv), _argv+_argtmp) \
		: ((argc > 0) ? (argc--, *argv++) : ((char*)0)))
#define EARGF(f) ((_inargv && _argv[_argi]) ? \
		(_argtmp=_argi, _argi=strlen(_argv), _argv+_argtmp) \
		: ((argc > 0) ? (argc--, *argv++) : ((f), (char*)0)))
#define USED(x) if(x){}else
#define SET(x) ((x)=0)

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
	time_t tv = val;

	ctime_r(&tv, buf);
	buf[strlen(buf) - 1] = '\0';
	return buf;
}

static void
print_stat(MIXP_STAT *s, int details) {
	if(details)
		fprintf(stdout, "%s %s %s %5llu %s %s\n", str_of_mode(s->mode),
				s->uid, s->gid, (long long unsigned int)s->length, str_of_time(s->mtime), s->name);
	else {
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
	if(mixp_remove(client, file) == 0)
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
	int lflag, dflag, count, nstat, mstat, i;

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

	stat = mixp_stat(client, file);
	if(stat == NULL)
		fatal("cannot stat file '%s': %s\n", file, mixp_errbuf());

	if(dflag || (stat->mode&P9_DMDIR) == 0) {
		print_stat(stat, lflag);
		mixp_stat_free(stat);
		return 0;
	}
	mixp_stat_free(stat);

	fid = mixp_open(client, file, P9_OREAD);
	if(fid == NULL)
		fatal("Can't open file '%s': %s\n", file, mixp_errbuf());

	nstat = 0;
	mstat = 16;
	stat = malloc(sizeof(*stat) * mstat);
	buf = malloc(fid->iounit);
	while((count = mixp_read(fid, buf, fid->iounit)) > 0) {
		m = mixp_message(buf, count, MsgUnpack);
		while(m.pos < m.end) {
			if(nstat == mstat) {
				mstat <<= 1;
				stat = ixp_erealloc(stat, sizeof(*stat) * mstat);
			}
			mixp_pstat(&m, &stat[nstat++]);
		}
	}

	qsort(stat, nstat, sizeof(*stat), comp_stat);
	for(i = 0; i < nstat; i++) {
		print_stat(&stat[i], lflag);
		mixp_stat_free(&stat[i]);
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

	address = getenv("IXP_ADDRESS");

	ARGBEGIN{
	case 'v':
		printf("%s-" VERSION ", (C) 2007 Kris Maglione, 2008 Enrico Weigelt <weigelt@metux.de>\n", argv0);
		exit(0);
	case 'd':
		mixp_dump = 1;
		break;
	case 'a':
		address = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	cmd = EARGF(usage());

	if(!address)
		fatal("$IXP_ADDRESS not set\n");
	
	MIXP_SERVER_ADDRESS* addr = mixp_srv_addr_parse(address);
	if (!addr)
		fatal("Could not parse address\n");

#ifdef DEBUG
	fprintf(stderr, "addr.hostname=\"%s\"\n", addr->hostname);
	fprintf(stderr, "addr.port=\"%d\"\n", addr->port);
	fprintf(stderr, "addr.path=\"%s\"\n", addr->path);
	fprintf(stderr, "addr.proto=\"%d\"\n", addr->proto);
#endif

	client = mixp_mount_addr(addr);
	
	if (!client)
		fatal("Could not mount: %s\n", mixp_errbuf());

	for(tab = etab; tab->cmd; tab++)
		if(strcmp(cmd, tab->cmd) == 0) break;
	if(tab->cmd == 0)
		usage();

	ret = tab->fn(argc, argv);

	mixp_unmount(client);
	return ret;
}
