#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdint.h>
#include <netinet/in.h>
#include "myreadline.h"

#define LISTEN_QUEUE_SIZE 1
#define MAX_FILENAME 1024
#define CAT_BUFFER_SIZE 1024

void usage()
{
	MY* my_out = myrl_fromfd (2);
	if (!my_out)
	{
		return;
	}
	
	myrl_writeline (my_out, "USAGE: myserver [ <port> | -h ]. Default port is 1025.\n", 10); //!!!!!!
	myrl_close (my_out);
}

int cat_buffer (int client, const char* buffer, size_t cb)
{
	size_t written = 0;
	while (written < cb)
	{
		ssize_t chunk_len = write (client, buffer + written, cb - written);
		if (chunk_len == -1)
		{
			return -1;
		}
		
		written += chunk_len;
	}
	
	return 0;
}

int cat_file (int client, const char* filename)
{
	int fd = open (filename, O_RDONLY);
	if (fd == -1)
	{
		return -1;
	}
	
	char buffer[CAT_BUFFER_SIZE];
	ssize_t read_cb;
	while (0 < (read_cb = read (fd, buffer, CAT_BUFFER_SIZE)))
	{
		if (-1 == cat_buffer (client, buffer, read_cb))
		{
			return -1;
		}
	}
	
	return read_cb == 0;
}

int interact (int client)
{
	MY* my_client = myrl_fromfd (client);
	if (!my_client)
	{
		close (client);
		return EXIT_FAILURE;
	}
	
	RES r;
	char buffer [MAX_FILENAME+1]; // +1 to add \0
	while (1)
	{
		r = myrl_readline (my_client, buffer, MAX_FILENAME);
		if (MYRL_RES(r) < 1)
		{
			break;
		}
		
		int len = MYRL_RES(r);
		if (buffer[len-1] == '\n')
		{
			len -= 1;
		}
		
		buffer[len] = 0;
		
		if (cat_file (client, buffer) == -1)
		{
			myrl_close (my_client);
			return EXIT_FAILURE;
		}
	}
	
	myrl_close (my_client);
	return MYRL_RES(r) >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int process_client (int server, int client)
{
	int pid = fork();
	if (pid == 0)
	{
		// client
		close (server);
		int r = interact (client);
		exit (r);
	}
	else
	{
		// parent
		close (client);
		return pid != -1;
	}
}

int run_server (uint16_t port)
{
	int sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		return EXIT_FAILURE;
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons (port);
	addr.sin_addr.s_addr = htonl (INADDR_ANY);
	
	if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == -1)
	{
		return EXIT_FAILURE;
	}
	
	if (listen (sock, LISTEN_QUEUE_SIZE) < 0)
	{
		return EXIT_FAILURE;
	}
	
	while (1)
	{
		struct sockaddr client_addr;
		socklen_t client_addr_size;
		int client = accept (sock, &client_addr, &client_addr_size);
		if (client == -1)
		{
			return EXIT_FAILURE;
		}
		
		if (process_client (sock, client) == -1)
		{
			return EXIT_FAILURE;
		}
	}
	
	return EXIT_SUCCESS;
}

int main (int argc, char** argv)
{
	uint16_t port = 1025;
	
	if (argc == 2 && 0 == strcmp (argv[1], "-h"))
	{
		usage ();
		return EXIT_SUCCESS;
	}
	
	if (argc == 2)
	{
		unsigned long arg_long = strtoul (argv[1], (char**) NULL, 10);
		if (arg_long == 0 || arg_long > 65535)
		{
			usage ();
			return EXIT_FAILURE;
		}
		
		port = arg_long;
	}
	
	int r = run_server (port);
	return r;
}
