#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <netinet/in.h>
#include <errno.h>
#include "myreadline.h"

#define LISTEN_QUEUE_SIZE 1
#define MAX_FILENAME 1024
#define CAT_BUFFER_SIZE 1024

#define countof(x) (sizeof (x) / sizeof (x[0]))

void usage ()
{
	static const char USAGE_LINE[] = "USAGE: myserver [ <port> | -h ]. Default port is 1025.\n";
	MY* my_out = myrl_fromfd (2);
	if (!my_out)
	{
		return;
	}
	
	myrl_writeline (my_out, USAGE_LINE, countof (USAGE_LINE));
	myrl_close (my_out);
}

/* feed a buffer to fd */
int cat_buffer (int client, const char* buffer, size_t cb)
{
	size_t written = 0;
	ssize_t chunk_len = 0;
	
	while (written < cb)
	{
		chunk_len = write (client, buffer + written, cb - written);
		
		if (chunk_len == -1)
		{
			return -1;
		}
		
		written += chunk_len;
	}
	
	return 0;
}

/* feed a file to fd */
int cat_file (int client, const char* filename)
{
	char buffer[CAT_BUFFER_SIZE];
	int fd = -1;
	ssize_t read_cb = -1;
	int status = -1;
	
	fd = open (filename, O_RDONLY);
	if (fd >= 0)
	{
		do
		{
			status = -1; // failed unless otherwise
			read_cb = read (fd, buffer, CAT_BUFFER_SIZE);
			
			if (read_cb == 0)
			{
				// eof
				status = 0;
				break;
			}
			
			if (read_cb > 0)
			{
				if (cat_buffer (client, buffer, read_cb) >= 0)
				{
					status = 0;
				}
			}
		}
		while (status == 0);
		
		close (fd);
	}
	
	return status;
}

/* process a command from client */
int process_client_line (int client, MY* my_client)
{
	char buffer [MAX_FILENAME+1]; /* +1 to add \0 */
	RES r;

	r = myrl_readline (my_client, buffer, MAX_FILENAME);
	
	if (MYRL_RES (r) == 0)
	{
		/* eof */
		return 0;
	}
	
	if (MYRL_RES (r) > 0)
	{
		int len = MYRL_RES (r);
		if (buffer[len-1] == '\n')
		{
			len -= 1;
		}
		
		buffer[len] = 0;
		
		if (cat_file (client, buffer) >= 0)
		{
			return 1;
		}
	}
	
	return -1;
}

/* acquire client, read commands from client */
int interact (int client)
{
	MY* my_client = NULL;
	int r = -1;
	
	my_client = myrl_fromfd (client);
	if (my_client)
	{
		do
		{
			r = process_client_line (client, my_client);
		}
		while (r == 1);
		/* successful if last r is 0 */
	
		myrl_close (my_client);
	}
	
	if (!my_client)
	{
		close (client);
	}
		
	return r != 0 ? -1 : 0;
}

/* acquire client, fork and do work in child */
int process_client (int server, int client)
{
	int r = -1;
	int pid = -1;
	
	pid = fork ();
	if (pid == 0)
	{
		/* client */
		close (server);
		r = interact (client);
		
		if (r < 0)
		{
			perror (NULL);
			exit (EXIT_FAILURE);
		}
		
		exit (EXIT_SUCCESS);
	}
	else
	{
		/* parent */
		close (client);
		return pid != -1;
	}
}

static void sigchld_handler (int s)
{
    while (waitpid (-1, NULL, WNOHANG) > 0);
}

/* set handler to wait for children */
int setup_zombie_catching ()
{
	struct sigaction chld_action;
	
	chld_action.sa_handler = &sigchld_handler;
	sigemptyset (&chld_action.sa_mask);
	chld_action.sa_flags = SA_NOCLDSTOP;
	
	return sigaction (SIGCHLD, &chld_action, NULL);
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
int accept_loop (int sock)
{
	int client = -1;
	struct sockaddr client_addr;
	socklen_t client_addr_size;
	
	while (1)
	{
		client = accept (sock, &client_addr, &client_addr_size);
		
		if (client < 0 && errno == EINTR)
		{
			continue;
		}
		
		if (client < 0)
		{
			return -1;
		}
		
		if (process_client (sock, client) < 0)
		{
			return -1;
		}
	}
	
	return 0;
}

int run_server (uint16_t port)
{
	int status = EXIT_FAILURE;
	int sock = -1;
	
	sock = setup_listening_socket (port, LISTEN_QUEUE_SIZE);
	if (sock >= 0)
	{
		if (setup_zombie_catching () >= 0)
		{
			if (accept_loop (sock) >= 0)
			{
				status = EXIT_SUCCESS;
			}
		}
		
		close (sock);
	}
	
	return status;
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
	
	r = run_server (port);
	if (r != EXIT_SUCCESS)
	{
		perror (NULL);
	}
	
	return r;
}
