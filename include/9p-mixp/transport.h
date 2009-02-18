
#ifndef __MIXP_TRANSPORT_H
#define __MIXP_TRANSPORT_H

#include <9p-mixp/msgs.h>

size_t mixp_sendmsg(int fd, MIXP_MESSAGE *msg);
size_t mixp_recvmsg(int fd, MIXP_MESSAGE *msg);

#endif
