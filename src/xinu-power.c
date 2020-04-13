/**
 * @file xinu-power.c
 *
 * The XINU Power Client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <libgen.h>
#include <rpc.h>
#include <xinu-power.h>

static void usage(char *name);
static int socket_connect(int port, char *ipaddr);
static int wait_for_msg(int sockfd, char *msg);

int main(int argc, char **argv)
{
	char *cmd;
	char buf[128]; /* for success message */
	int sockfd, i;

	/* check arguments */
	if(argc != 2)
	{
		fprintf(stderr, "invalid number of arguments\n");
		usage(argv[0]);
	}
	if(strlen(argv[1]) != VALID_CMD_LEN)
	{
		fprintf(stderr, "invalid argument length\n");
		usage(argv[0]);
	}

	cmd = argv[1];

	/* check command syntax */
	if( valid_command(cmd) )
	{
		/* valid command, send it to xinu-rebootd */

		/* emulating old output to play nice with existing expect scripts */
		printf("\nConnecting to Rebooter...\n");

		/* connect to daemon, retry on missed connection */
		for ( i = 0; i < CONNECT_RETRIES; i++ )
		{
			if ( -1 == (sockfd = socket_connect(REBOOTDPORT, REBOOTDIP)))
			{
				fprintf(stderr,
						"could not connect! try %d of %d\n", i,
						CONNECT_RETRIES);
				sleep(1);
			}
			else { break; }
		}
		if ( -1 == sockfd )
		{
			fprintf(stderr,
					"connect failed after %d tries!\n", CONNECT_RETRIES);
			exit(1);
		}

		/* send command along to connected daemon */
		if ( -1 == send_msg(sockfd, cmd) )
		{
			fprintf(stderr, "error sending command!\n");
			exit(1);
		}

		/* wait for success response */	
		if ( -1 == wait_for_msg(sockfd, MSG_SUCCESS) )
		{
			fprintf(stderr, "success message not received!\n");
			exit(1);
		}

		if( close(sockfd) == -1 )
		{
			perror("close()");
			exit(1);
		}

		/* emulating old output to play nice with existing expect scripts */
		success_string(cmd, buf);
		printf("%s\n", buf);

	}
	else
	{
		fprintf(stderr, "invalid command: %s\n", argv[1]);
		usage(argv[0]);
	}
			
	exit(0);
	return 0;
}

/**
 * Connect to a given host on a socket.
 * @return sockfd on success, -1 on error
 */
static int socket_connect(int port, char *ipaddr)
{
	int sockfd;
	struct sockaddr_in dest_addr;   // will hold the destination addr

    sockfd = socket(PF_INET, SOCK_STREAM, 0);

	if(sockfd == -1)
	{
		perror("socket()");
		return -1;
	}

    dest_addr.sin_family = AF_INET;          // host byte order
    dest_addr.sin_port = htons(REBOOTDPORT);   // short, network byte order
    dest_addr.sin_addr.s_addr = inet_addr(REBOOTDIP);
    memset(dest_addr.sin_zero, '\0', sizeof dest_addr.sin_zero);

    if( connect(sockfd, (struct sockaddr *)&dest_addr, sizeof dest_addr)
			== -1 )
	{
		perror("connect()");
		return -1;
	}
	return sockfd;
}

/**
 * Wait for specified message on a socket for a given length of time.
 * @arg sockfd socket to read from
 * @arg msg message to look for, properly null terminated
 * @return 1 on success, -1 on error
 */
static int wait_for_msg(int sockfd, char *msg)
{
	char buf[RECVBUFLEN];
	if ( 0 >= recv(sockfd, buf, RECVBUFLEN, 0) )
	{
		return -1;
	}
//	printf("wait_for_msg: msg = [%s], buf = [%s]\n", msg, buf);
	if ( strcmp(buf, msg) != 0 ) 
	{
		return -1;
	}

	return 1;
}

/**
 * Print program usage.
 * @arg name name of the program as invoked, usually gotten from argv[0]
 */
static void usage(char *name)
{
	fprintf( stderr, "usage: %s <command>\n", name);
	fprintf( stderr, "<command>:\n");
	fprintf( stderr, "\t\tdXX - power[d]own outlet #XX\n");
	fprintf( stderr, "\t\tuYY - power[u]p outlet #YY\n");
	fprintf( stderr, "\t\trZZ - [r]eboot outlet #ZZ\n");
	fprintf( stderr, "\t\toff - all outlets [off]\n");
	exit(1);
}
