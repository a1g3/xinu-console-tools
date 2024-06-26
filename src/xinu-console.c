/*
 * xinu-console.c - connect to remote console
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "cserver.h"

#define KEEPALIVE

static char class[ MAXCLASSNAME ];
static char host[ MAXHOSTNAME + 1 ];
static char connection[ MAXCONNECTIONNAME ];
static int g_dev = -1;

#define ESC_CHARACTER	0x0
#define SECONDS_DELAY	30

static int flushdata(int, char *, int);
static int getInput(char *);
static int getInputString(char *, int);

static char *prog = 0;

static int printusage(char *sb)
{
	fprintf( stderr,
		 "usage: %s [-t] [-f] [-c class] [-s server] [connection]\n",
		 sb );
	fprintf( stderr,
		 "xinu-console v2.04 \"Millenium Edition\"\n");
	exit( 1 );
}

#ifdef KEEPALIVE

static int g_sock = -1;
static struct request g_req;
static int g_gotinput = 0;

static void keepalive(int i)
{
	if( g_gotinput ) {
		sendRequest( g_sock, & g_req );
		g_gotinput = 0;
	}
	alarm( SECONDS_DELAY );
}

#endif

static void setSignals(void)
{
#ifdef KEEPALIVE
	alarm( 0 );
#endif
#ifdef SOLARIS
	{
		struct sigaction sa;
		
		sigemptyset( &(sa.sa_mask) );
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = SIG_IGN;
		(void) sigaction( SIGPIPE, & sa, 0 );
		(void) sigaction( SIGINT, & sa, 0 );
		(void) sigaction( SIGTSTP, & sa, 0 );
		sa.sa_handler = SIG_DFL;
		(void) sigaction( SIGCHLD, & sa, 0 );
#ifdef KEEPALIVE
		sa.sa_handler = keepalive;
#else
		sa.sa_handler = SIG_DFL;
#endif
		(void) sigaction( SIGALRM, & sa, 0 );
	}
#else
	signal( SIGPIPE, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGTSTP, SIG_IGN );
#ifdef KEEPALIVE
	signal( SIGALRM, keepalive );
#endif
#endif
}

static void resetSignals(void)
{
#ifdef KEEPALIVE
	alarm( 0 );
#endif
#ifdef SOLARIS
	{
		struct sigaction sa;
		
		sigemptyset( &(sa.sa_mask) );
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = SIG_DFL;
		(void) sigaction( SIGPIPE, & sa, 0 );
		(void) sigaction( SIGINT, & sa, 0 );
		(void) sigaction( SIGTSTP, & sa, 0 );
		(void) sigaction( SIGCHLD, & sa, 0 );
		(void) sigaction( SIGALRM, & sa, 0 );
	}
#else
	signal( SIGPIPE, SIG_DFL );
	signal( SIGINT, SIG_DFL );
	signal( SIGTSTP, SIG_DFL );
	signal( SIGALRM, SIG_DFL );
#endif
}

static int isatty_stdin = -1;

static void restoreTTY(void)
{
	if( isatty_stdin == 1 ) {
		restoretty( fileno( stdin ) );
	}
}

static void setTTY(void)
{
	if( isatty_stdin == -1 ) {
		isatty_stdin = 0;
		if( isatty( fileno( stdin ) ) ) {
			isatty_stdin = 1;
		}
	}
	if( isatty_stdin == 1 ) {
		rawtty( fileno( stdin ) );
	}
}

#define FLUSH_TIMEOUT 2

static int transparent = 0;

int main(int argc, char **argv)
{
	int i;
	int flushc;
    int getserial2;
	
	prog = argv[0];
	flushc = 0;

	host[ 0 ] = '\0';
	host[ MAXHOSTNAME ] = '\0';
	connection[ 0 ] = '\0';
	connection[ MAXCONNECTIONNAME - 1 ] = '\0';
	class[ 0 ] = '\0';
	class[ MAXCLASSNAME - 1 ] = '\0';

    getserial2 = 0;

	for( i = 1; i < argc; i++ ) {
		if( strnequ( argv[ i ], "-h", 2 ) ) {
			printusage( prog );
		}
		else if( strnequ( argv[ i ], "-t", 2 ) ) {
			transparent = 1;
		}
		else if( strncmp( argv[ i ], "-f", 2 ) == 0 ) {
			flushc = FLUSH_TIMEOUT;
		}
		else if( strnequ( argv[ i ], "-c", 2 ) ) 
        {
            /* Check to see if the class of the target is specified */
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					 "error: missing class name\n" );
				printusage( prog );
			}
			strncpy( class, argv[ i ], MAXCLASSNAME - 1 );
		}
		else if( strnequ( argv[ i ], "-s", 2 ) ) 
        {
            /* Check to see if the server is specified by the user */
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					"error: missing server name\n" );
				printusage( prog );
			}
			strncpy( host, argv[ i ], MAXHOSTNAME - 1 );
		}
        else if( strnequ(argv[i], "-2", 2) ) 
        {
            /* Check to see if user wants the second serial of target */
            i++;
            getserial2 = 1;
        }
		else if( argv[ i ][ 0 ] == '-' ) {
			fprintf( stderr, "error: unexpected argument '%s'\n",
				   argv[ i ] );
			printusage( prog );
		}
		else {
			strncpy( connection, argv[ i ], MAXCONNECTIONNAME-1 );
		}
	}

	setSignals();

	if( ( g_dev = 
            makeConnection( connection, class, host, getserial2 ) ) < 0 ) 
    {
		exit( 1 );
	}

	fprintf( stdout, "connection '%s', class '%s', host '%s'\n",
		 connection, class, host );

