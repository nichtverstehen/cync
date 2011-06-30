/* Cyril Nikolaev */

#include "async_rl.h"

#include <stdlib.h> /* malloc */
#include <string.h> /* memcpy */
#include <limits.h> /* SIZE_MAX */
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "async_io.h"

#define RL_BUFFER_SIZE 10

#define min(a, b) ((a)<(b) ? (a) : (b))

struct AMY
{
	int fd;
    char buf[RL_BUFFER_SIZE];
	size_t start, limit; /* content */
};

/* read from file to internal buffer replacing current content
-1 - error, 0 - eof, >0 - ok */
a_Function(feed_buffer, { AMY* my; })
{
	a_Begin;
	
	a_Call(async_read, aArg.my->fd, aArg.my->buf, RL_BUFFER_SIZE);
	
	ssize_t r = aRet;
	if (r == -1)
	{
		a_Return(-1);
	}
	
	aArg.my->start = 0;
	aArg.my->limit = r;
	
	a_EndR(r);
}

/* read from buffer contents
0 - line read, 1 - line incomplete, 2 - buffer overrun */
static int read_buffer (AMY* my, char* dest, size_t len, size_t* have_read)
{
	const char* newline = memchr (my->buf + my->start, '\n', my->limit - my->start);
	size_t line_len = newline ? newline - my->buf - my->start + 1: my->limit - my->start;
	
	size_t limit = min(line_len, len);
	memcpy (dest, my->buf + my->start, limit);
	
	my->start += limit;
	*have_read = limit;
	
	return limit < line_len ? 2 : newline ? 0 : 1;
}

/*/////////////////////////////////////////////////////////////////////////////*/

AMY* arl_fromfd (int fd)
{
	AMY* my = malloc (sizeof(struct AMY));	
	my->fd = fd;
	my->start = 0;
	my->limit = 0;
	
	return my;
}

int arl_close (AMY* my)
{
	free (my);
	return 0;
}

a_FunctionH (arl_readline, { AMY* my; char* buf; size_t len; })
{
	a_Local {
		char* buf_pos;
		int r;
	};
	
	a_Begin;
	
	aLoc.buf_pos = aArg.buf;
	aLoc.r = 0;
	
	do
	{
		size_t have_read = 0;
		aLoc.r = read_buffer (aArg.my, aLoc.buf_pos, aArg.len - (aLoc.buf_pos-aArg.buf), &have_read);
		aLoc.buf_pos += have_read;
		
		if (aLoc.r == 2) /* buffer full */
		{
			return -2;
		}
	
		if (aLoc.r == 1) /* no line boundary */
		{
			a_Call(feed_buffer, aArg.my);
			aLoc.r = aRet;
			
			if (aLoc.r < 0)
			{
				a_Return(-1);
			}
		}
	}
	while (aLoc.r != 0);
	
	a_EndR(aLoc.buf_pos - aArg.buf);
}
