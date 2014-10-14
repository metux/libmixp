
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <9p-mixp/mixp.h>
#include <9p-mixp/convert.h>
#include <9p-mixp/stat.h>
#include "mixp_local.h"

// #define _DEBUG

enum {
	SByte = 1,
	SWord = 2,
	SDWord = 4,
	SQWord = 8,
};

/*
void
mixp_puint(MIXP_MESSAGE *msg, unsigned int size, unsigned int *val) {
	int v;

	if(msg->pos + size <= msg->end) {
		switch(msg->mode) {
		case MsgPack:
			v = *val;
			unsigned short int b;
			printf("pack int: v=%d\n", v);
			switch(size) {
			case SDWord:
				msg->pos[3] = v>>24;
				msg->pos[2] = v>>16;
				b = msg->pos[3];
				printf("pack int: SDW[3]=%d\n", b);
				b = msg->pos[2];
				printf("pack int: SDW[2]=%d\n", b);
			case SWord:
				msg->pos[1] = v>>8;
				b = msg->pos[1];
				printf("pack int: SDW[1]=%d\n", b);
			case SByte:
				msg->pos[0] = v;
				b = msg->pos[0];
				printf("pack int: SDW[0]=%d\n", b);
				break;
			default:
				printf("mixp_puint32: unhandled int size: %d\n", size);
			}
		break;
		case MsgUnpack:
			v = 0;
			unsigned short int a;
			switch(size) {
			case SDWord:
				a = msg->pos[3];
				printf("SDW pos[3]=%d\n", a);
				a = msg->pos[2];
				printf("SDW pos[2]=%d\n", a);
				v |= msg->pos[3]<<24;
				v |= msg->pos[2]<<16;
			case SWord:
				a = msg->pos[1];
				printf("SW pos[1]=%d\n", a);
				v |= msg->pos[1]<<8;
			case SByte:
				a = msg->pos[0];
				printf("B pos[0]=%d\n", a);
				v |= msg->pos[0];
				break;
			}
			*val = v;
			printf("unpack int: %d\n", v);
		}
	}
	msg->pos += size;
}
*/

void mixp_pu8_store(MIXP_MESSAGE* msg, uint8_t val)
{
	uint8_t v;

	int size = SByte;
	if(msg->pos + size <= msg->end) 
	{
	    v = val;
	    unsigned char p0 = v;
	    unsigned char* packtext = (unsigned char*)msg->pos;

	    packtext[0] = p0;
#ifdef _DEBUG
	    fprintf(mixp_debug_stream, "mixp_pu8_store() val=%d text=%02X\n",
		v,
		packtext[0]);
#endif
	}
	msg->pos += size;
}

void mixp_pu32_store(MIXP_MESSAGE* msg, uint32_t val)
{
	uint32_t v;

	int size = SDWord;
	if(msg->pos + size <= msg->end) 
	{
	    v = val;
	    unsigned char p3 = v>>24;
	    unsigned char p2 = v>>16;
	    unsigned char p1 = v>>8;
	    unsigned char p0 = v;
	    unsigned char* packtext = (unsigned char*)msg->pos;

	    packtext[3] = p3;
	    packtext[2] = p2;
	    packtext[1] = p1;
	    packtext[0] = p0;
#ifdef _DEBUG
	    fprintf(mixp_debug_stream, "mixp_pu32_store() val=%d text=%02X:%02X:%02X:%02X\n",
		v,
		packtext[0],
		packtext[1],
		packtext[2],
		packtext[3]);
#endif
	}
	msg->pos += size;
}

void mixp_pu64_store(MIXP_MESSAGE* msg, uint64_t val)
{
	uint64_t v;

	int size = SDWord*2;
	if(msg->pos + size <= msg->end) 
	{
	    v = val;
	    
	    unsigned char p7 = v>>56;
	    unsigned char p6 = v>>48;
	    unsigned char p5 = v>>40;
	    unsigned char p4 = v>>32;
	    unsigned char p3 = v>>24;
	    unsigned char p2 = v>>16;
	    unsigned char p1 = v>>8;
	    unsigned char p0 = v;
	    unsigned char* packtext = (unsigned char*)msg->pos;

	    packtext[7] = p7;
	    packtext[6] = p6;
	    packtext[5] = p5;
	    packtext[4] = p4;
	    packtext[3] = p3;
	    packtext[2] = p2;
	    packtext[1] = p1;
	    packtext[0] = p0;
#ifdef _DEBUG
	    fprintf(mixp_debug_stream,"mixp_pu64_store() val=%llu text=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
		(long long unsigned int)v,
		packtext[0],
		packtext[1],
		packtext[2],
		packtext[3],
		packtext[4],
		packtext[5],
		packtext[6],
		packtext[7]
	    );
#endif
	}
	msg->pos += size;
}

