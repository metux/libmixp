
#include <stdio.h>
#include <string.h>
#include <9p-mixp/srv_addr.h>

void dumpaddr(MIXP_SERVER_ADDRESS* a, const char* prefix, const char* uri)
{
	if (!a)
	{
		printf("%s FAIL URI = %s\n", prefix, uri);
		return;
	}

	switch (a->proto)
	{
		case P9_PROTO_TCP:
			printf("%s proto    = TCP\n", prefix);
		break;
		case P9_PROTO_UNIX:
			printf("%s proto    = UNIX\n", prefix);
		break;
		default:
			printf("%s proto    = %d\n", prefix, a->proto);
	}

	printf("%s addrstr  = %s\n", prefix, a->addrstr);
	printf("%s port     = %d\n", prefix, a->port);
	printf("%s hostname = %s\n", prefix, a->hostname);
	if (a->username)
		printf("%s username = %s\n", prefix, a->username);
	if (a->key)
		printf("%s key      = %s\n", prefix, a->key);
	printf("%s path     = %s\n", prefix, a->path);
}

int test_parse_tcp(const char* str, const char* want_server, int want_port, const char* want_path)
{
	MIXP_SERVER_ADDRESS* addr = mixp_srv_addr_parse(str);

	printf("[TCP] Testing address: %s\n", str);

	if (addr==NULL)
	{
		fprintf(stderr,"[test-tcp] could not parse: %s\n", str);
		return 0;
	}

	dumpaddr(addr, "", str);

	if (addr->proto != P9_PROTO_TCP)
		fprintf(stderr,"[test-tcp] wrong proto: %d\n", addr->proto);

	if (strcmp(addr->hostname, want_server)!=0)
		fprintf(stderr,"[test-tcp] wrong hostname - should be \"%s\"\n", want_server);

	if (addr->port != want_port)
		fprintf(stderr,"[test-tcp] wrong port - should be %d\n", want_port);

	if (strcmp(addr->path, want_path)!=0)
		fprintf(stderr,"[test-tcp] wrong path - should be \"%s\"\n", want_path);

	printf("\n");
	return 1;
}

int test_parse_unix(const char* url, const char* test_path)
{
	MIXP_SERVER_ADDRESS* addr = mixp_srv_addr_parse(url);

	printf("[UNIX] Testing address: %s\n", url);

	if (addr==NULL)
	{
		fprintf(stderr,"[test unix] could not parse: %s\n", url);
		return 0;
	}

	dumpaddr(addr, "", url);

	if (addr->proto != P9_PROTO_UNIX)
	fprintf(stderr,"[test unix] wrong proto: %d\n", addr->proto);

	if (strcmp(addr->path, test_path)!=0)
		fprintf(stderr,"[test unix] pathname mismatch - should be \"%s\"\n", test_path);

	if (addr->port != -1)
		fprintf(stderr,"[test unix] wrong port - should be -1\n");

	if (strlen(addr->hostname))
		fprintf(stderr,"[test unix] wrong hostname - should be \"\"");

	printf("\n");

	return 1;
}

int main()
{
	printf("\n");
	test_parse_tcp  ("9p://server:1234/prefix",     "server",        1234, "/prefix");
	test_parse_tcp  ("9p:/server:99",               "server",          99, "/");
	test_parse_tcp  ("9p:tcp://srv:0023/path/name", "srv",             23, "/path/name");
	test_parse_unix ("9p:unix://my/local/path",     "/my/local/path");
	test_parse_unix ("9p:unix:/tmp/foo/bar",        "/tmp/foo/bar");
	return 0;
}
