/*
 * @file xinu-rebootd.c
 *
 * The XINU Reboot Daemon
 */

#include <config.h>
#include <fcntl.h>
#include <libgen.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include <xinu-power.h>

#ifdef LIBWRAP
#include <tcpd.h>
int allow_severity = 0;
int deny_severity  = 0;
#endif

static int dumppid = 0;

static struct rpc_status {
	unsigned char state;
} outlets[NUM_OUTLETS];

static struct sigaction ignoreChildDeath =
{
    NULL, 0, SA_NOCLDSTOP | SA_RESTART, (int)NULL
};

/* prototypes for this file */
static int init_device(void);
static int init_socket(int port);
static int get_command(int connected_sockfd, char *received_cmd);
static int execute_command(char *cmd);
static void setup_daemon(void);

int main(int argc, char **argv)
{
    int listening_sockfd; // socket to listen for incoming connections
	int connected_sockfd; // socket to read/write to connected process
    struct sockaddr_in their_addr; // connector's address information
	int their_addrlen;    // socket funcs require pointer to size int, weird
	char received_cmd[VALID_CMD_LEN+1]; // holds cmd, null terminated
	int len, bytes_sent, tcp_wrap_result;

	if( (2 == argc) && (0 == strncmp( argv[ 1 ], "-p", 2 )) ) {
			dumppid = 1;
		}

	setup_daemon();

	/* open connection to syslogd, see syslog(3) man page */
	openlog("xinu-powerd",
			(LOG_CONS | LOG_PID),
			LOG_DAEMON);

	syslog(LOG_INFO, "Starting XINU Power Daemon %s\n", VERSION);

	if( init_device() == -1 )
	{
		syslog(LOG_ERR, "Failed to initialize rebooter device!\n");
		exit(1);
	}

	syslog(LOG_INFO,"Listening on port %d\n", POWERD_PORT);
	if( (listening_sockfd = init_socket(POWERD_PORT)) == -1 )
	{
		syslog(LOG_ERR, "Socket initialization failed!\n");
		exit(1);
	}

	for(EVER)
	{
		their_addrlen = sizeof their_addr;
		connected_sockfd = accept(listening_sockfd, 
						(struct sockaddr *)&their_addr,
						&their_addrlen);

		if( connected_sockfd == -1 )
		{
			syslog(LOG_ERR, "accept(): %s", strerror(errno));
			syslog(LOG_ERR, "Socket connection to failed!\n");
			exit(1);
		}

#ifdef LIBWRAP
		/* Ask TCP wrappers if this source address is OK. */
		tcp_wrap_result = hosts_ctl(CS_PSERVER, STRING_UNKNOWN,
									inet_ntoa(their_addr.sin_addr),
									STRING_UNKNOWN);
		/* Nope.  Reject request without even responding. */
		if (0 == tcp_wrap_result)
		{
			syslog(LOG_ERR, "Rejecting connection from barred IP %s.\n",
				   inet_ntoa(their_addr.sin_addr) );
			if( -1 == close(connected_sockfd) )
			{
				syslog(LOG_ERR, "close() connected: %s", strerror(errno));
			}
			continue;
		}
#endif

		syslog(LOG_INFO,"Client %s connected!\n", inet_ntoa(their_addr.sin_addr));

		if( -1 == get_command(connected_sockfd, received_cmd) )
		{
			syslog(LOG_ERR, "Could not get command from client!\n");
			if( -1 == close(connected_sockfd) )
			{
				syslog(LOG_ERR, "close() connected: %s", strerror(errno));
			}
			continue;
		}
		else
		{
			if( -1 == execute_command(received_cmd) )
			{
				syslog(LOG_INFO, "Sending invalid message.");
				if( send_msg(connected_sockfd, MSG_INVALID) <= 0 )
				{
					syslog(LOG_ERR, "Sending invalid message failed!\n");
					exit(1);
				}
				syslog(LOG_INFO,"Received invalid command.\n");
				if( -1 == close(connected_sockfd) )
				{
					syslog(LOG_ERR, "close() connected: %s", strerror(errno));
				}
				continue;
			}
			else {
				syslog(LOG_INFO, "Sending success message.");
				if( send_msg(connected_sockfd, MSG_SUCCESS) <= 0 )
				{
					syslog(LOG_ERR, "Sending success message failed!\n");
					exit(1);
				}
				syslog(LOG_INFO,"Command executed successfully.\n");
			}
		}

		if( -1 == close(connected_sockfd) )
		{
			syslog(LOG_ERR, "close() connected: %s", strerror(errno));
		}

		syslog(LOG_INFO,"Client %s disconnected!\n", inet_ntoa(their_addr.sin_addr));
	}

	if( close(listening_sockfd) == -1 )
	{
		syslog(LOG_ERR, "close() listening: %s", strerror(errno));
	}

	syslog(LOG_INFO, "Exiting XINU Reboot Daemon\n");

	exit(0);
    return 0;
}