#ifdef KEEPALIVE
	if( ( g_sock = connectUDP( host, CS_PORT ) ) < 0 ) {
		exit( 1 );
	}

	initRequest( & g_req, REQ_TOUCHCONNECTION,
		     connection, class );

	alarm( SECONDS_DELAY );
#endif

	if( transparent ) {
		connectto( g_dev, flushdata, prog, flushc, 1 );
	}
	else {
		setTTY();
		connectto( g_dev, flushdata, prog,  flushc, 1 );
		restoreTTY();
	}

	printf( "\r\n" );

	exit( 0 );
}

static int handlebreak(int devout)
{
	char ch;

	printf( "\r\n" );
	while( 1 ) {
		printf( "\r\n(command-mode) " );
		fflush(stdout);
		if( getInput( & ch ) <= 0 ) 
		{
			printf( "\r\n" );
			return( 0 );
		}
		ch &= 0x7F;
		printf( "%c\r\n", ch );
		switch( ch ) {
		case '?':
		case 'h':
			printf ("h, ?\t: help message\r\n");
			printf ("b\t: send break\r\n");
			printf ("c\t: continue session\r\n");
			printf ("z\t: suspend\r\n");
			printf ("d\t: download image\r\n");
			printf ("p\t: powercycle backend\r\n");
			printf ("n\t: power on backend\r\n");
			printf ("f\t: power off backend\r\n");
			printf ("s\t: spawn a program\r\n");
			printf ("x\t: quit and leave power on\r\n"); 
			printf ("q\t: quit and power down\r\n");
			break;
			
		case '\n':
		case '\r':
			break;

		case '\004':
		case 'q':
			restoreTTY();
			if( subconnect( connection, "POWEROFF", "-pf", host,
					0 ) == 0 ) 
			{
				setTTY();
				return( 1 );
			}
			setTTY();
			return( 0 );

		case 'x':
			return( 0 );

		case 'c':
			return( 1 );

		case 'z':
			restoreTTY();
			resetSignals();
			kill( getpid(), SIGTSTP );
			setSignals();
			setTTY();
#ifdef KEEPALIVE
			alarm( SECONDS_DELAY );
#endif
			break;
			
		case 'b':
			write( devout, "\\b", 2 );
			printf( "\r\n" );
			return( 1 );
			break;

		case 'd':
			restoreTTY();
			if( subconnect( connection, "DOWNLOAD", "-dl", host,
					1 ) == 0 ) 
			{
				setTTY();
				return( 1 );
			}
			setTTY();
			break;
			
		case 'p':
			restoreTTY();
			if( subconnect( connection, "POWERCYCLE", "-pc", host,
						    0 ) == 0 )
			{
				setTTY();
				return( 1 );
			}
			setTTY();
			break;

		case 'f':
			restoreTTY();
			if( subconnect( connection, "POWEROFF", "-pf", host,
					0 ) == 0 ) 
			{
				setTTY();
				return( 1 );
			}
			setTTY();
			break;

		case 'n':
			restoreTTY();
			if( subconnect( connection, "POWERON", "-pn", host,
					0 ) == 0 ) 
			{
				setTTY();
				return( 1 );
			}
			setTTY();
			break;
			
		case 's':
			restoreTTY();
			spawnconnect( devout );
			setTTY();
			break;
			
		default:
			printf( "Illegal option '0x%x'.  Type h for help\r\n",
			        (unsigned char) ch );
			break;
		}
	}
}

