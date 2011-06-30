#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "async_io.h"
#include "async.h"
#include "mlist.h"

#define ASSERT assert

static struct pollfd* s_pollfd_list = NULL;
static struct pollfd** s_pollfd_ptrs = NULL;
static hstack_t* s_waiters = NULL;

static int async_ioloop_worker()
{
	int status = 0;
	
	while (mlist_len (s_waiters) && status >= 0)
	{
		ASSERT (mlist_len (s_pollfd_list) == mlist_len (s_pollfd_ptrs));
		ASSERT (mlist_len (s_pollfd_list) == mlist_len (s_waiters));
		
		int list_len = mlist_len (s_pollfd_list);
		int nfds = poll (s_pollfd_list, list_len, -1);
		
		if (nfds == -1 && errno == EINTR) continue;
		if (nfds == -1) return -1;
		
		size_t i, proc;
		for (i = 0, proc = 0; i < list_len && proc < nfds; ++i)
		{
			if (s_pollfd_list[i].revents != 0)
			{
				*s_pollfd_ptrs[i] = s_pollfd_list[i];
				int cont_status = async_run_stack (s_waiters[i], NULL);
				
				if (cont_status < 0) status = -1;
				++proc;
			}
		}
		
		for (i = list_len; i --> 0; )
		{
			if (s_pollfd_list[i].revents != 0)
			{
				mlist_remove (s_pollfd_list, i);
				mlist_remove (s_pollfd_ptrs, i);
				mlist_remove (s_waiters, i);
			}
		}
	}
}

int async_ioloop (hstack_t mainstack)
{
	int status = -1;
	
	ASSERT (!s_pollfd_list && !s_pollfd_ptrs && !s_waiters);
	
	s_pollfd_list = mlist_new (struct pollfd);
	s_pollfd_ptrs = mlist_new (struct pollfd*);
	s_waiters = mlist_new (hstack_t);
	
	if (!s_pollfd_list || !s_pollfd_ptrs || !s_waiters)
	{
		if (s_pollfd_list) mlist_delete (s_pollfd_list);
		if (s_pollfd_ptrs) mlist_delete (s_pollfd_list);
		if (s_waiters) mlist_delete (s_waiters);
		return -1;
	}
	
	if (async_run_stack (mainstack, NULL) >= 0)
	{
		status = async_ioloop_worker();
	}
	
	mlist_delete (s_pollfd_list);
	mlist_delete (s_pollfd_ptrs);
	mlist_delete (s_waiters);
	
	s_pollfd_list = NULL; s_pollfd_ptrs = NULL; s_waiters = NULL;
	return 0;
}

static int add_to_poll (struct pollfd* pollfd, hstack_t cont)
{
	ssize_t add_fd = mlist_add (s_pollfd_list, pollfd);
	ssize_t add_pfd = mlist_add (s_pollfd_ptrs, &pollfd);
	ssize_t add_cont = mlist_add (s_waiters, &cont);
	
	if (add_fd < 0 || add_pfd < 0 || add_cont < 0)
	{
		if (add_fd >= 0) mlist_remove (s_pollfd_list, add_fd);
		if (add_pfd >= 0) mlist_remove (s_pollfd_ptrs, add_pfd);
		if (add_cont >= 0) mlist_remove (s_waiters, add_cont);
		return -1;
	}
	
	ASSERT (add_fd == add_pfd && add_pfd == add_cont);
	return 0;
}

a_FunctionH (async_waitfor, { struct pollfd* pollfd; })
{
	a_Begin;
	
	add_to_poll (aArg.pollfd, aStack); a_Junction();
	
	a_End;
}

a_FunctionH (async_read, { int fildes; void *buf; size_t nbyte; })
{
	a_Local {
		struct pollfd pollfd;
	};
	
	a_Begin;
	
	aLoc.pollfd.fd = aArg.fildes;
	aLoc.pollfd.events = POLLIN | POLLERR|POLLHUP|POLLNVAL;
	aLoc.pollfd.revents = 0;
	a_Call (async_waitfor, &aLoc.pollfd);
	
	if (aLoc.pollfd.revents & (POLLERR|POLLNVAL))
	{
		a_Return(-1);
	}
	
	ssize_t status = read (aArg.fildes, aArg.buf, aArg.nbyte);
	ASSERT (!(status == -1 && errno == EAGAIN));
	
	a_EndR (status);
}

a_FunctionH (async_write, { int fildes; const void *buf; size_t nbyte; })
{
	a_Local {
		struct pollfd pollfd;
	};
	
	a_Begin;
	
	aLoc.pollfd.fd = aArg.fildes;
	aLoc.pollfd.events = POLLOUT | POLLERR|POLLHUP|POLLNVAL;
	aLoc.pollfd.revents = 0;
	a_Call (async_waitfor, &aLoc.pollfd);
	
	if (aLoc.pollfd.revents & (POLLHUP|POLLERR|POLLNVAL))
	{
		a_Return(-1);
	}
	
	ssize_t status = write (aArg.fildes, aArg.buf, aArg.nbyte);
	ASSERT (!(status == -1 && errno == EAGAIN));
	
	a_EndR (status);
}

a_FunctionH (async_accept, { int socket; struct sockaddr* address; socklen_t* address_len; })
{
	a_Local {
		struct pollfd pollfd;
	};
	
	a_Begin;
	
	aLoc.pollfd.fd = aArg.socket;
	aLoc.pollfd.events = POLLIN | POLLERR|POLLHUP|POLLNVAL;
	aLoc.pollfd.revents = 0;
	a_Call (async_waitfor, &aLoc.pollfd);
	
	if (aLoc.pollfd.revents & (POLLHUP|POLLERR|POLLNVAL))
	{
		a_Return(-1);
	}
	
	int fd = accept (aArg.socket, aArg.address, aArg.address_len);
	ASSERT (!(fd == -1 && errno == EAGAIN));
	
	a_EndR (fd);
}