/**
 * Initialize the rebooter device.  Sets all ports to a known state (OFF) and
 * updates internal representation of state.
 * @return -1 on error
 */
int init_device(void)
{
	int i;
	for(i = 0; i < NUM_OUTLETS; i++) { outlets[i].state = OFF; }
	return rpc_init();
}

/**
 * Start a listening socket on the specified port.
 * Print its own message on error.
 * @arg port port on which to open listening socket
 * @return socket file descriptor, -1 on error
 */
static int init_socket(int port)
{
	int sockfd;
	int yes = 1;                   // setsockopt is weird
    struct sockaddr_in my_addr;    // my address information

	if( (sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) 
	{
		syslog(LOG_ERR, "socket(): %s", strerror(errno));
		return -1;
	}

    my_addr.sin_family = AF_INET;          // host byte order
    my_addr.sin_port = htons(REBOOTDPORT); // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY;  // auto-fill with my IP
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	// lose the pesky "Address already in use" error message
 	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		return -1;
	} 

    if( bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) < 0 )
	{
		syslog(LOG_ERR, "bind(): %s", strerror(errno));
		return -1;
	}

    if( listen(sockfd, BACKLOG) < 0 )
	{
		syslog(LOG_ERR, "listen(): %s", strerror(errno));
		return -1;
	}

	return sockfd;
}

/**
 * Receive a command from the client through a connected socket.
 * @arg connected_sockfd a properly initialized and connected socket file
 *                       descriptor (use init_socket() and accept())
 * @arg received_cmd     pointer to buffer for received command, must 
 *                       be >= VALID_CMD_LEN + 1 for null terminator
 * @returns 1 on success, -1 on error
 */
static int get_command(int connected_sockfd, char *received_cmd)
{
	char recvbuf[RECVBUFLEN];
	int bytes_received;

	/* get a line of input from the client */
	bytes_received = recv(connected_sockfd,
							recvbuf,
							RECVBUFLEN, 0);
	if( bytes_received < 0 )
	{
		syslog(LOG_ERR, "receive(): %s", strerror(errno));
		return -1;
	}

	/* copy the command into the return buffer */
	strncpy(received_cmd, recvbuf, VALID_CMD_LEN);
	received_cmd[VALID_CMD_LEN] = '\0';

	/* check command syntax; */
	if(!valid_command(received_cmd)) 			
	{
		syslog(LOG_ERR,"command invalid! [%s]\n", received_cmd);
		return -1;
	}
	return 1;
}

/**
 * Interpret and execute a command received from a client.
 * @arg cmd a (properly null-terminated) three character command string with
 *          the first character being the command and the second two being a
 *          number between 00 and 99 which refers to the number of the backend
 *          on which to perform the command.
 * @return 1 on successful execution of command, -1 on invalid command
 */
static int execute_command(char *cmd)
{
	/* all[off] */
	if( strncmp( "off", cmd, 3 ) == 0 )
	{
		syslog(LOG_INFO, "turning off all devices");
		init_device();
		return 1;
	}
	/* power[d]own */
	else if( strncmp( "d", cmd, 1 ) == 0 )
	{
		syslog(LOG_INFO, "turning off outlet %d", atoi(cmd+1));
		/* Even if the state is off, send the off command. */
		//if(outlets[atoi(cmd+1)].state == OFF) return 1;
		outlets[atoi(cmd+1)].state = OFF;
		rpc_off(atoi(cmd+1));
		return 1;
	}
	/* power[u]p */
	else if( strncmp( "u", cmd, 1 ) == 0 )
	{
		syslog(LOG_INFO, "turning on outlet %d", atoi(cmd+1));
		/* If the state is already on, just return. */
		if(outlets[atoi(cmd+1)].state == ON) return 1;
		outlets[atoi(cmd+1)].state = ON;
		rpc_on(atoi(cmd+1));
		return 1;
	}
	/* [r]eboot */
	else if( strncmp( "r", cmd, 1 ) == 0 )
	{
		syslog(LOG_INFO, "resetting outlet %d", atoi(cmd+1));
		if(outlets[atoi(cmd+1)].state == ON)
		{
			rpc_reset(atoi(cmd+1));
			return 1;
		}
		else
		{
			outlets[atoi(cmd+1)].state = ON;
			rpc_on(atoi(cmd+1));
			return 1;
		}
	}

	return -1;
}