#define MAXFILENAME	256

static int getFilename(char *last, char *env, char *def_file) 
{
	char filename[ MAXFILENAME ];
		
	printf( "file: " );
	if( getInputString( filename, sizeof( filename ) ) < 0 ) {
		return( -1 );
	}

	if( filename[ 0 ] == '\0' ) {
		char * sb;

		if( last[ 0 ] != '\0' ) {
			strncpy( filename, last, MAXFILENAME - 1 );
		}
		else if( env != NULL && ( ( sb = getenv( env ) ) != NULL ) ) {
			strncpy( filename, sb, MAXFILENAME - 1 );
		}
		else if( def_file != 0 ) {
			strncpy( filename, def_file, MAXFILENAME - 1 );
		}
	}

	filename[ MAXFILENAME - 1 ] = '\0';

	if( *filename == '\0' ) {
		return( -1 );
	}
		
	strcpy( last, filename );

	return( 0 );
}

#define ENVFILENAME	"CS_FILENAME"

#define DEFAULTFILENAME	"a.out"

int subconnect(char *connection, char *class, char *connection_ext, 
	       char *host, int getfile )
{
	int childpid;
	int status;
	char * file = 0;
	
	if( getfile ) {
		static char last[ MAXFILENAME ];
		static int init = 0;

		if( ! init ) {
			last[ 0 ] = '\0';
			init = 1;
		}

		if( getFilename( last, ENVFILENAME, DEFAULTFILENAME ) != 0 ) {
			return( -1 );
		}

		file = last;
	}
			
	if( ( childpid = fork() ) == 0 ) {
		char con[ 128 ];

#ifdef KEEPALIVE
		close( g_sock );
#endif
		close( g_dev );

		sprintf( con, "%s%s", connection, connection_ext );

		if( getfile ) {
			int fd;
			
			if( ( fd = open( file, O_RDONLY ) ) < 0 ) {
				perror( "open()" );
				exit( 1 );
			}
			
			if( dup2( fd, 0 ) < 0 ) {
				perror( "dup2()" );
				exit( 1 );
			}
			close( fd );
		}
		
		execlp( prog, prog, "-t", "-f", "-c", class, "-s", host, con,
			NULL );
		
		fprintf( stderr, "execlp(%s) failed\n", prog );

		exit( 1 );
	}
			
	if( childpid == -1 ) {
		fprintf( stderr, "error: fork() failed\n" );
		return( -1 );
	}
	
	status = 0;

	wait( & status );
	
	return( status );
}

