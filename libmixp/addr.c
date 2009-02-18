/*
   Plan9 URI address handling. 
   
   Author(s): Enrico Weigelt, metux IT service <weigelt@metux.de>
   
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <9p-mixp/srv_addr.h>
#include <9p-mixp/err.h>
#include "mixp_local.h"

/*

   ninep://localhost:9999/

*/

static MIXP_SERVER_ADDRESS* __parse_uri(const char* addr)
{
    MIXP_SERVER_ADDRESS* as = calloc(1,sizeof(MIXP_SERVER_ADDRESS));
    as->addrstr = strdup(addr);
    as->port    = -1;
    as->proto   = P9_PROTO_TCP;

    char* tmp;
    char buffer[4096];	// FIXME !
    
    if ((tmp=strchr(addr,'/'))!=NULL)
    {
	as->path = strdup(tmp);
	memset(&buffer, 0, sizeof(buffer));
	strncpy(buffer,addr,(tmp-addr));
    }
    else
    {
	as->path=strdup("/");
	strcpy(buffer,addr);
    }

    if ((tmp=strchr(buffer,':'))!=NULL)
    {
	*tmp=0;
	tmp++;
	as->hostname = strdup(buffer);
	as->port = atoi(tmp);
    }
    else
    {
	as->hostname = strdup(buffer);
    }	
	
    return as;
}

static MIXP_SERVER_ADDRESS* __parse_old_tcp(const char* addr)
{
    MIXP_SERVER_ADDRESS * a = calloc(1,sizeof(MIXP_SERVER_ADDRESS));
    char buffer[4096];
    strcpy(buffer,addr);

    char* tmp = strchr(buffer,'!');
    if (tmp)
    {
	*tmp=0;
	tmp++;
	a->port = atoi(tmp);
    }
    else
    {
	a->port = -1;
    }

    a->addrstr  = strdup(addr);
    a->proto    = P9_PROTO_TCP;
    a->hostname = buffer;
    a->path     = strdup("/");

    return a;
}

MIXP_SERVER_ADDRESS* mixp_srv_addr_parse(const char* addr)
{
    if (addr==NULL)
    {
	mixp_werrstr("null address given");
	return NULL;
    }

    if (strlen(addr)>4096)
    {
	mixp_werrstr("address too long");
	return NULL;
    }

    if (strncmp(addr,"9p://",5)==0)
	return __parse_uri(addr+5);
    else if (strncmp(addr,"9p:/",4)==0)
	return __parse_uri(addr+4);
    else if (strncmp(addr,"ninep://",8)==0)
	return __parse_uri(addr+8);
    else if (strncmp(addr,"tcp!",4)==0)
	return __parse_old_tcp(addr+4);

    fprintf(stderr,"could not parse address \"%s\"\n", addr);
    return NULL;
}

int mixp_srv_addr_free(MIXP_SERVER_ADDRESS* addr)
{
    if (addr==NULL)
	return -1;

    if (addr->addrstr)
	free(addr->addrstr);
    if (addr->hostname)
	free(addr->hostname);
    if (addr->key)
	free(addr->key);
    if (addr->path)
	free(addr->path);
    if (addr->username)
	free(addr->username);

    free(addr);
    return 0;
}