/**
 * Perform necessary tasks to become a proper network daemon,
 * taken from Linux Journal's "Linux Network Programming, Part 2"
 * <http://www.linuxjournal.com/article/2335>
 */
static void setup_daemon(void)
{
    struct rlimit resourceLimit = { 0 };
    int status = -1;
    int fileDesc = -1;
	FILE *pidfs;
	int i;

    /*
     * somewhere in the code
     */
    sigaction(SIGCHLD, &ignoreChildDeath, NULL);
    status = fork();
    switch (status)
    {
    case -1:
        perror("fork()");
        exit(1);
    case 0: /* child process */
        break;
    default: /* parent process */
        exit(0);
    }
    /*
     * child process
     */
    resourceLimit.rlim_max = 0;
    status = getrlimit(RLIMIT_NOFILE, &resourceLimit);
    if (-1 == status) /* shouldn't happen */
    {
        perror("getrlimit()");
        exit(1);
    }
    if (0 == resourceLimit.rlim_max)
    {
        syslog(LOG_CRIT, "Max number of open file descriptors is 0!!\n");
        exit(1);
    }

    for (i = 0; i < resourceLimit.rlim_max; i++)
    {
        (void) close(i);
    }
    status = setsid();
    if (-1 == status)
    {
        perror("setsid()");
        exit(1);
    }
    status = fork();
    switch (status)
    {
    case -1:
        perror("fork()");
        exit(1);
    case 0: /* (second) child process */
        break;
    default: /* parent process */
        exit(0);
    }
    /*
     * now we are in a new session and process
     * group than process that started the
     * daemon. We also have no controlling
     * terminal */
	if (dumppid)
	{
		char pidfile[32];
		sprintf(pidfile, "/var/run/%s.pid", CS_PSERVER);
		pidfs = fopen(pidfile, "w");
		fprintf(pidfs, "%d\n", getpid());
		fclose(pidfs);
	}

    chdir("/");
    umask(0);
    fileDesc = open("/dev/null", O_RDWR);/* stdin */
    (void) dup(fileDesc);  /* stdout */
    (void) dup(fileDesc);  /* stderr */
    /*
     * the rest of the daemon code executes in
     * this environment
     */
#ifdef CS_GROUP
	char pwname[ 32 ];
		
	pwname[ sizeof( pwname ) - 1 ] = '\0';
	pwname[ 0 ] = '\0';
	{
		struct passwd * pw;
	
		if( ( pw = getpwuid( geteuid() ) ) == NULL ) {
			perror( "getpwuid()" );
			exit( 1 );
		}
		strncpy( pwname, pw->pw_name, sizeof( pwname ) - 1 );
	}

	if( strncmp( pwname, "root", 4 ) == 0 ) {
		int gid;
		struct group * gent;
		struct passwd * pw;

		if( ( gent = getgrnam( CS_GROUP ) ) == NULL ) {
			syslog(LOG_ERR, "getgrnam() group not found: %s", strerror(errno));
			exit( 1 );
		}
		if( setgid( gent->gr_gid ) != 0 ) {
			syslog(LOG_ERR, "setgid(): %s", strerror(errno));
			exit( 1 );
		}
		if( ( pw = getpwnam( "nobody" ) ) == NULL ) {
			syslog(LOG_ERR, "getpwnam(): %s", strerror(errno));
			exit( 1 );
		}
		if( setuid( pw->pw_uid ) != 0 ) {
			syslog(LOG_ERR, "setuid(): %s", strerror(errno));
			exit( 1 );
		}
	}
#endif /* CS_GROUP */
}