int spawnconnect(int dev)
{
	int childpid;
	int status;
	char * file = 0;
	
	{
		static char last[ MAXFILENAME ];
		static int init = 0;

		if( ! init ) {
			last[ 0 ] = '\0';
			init = 1;
		}

		if( getFilename( last, 0, DEFAULTFILENAME ) != 0 ) {
			return( -1 );
		}

		file = last;
	}
	if( ( childpid = fork() ) == 0 ) {
		if( dev != 4 ) {
			if( dup2( dev, 4 ) < 0 ) {
				perror( "dup2()" );
				exit( 1 );
			}
			close( dev );
		}
		
		resetSignals();

		execlp( file, file, NULL );
		
		fprintf( stderr, "execlp(%s) failed\n", file );
		exit( 1 );
	}
			
	if( childpid == -1 ) {
		fprintf( stderr, "error: fork() failed\n" );
		return( -1 );
	}
	
	status = 0;
	wait( & status );
	
	return( status );
}

static unsigned char * stdin_buf = 0;
static int stdin_len = 0;
static int stdin_curr = 0;

static int getInput(char *pch)
{
	int rv;
	
	if( stdin_curr < stdin_len ) {
		*pch = (char) stdin_buf[ stdin_curr++ ];
		return( 1 );
	}
	if( ( rv = read( fileno( stdin ), pch, 1 ) ) == -1 ) {
		clearerr( stdin );
		return( -1 );
	}
	return( rv );
}

static int getInputString(char *str, int maxlen)
{
	str[ 0 ] = '\0';
	str[ maxlen - 1 ] = '\0';

	errno = 0;
	if( fgets( str, maxlen - 1, stdin ) == NULL ) {
		clearerr( stdin );
		return( -1 );
	}

	if( str[ 0 ] != '\0' ) {
		if( str[ strlen( str ) - 1 ] == '\n' ) {
			str[ strlen( str ) - 1 ] = '\0';
		}
	}
	
	return( 1 );
}

static int flush2net(int devout, char *buf, int len)
{
	int i;
	int start;

	stdin_buf = (unsigned char *) buf;
	stdin_len = len;
	stdin_curr = 0;

	for( i = 0; i < stdin_len; i++ ) {
		stdin_buf[ i ] &= 0x7F;
	}

#ifdef KEEPALIVE
	g_gotinput = 1;
#endif

	start = stdin_curr;
	while( stdin_curr < stdin_len ) {
		unsigned char ch = stdin_buf[ stdin_curr ];

		stdin_curr++;
		
		if( ((char) ch) == '\\' ) {
			int clen = stdin_curr - start;

			if( write( devout, stdin_buf + start, clen ) < clen ) {
				return( start );
			}
			if( write( devout, "\\", 1 ) != 1 ) {
				return( start );
			}
			
			start = stdin_curr;
		}
		else if( ch == ESC_CHARACTER ) {
			int rv;
			
			int clen = stdin_curr - start - 1;

			if( clen > 0 ) {
				if( write( devout, stdin_buf + start,
					   clen ) < clen ) {
					return( start );
				}
			}
			if( ( rv = handlebreak( devout ) ) <= 0 ) {
				return( rv );
			}
			start = stdin_curr;
		}
	}
	if( stdin_curr != start ) {
		int clen = stdin_curr - start;
		if( write( devout, stdin_buf + start, clen ) < clen ) {
			return( start );
		}
	}
	return( len );
}

static int flushdata(int devout, char *buf, int len)
{
	if( transparent ) {
		/* writing transparently */
		
		return( write( devout, buf, len ) );
	}
	else if( devout == fileno( stdout ) ) {
		/* writing to stdout */
		
		int i;

		for( i = 0; i < len; i++ ) {
			buf[ i ] &= 0x7F;
		}
		return( write( devout, buf, len ) );
	}
	else {
		/* writing to network */
		
		return( flush2net( devout, buf, len ) );
	}
}