void mixp_pu16_store(MIXP_MESSAGE* msg, uint16_t val)
{
	uint16_t v;

	if(msg->pos + SWord <= msg->end) 
	{
	    v = val;
	    unsigned char p1 = v>>8;
	    unsigned char p0 = v;
	    unsigned char* packtext = (unsigned char*)msg->pos;

	    packtext[1] = p1;
	    packtext[0] = p0;
#ifdef _DEBUG
	    fprintf(mixp_debug_stream, "mixp_pu16_store() val=%d text=%02X:%02X\n",
		v,
		packtext[1],
		packtext[0]);
#endif
	}
	msg->pos += SWord;
}

uint8_t mixp_pu8_load(MIXP_MESSAGE* msg)
{
	uint8_t v = 0;
	if(msg->pos + SByte <= msg->end) 
	{
	    unsigned char* packtext = (unsigned char*)msg->pos;
	    v |= packtext[0];
#ifdef _DEBUG
	    fprintf(mixp_debug_stream, "mixp_pu8_load() val=%d text=%02X\n", v, packtext[0]);
#endif
	}
	msg->pos += SByte;
	return v;
}

uint32_t mixp_pu32_load(MIXP_MESSAGE* msg)
{
    uint32_t v = 0;
    int size = SDWord;
    if(msg->pos + size <= msg->end) 
    {
	unsigned char* buf = (unsigned char*)msg->pos;
	
	v |= buf[3]<<24;
	v |= buf[2]<<16;
	v |= buf[1]<<8;
	v |= buf[0];
#ifdef _DEBUG
	fprintf(mixp_debug_stream, "mixp_pu32_load() value=%d text=%02X:%02X:%02X:%02X\n",
	    v,
	    buf[0],
	    buf[1],
	    buf[2],
	    buf[3]);
#endif
    }
    msg->pos += size;
    return v;
}

uint64_t mixp_pu64_load(MIXP_MESSAGE* msg)
{
    uint64_t v = 0;
    int size = SDWord*2;
    if(msg->pos + size <= msg->end) 
    {
	unsigned char* buf = (unsigned char*)msg->pos;

	v = buf[0]     + 
	    (buf[1]<<8)  + 
	    (buf[2]<<16) + 
	    (buf[3]<<24) +
	    ((uint64_t)buf[4]<<32) + 
	    ((uint64_t)buf[5]<<40) + 
	    ((uint64_t)buf[6]<<48) + 
	    ((uint64_t)buf[7]<<52);

#ifdef _DEBUG
	fprintf(mixp_debug_stream,"mixp_pu64_load() value=%llu text=%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
	    (long long unsigned int)v,
	    buf[0],
	    buf[1],
	    buf[2],
	    buf[3],
	    buf[4],
	    buf[5],
	    buf[6],
	    buf[7]);
#endif
    }
    msg->pos += size;
    return v;
}

uint16_t mixp_pu16_load(MIXP_MESSAGE* msg)
{
    uint32_t v = 0;
    int size = SWord;
    if(msg->pos + size <= msg->end) 
    {
	unsigned char* buf = (unsigned char*)msg->pos;
	
	v |= buf[1]<<8;
	v |= buf[0];
#ifdef _DEBUG
	fprintf(mixp_debug_stream, "mixp_pu16_load() value=%d text=%02X:%02X\n",
	    v,
	    buf[1],
	    buf[0]);
#endif
    }
    msg->pos += size;
    return v;
}

void
mixp_pu32(MIXP_MESSAGE *msg, uint32_t *val) 
{
	if (msg->mode == MsgPack)
	    mixp_pu32_store(msg, *val);
	else
	    *val = mixp_pu32_load(msg);
}

void
mixp_psize(MIXP_MESSAGE *msg, size_t *val) {
	if (msg->mode == MsgPack)
	{
		uint32_t sz = (uint32_t)*val;
		mixp_pu32_store(msg, sz);
	}
	else
	{
		uint32_t sz = mixp_pu32_load(msg);
		*val = (size_t)sz;
	}
}

void
mixp_pu8(MIXP_MESSAGE *msg, unsigned char *val) 
{
	if (msg->mode==MsgPack)
	    mixp_pu8_store(msg,*val);
	else
	    *val = mixp_pu8_load(msg);
}

