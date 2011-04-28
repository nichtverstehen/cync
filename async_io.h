#ifndef async_io_h__
#define async_io_h__

#include <stddef.h>
#include <sys/socket.h>
#include "async.h"

int async_ioloop (hstack_t mainstack);

a_Function (async_waitfor, { struct pollfd* pollfd; });

a_Function (async_read, { int fildes; void *buf; size_t nbyte; });
a_Function (async_write, { int fildes; const void *buf; size_t nbyte; });
a_Function (async_accept, { int socket; struct sockaddr* address; socklen_t* address_len; });

#endif
