
#ifndef __IXP_LOCAL_H
#define __IXP_LOCAL_H

#define IXP_NO_P9_
#define IXP_P9_STRUCTS

#include <9p-mixp/mixp.h>

// #define thread ixp_thread

#define muxinit ixp_muxinit
#define muxfree ixp_muxfree
#define muxrpc ixp_muxrpc

void muxinit(MIXP_CLIENT*);
void muxfree(MIXP_CLIENT*);
MIXP_FCALL *muxrpc(MIXP_CLIENT*, MIXP_FCALL*);

static inline void __init_errstream()
{
    if (!mixp_error_stream)
	mixp_error_stream = stderr;
    if (!mixp_debug_stream)
	mixp_debug_stream = stderr;
}

#define USED(v) if(v){}else{}

#define MIXP_FREE(f)		{ if (f != NULL) free(f); f=NULL; }

#define DEFAULT_IOUNIT		4096

/** message.c **/
size_t mixp_fcall2msg(MIXP_MESSAGE *msg, MIXP_FCALL *fcall);
size_t mixp_msg2fcall(MIXP_MESSAGE *msg, MIXP_FCALL *fcall);
void mixp_fcall_free(MIXP_FCALL *fcall);

#endif
