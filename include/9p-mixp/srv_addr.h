/*
	Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
*/

#ifndef __MIXP_SRV_ADDR_H
#define __MIXP_SRV_ADDR_H

typedef struct MIXP_SERVER_ADDRESS MIXP_SERVER_ADDRESS;

typedef enum
{
	P9_PROTO_TCP	= 1,
	P9_PROTO_UNIX	= 2
} MIXP_PROTO_ID;

struct MIXP_SERVER_ADDRESS
{
	MIXP_PROTO_ID	proto;
	int		port;
	char*		addrstr;	// the unparsed addr string
	char*		hostname;
	char*		username;
	char*		key;
	char*		path;
};

MIXP_SERVER_ADDRESS* mixp_srv_addr_parse(const char* address);
int                  mixp_srv_addr_free(MIXP_SERVER_ADDRESS* addr);

#endif