void
mixp_pu16(MIXP_MESSAGE *msg, unsigned short *val) 
{
	if (msg->mode == MsgPack)
	    mixp_pu16_store(msg,*val);
	else
	    *val = mixp_pu16_load(msg);
}

void
mixp_pu64(MIXP_MESSAGE *msg, uint64_t *val) 
{
	if (msg->mode == MsgPack)
	    mixp_pu64_store(msg,*val);
	else
	    *val = mixp_pu64_load(msg);
}

void
mixp_pstring(MIXP_MESSAGE *msg, char **s) {
	unsigned short len;

	if(msg->mode == MsgPack)
		len = strlen(*s);
	mixp_pu16(msg, &len);

	if(msg->pos + len <= msg->end) {
		if(msg->mode == MsgUnpack) {
			*s = malloc(len + 1);
			memcpy(*s, msg->pos, len);
			(*s)[len] = '\0';
		}else
			memcpy(msg->pos, *s, len);
	}
	msg->pos += len;
}

void
mixp_pstrings(MIXP_MESSAGE *msg, unsigned short *num, char *strings[]) {
	char *s = NULL;		// suppress compiler warning. YES, it's really okay (look carefully ;-P)
	unsigned int i, size;
	unsigned short len;

	mixp_pu16(msg, num);
	if(*num > MIXP_MAX_WELEM) {
		msg->pos = msg->end+1;
		return;
	}

	if(msg->mode == MsgUnpack) {
		s = (char*)msg->pos;
		size = 0;
		for(i=0; i < *num; i++) {
			mixp_pu16(msg, &len);
			msg->pos += len;
			size += len;
			if(msg->pos > msg->end)
				return;
		}
		msg->pos = s;
		size += *num;
		s = malloc(size);
	}

	for(i=0; i < *num; i++) {
		if(msg->mode == MsgPack)
			len = strlen(strings[i]);
		mixp_pu16(msg, &len);

		if(msg->mode == MsgUnpack) {
			memcpy(s, msg->pos, len);
			strings[i] = s;
			s += len;
			msg->pos += len;
			*s++ = '\0';
		}else
			mixp_pdata(msg, &strings[i], len);
	}
}

void
mixp_pdata(MIXP_MESSAGE *msg, char **data, unsigned int len) {
	if(msg->pos + len <= msg->end) {
		if(msg->mode == MsgUnpack) {
			*data = malloc(len);
			memcpy(*data, msg->pos, len);
		}else
			memcpy(msg->pos, *data, len);
	}
	msg->pos += len;
}

void
mixp_pqid(MIXP_MESSAGE *msg, MIXP_QID *qid) {
	mixp_pu8(msg, &qid->type);
	mixp_pu32(msg, &qid->version);
	mixp_pu64(msg, &qid->path);
}

void
mixp_pqids(MIXP_MESSAGE *msg, unsigned short *num, MIXP_QID qid[]) {
	int i;

	mixp_pu16(msg, num);
	if(*num > MIXP_MAX_WELEM) {
		msg->pos = msg->end+1;
		return;
	}

	for(i = 0; i < *num; i++)
		mixp_pqid(msg, &qid[i]);
}

int
mixp_pstat(MIXP_MESSAGE *msg, MIXP_STAT *stat) {
	unsigned short size;
	
	if (stat==NULL)
	{
	    fprintf(mixp_error_stream,"mixp_pstat() stat ptr NULL\n");
	    return -1;
	}

	if(msg->mode == MsgPack)
		size = mixp_stat_sizeof(stat) - 2;

	mixp_pu16(msg, &size);
	mixp_pu16(msg, &stat->type);
	mixp_pu32(msg, &stat->dev);
	mixp_pqid(msg, &stat->qid);
	mixp_pu32(msg, &stat->mode);
	mixp_pu32(msg, &stat->atime);
	mixp_pu32(msg, &stat->mtime);
	mixp_pu64(msg, &stat->length);
	mixp_pstring(msg, &stat->name);
	mixp_pstring(msg, &stat->uid);
	mixp_pstring(msg, &stat->gid);
	mixp_pstring(msg, &stat->muid);

	// fix NULL values
	if (stat->name == NULL)
	    stat->name = strdup("");
	if (stat->uid == NULL)
	    stat->uid = strdup("");
	if (stat->gid == NULL)
	    stat->gid = strdup("");
	if (stat->muid == NULL)
	    stat->muid = strdup("");

	return 0;
}
