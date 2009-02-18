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

static MIXP_SERVER_ADDRESS* __parse_uri(const char* addr, const char* fulladdr, MIXP_PROTO_ID proto)
{
    MIXP_SERVER_ADDRESS* as = calloc(1,sizeof(MIXP_SERVER_ADDRESS));
    as->addrstr = strdup(fulladdr);
    as->port    = -1;
    as->proto   = proto;

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

static MIXP_SERVER_ADDRESS* __parse_uri_unix(const char* addr, const char* fulladdr, MIXP_PROTO_ID proto)
{
    MIXP_SERVER_ADDRESS* as = calloc(1,sizeof(MIXP_SERVER_ADDRESS));
    as->addrstr = strdup(fulladdr);
    as->port    = -1;
    as->proto   = proto;

    if (addr[0]=='/')
	while (addr[1]=='/')
	    addr++;

    as->path     = strdup(addr);
    as->hostname = strdup("");
    
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

static inline const char* cmpprefix(const char* str, const char* prefix)
{
    int sz = strlen(prefix);
    if (strncmp(str,prefix,sz)==0)
	return str+sz;
    else
	return NULL;
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

    const char* c;

    // --- try different URI schemes ---
        
    // simple 9p:// scheme for implicit tcp
    if ((c=cmpprefix(addr,"9p://")))
	return __parse_uri(c, addr, P9_PROTO_TCP);

    // explicit tcp
    if ((c=cmpprefix(addr,"9p:tcp://")))
	return __parse_uri(c, addr, P9_PROTO_TCP);
    if ((c=cmpprefix(addr,"9p:tcp:/")))
	return __parse_uri(c, addr, P9_PROTO_TCP);
	
    // explicit unix
    if (strncmp(addr,"9p:unix:/",9)==0)
	return __parse_uri_unix(addr+8, addr, P9_PROTO_UNIX);

    // shortcut 9p:/ scheme - implicit unix
    if ((c=cmpprefix(addr,"9p:/")))
	return __parse_uri(c, addr, P9_PROTO_TCP);

    if ((c=cmpprefix(addr,"ninep://")))
	return __parse_uri(c, addr, P9_PROTO_TCP);

    if (strncmp(addr,"tcp!",4)==0)
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
