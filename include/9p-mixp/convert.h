
#ifndef __MIXP_CONVERT_H
#define __MIXP_CONVERT_H

#include <inttypes.h>
#include <9p-mixp/msgs.h>
#include <9p-mixp/qid.h>
#include <9p-mixp/stat.h>

void mixp_pu8      (MIXP_MESSAGE *msg, unsigned char *val);
void mixp_pu16     (MIXP_MESSAGE *msg, uint16_t *val);
void mixp_pu32     (MIXP_MESSAGE *msg, uint32_t *val);
void mixp_psize    (MIXP_MESSAGE *msg, size_t* val);
void mixp_pu64     (MIXP_MESSAGE *msg, uint64_t *val);
void mixp_pdata    (MIXP_MESSAGE *msg, char **data, unsigned int len);
void mixp_pstring  (MIXP_MESSAGE *msg, char **s);
void mixp_pstrings (MIXP_MESSAGE *msg, unsigned short *num, char *strings[]);
void mixp_pqid     (MIXP_MESSAGE *msg, MIXP_QID *qid);
void mixp_pqids    (MIXP_MESSAGE *msg, unsigned short *num, MIXP_QID qid[]);
int  mixp_pstat    (MIXP_MESSAGE *msg, MIXP_STAT *stat);

#endif
