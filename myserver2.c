#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>

#include "async_rl.h"
#include "async_io.h"

#define LISTEN_QUEUE_SIZE 1
#define MAX_FILENAME 1024
#define CAT_BUFFER_SIZE 1024

#define countof(x) (sizeof (x) / sizeof (x[0]))

void usage ()
{
	static const char USAGE_LINE[] = "USAGE: myserver [ <port> | -h ]. Default port is 1025.\n";
	fprintf(stderr, "%s", USAGE_LINE);
}

/* feed a buffer to fd */
a_Function(cat_buffer, { int client; const char* buffer; size_t cb; })
{
	a_Local {
		size_t written;
	};
	
	a_Begin;
	aLoc.written = 0;
	
	while (aLoc.written < aArg.cb)
	{
		a_Call(async_write, aArg.client, aArg.buffer + aLoc.written, aArg.cb - aLoc.written);
		
		ssize_t chunk_len = aRet;
		
		if (chunk_len == -1)
		{
			a_Return(-1);
		}
		
		aLoc.written += chunk_len;
	}
	
	a_EndR(0);
}

/* feed a file to fd */
a_Function(cat_file, { int client; const char* filename; })
{
	a_Local {
		char buffer[CAT_BUFFER_SIZE];
		int fd;
		ssize_t read_cb;
		int status;
	};
	
	a_Begin;
	aLoc.status = -1;
	
	printf("cat_file %d: %s\n", aArg.client, aArg.filename);
	
	aLoc.fd = open (aArg.filename, O_RDONLY);
	if (aLoc.fd >= 0)
	{
		do
		{
			aLoc.status = -1; // failed unless otherwise
			aLoc.read_cb = read (aLoc.fd, aLoc.buffer, CAT_BUFFER_SIZE);
			
			if (aLoc.read_cb == 0)
			{
				// eof
				aLoc.status = 0;
				break;
			}
			
			if (aLoc.read_cb > 0)
			{
				a_Call(cat_buffer, aArg.client, aLoc.buffer, aLoc.read_cb);
				if (aRet >= 0)
				{
					aLoc.status = 0;
				}
			}
		}
		while (aLoc.status == 0);
		
		close (aLoc.fd);
	}
	
	a_EndR(aLoc.status);
}

int is_safe_filename(const char* filename)
{
	// probably some cases are not handled
	return 1;//filename[0] != '/' && strstr(filename, "/../") == 0 && strncmp(filename, "../", 3) != 0;
}

/* process a command from client */
a_Function(process_client_line, { int client; AMY* my_client; })
{
	a_Local {
		char buffer [MAX_FILENAME+1]; /* +1 to add \0 */
		ssize_t r;
	};
	
	a_Begin;
	
	a_Call(arl_readline, aArg.my_client, aLoc.buffer, MAX_FILENAME);
	aLoc.r = aRet;
	
	if (aLoc.r == 0)
	{
		/* eof */
		a_Return(0);
	}
	
	if (aLoc.r > 0)
	{
		if (aLoc.buffer[aLoc.r-1] == '\n')
		{
			aLoc.r -= 1;
		}
		
		aLoc.buffer[aLoc.r] = 0;
	
		if (is_safe_filename(aLoc.buffer))
		{
			a_Call(cat_file, aArg.client, aLoc.buffer);
			
			if (aRet >= 0)
			{
				a_Return(1);
			}
		}

	}
	
	a_EndR(-1);
}

/* acquire client, read commands from client */
a_Function(interact, { int client; })
{
	a_Local {
		AMY* my_client;
		int r;
	};
	
	a_Begin;
	aLoc.r = -1;
	aLoc.my_client = arl_fromfd (aArg.client);
	
	do
	{
		a_Call(process_client_line, aArg.client, aLoc.my_client);
		aLoc.r = aRet;
	}
	while (aLoc.r == 1);
	/* successful if last r is 0 */
	
	arl_close (aLoc.my_client);
	close (aArg.client);
	printf("closed %d\n", aArg.client);
	
	a_EndR(aLoc.r != 0 ? -1 : 0);
}

/* acquire client, fork and do work in child */
a_Function(process_client, { int client; })
{
	a_Local {
		int status;
	};
	a_Begin;
	
	async_start(interact, &aLoc.status, NULL, aArg.client);
	//a_Call(interact, aArg.client);
	
	if (aLoc.status < 0)
	{
		a_Return(-1);
	}
	
	a_EndR(0);
}

/* create socket, bind on port and listen */
int setup_listening_socket (uint16_t port, int backlog)
{
	int sock = -1;
	struct sockaddr_in addr;
	
	sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock >= 0)
	{
		addr.sin_family = AF_INET;
		addr.sin_port = htons (port);
		addr.sin_addr.s_addr = htonl (INADDR_ANY);
	
		if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) >= 0)
		{
			if (listen (sock, backlog) >= 0)
			{
				/* everything is ok */
				return sock;
			}
		}
		
		close (sock);
	}
	
	return -1;
}

/* accept connection and pass to process client */
a_Function(accept_loop, { int sock; })
{
	a_Local {
		int client;
		struct sockaddr client_addr;
		socklen_t client_addr_size;
	};
	
	a_Begin;
	aLoc.client = -1;
	
	while (1)
	{
		a_Call(async_accept, aArg.sock, &aLoc.client_addr, &aLoc.client_addr_size);
		aLoc.client = aRet;
		printf("accepted %d\n", aLoc.client);
		
		if (aLoc.client < 0)
		{
			a_Return(-1);
		}
		
		a_Call(process_client, aLoc.client);
		if (aRet < 0)
		{
			a_Return(-1);
		}
	}
	
	a_EndR(0);
}

a_Function (run_server, { uint16_t port; })
{
	a_Local {
		int status;
		int sock;
	};
	
	a_Begin;
	aLoc.status = -1;
	aLoc.sock = -1;
	
	aLoc.sock = setup_listening_socket (aArg.port, LISTEN_QUEUE_SIZE);
	if (aLoc.sock >= 0)
	{
		a_Call(accept_loop, aLoc.sock);
			
		if (aRet >= 0)
		{
			aLoc.status = EXIT_SUCCESS;
		}
		
		close (aLoc.sock);
	}
	
	a_EndR(aLoc.status);
}

int main (int argc, char** argv)
{
	uint16_t port = 1025;
	unsigned long arg_long;
	int r = -1;
	
	if (argc == 2 && 0 == strcmp (argv[1], "-h"))
	{
		usage ();
		return EXIT_SUCCESS;
	}
	
	if (argc == 2)
	{
		arg_long = strtoul (argv[1], (char**) NULL, 10);
		if (arg_long == 0 || arg_long > 65535)
		{
			usage ();
			return EXIT_FAILURE;
		}
		
		port = arg_long;
	}
	
	hstack_t stack = hstack_new ();
	async_init_stack(stack, run_server, port);
	r = async_ioloop(stack);
	
	return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
