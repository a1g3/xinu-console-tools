/* 
 * xinu-status.c - prints both server and connection status
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cserver.h"
#include "connect.h"

void printusage(char *sb);
void printstatus(struct reply *reply, int fflag, int bflag);

int main(int argc, char **argv)
{
	int sock;
	int fflag = 0;
	int bflag = 0;
	char class[ MAXCLASSNAME ];
	char host[ MAXHOSTNAME ];
	char connection[ MAXCONNECTIONNAME ];
	int i;
    int numservers = 0;
    int count;
    char * servers[MAXSERVERS];

	host[ 0 ] = '\0';
	host[ MAXHOSTNAME - 1 ] = '\0';
	connection[ 0 ] = '\0';
	connection[ MAXCONNECTIONNAME - 1 ] = '\0';
	class[ 0 ] = '\0';
	class[ MAXCLASSNAME - 1 ] = '\0';

	for( i = 1; i < argc; i++ ) {
		if( strnequ( argv[ i ], "-h", 2 ) ) {
			printusage( argv[ 0 ] );
		}
		else if( strnequ( argv[ i ], "-b",2 ) ) {
			bflag = 1;
		}
		else if( strnequ( argv[ i ], "-f", 2 ) ) {
			fflag = 1;
		}
		else if( strnequ( argv[ i ], "-c", 2 ) ) {
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					"error: missing class name\n" );
				printusage( argv[ 0 ] );
			}
			strncpy( class, argv[ i ], MAXCLASSNAME - 1 );
		}
		else if( strnequ( argv[ i ], "-s", 2 ) ) {
			i++;
			if( i >= argc ) {
				fprintf( stderr,
					"error: missing server name\n" );
				printusage( argv[ 0 ] );
			}
			strncpy( host, argv[ i ], MAXHOSTNAME - 1 );
		}
		else if( argv[ i ][ 0 ] == '-' ) {
			fprintf( stderr, "error: unexpected argument '%s'\n",
				   argv[ i ] );
			printusage( argv[ 0 ] );
		}
		else {
			strncpy( connection, argv[ i ], MAXCONNECTIONNAME-1 );
		}
	}
	
	if( ! fflag && ! bflag ) {
		fflag = bflag = 1;
	}

	if( ( sock = statusrequest( connection, 
				    getdfltClass( class, connection ), 
				    host ) ) < 0 ) 
	  {
	    exit( 1 );
	  }

    /* Grab the number of servers present */
    servers[0] = NULL;
    numservers = getservers(servers, host);

    if(numservers > 0)
    {
        count = 0;
        /* Loop through getting the states of targets for each server */
        while( count < numservers )
        {
            struct reply reply;
            if( statusrecv( sock, & reply ) < 0 ) 
            {
                break;
            }
            else
            {
                printstatus( & reply, fflag, bflag );
            }
            count++;
        }
    }
    else
    {
        /* Servers enironment variable not set, use other means */
        while(1)
        {
            struct reply reply;
            if( statusrecv( sock, & reply ) < 0 ) 
            {
                break;
            }
            else
            {
                printstatus( & reply, fflag, bflag );
            }
        }

    }
	exit( 0 );
}

void printusage(char *sb)
{
	fprintf( stderr, "usage: %s [-b] [-f] [-c class] [-s server] [connection]\n", sb );
	fprintf( stderr,
		 "xinu-status v2.04 \"Millenium Edition\"\n");
	exit( 1 );
}

/*---------------------------------------------------------------------------
 * printName - print the name of the frontend (ignoring DOMAINNAME)
 *---------------------------------------------------------------------------
 */
static void printName(char *sb)
{
#ifdef DOMAINNAME
	int len = strlen( sb );
	int dlen = strlen( DOMAINNAME );
	char * end = sb + len;

	if( ( len > dlen ) && ( *(end - dlen - 1) == '.' ) &&
	    ( strcmp( end - dlen, DOMAINNAME ) == 0 ) ) {
		*(end - dlen - 1) = '\0';
	}
#endif
	printf( "%s:", sb );
}

/*
 *---------------------------------------------------------------------------
 * printstatus - print the status message recieved
 *---------------------------------------------------------------------------
 */
void printstatus(struct reply *reply, int fflag, int bflag)
{
	struct statusreplyData * stat;
	int i;
	int numc;
	
	if( fflag ) {
		printName( reply->hostname );

		if( bflag ) {
			printf( "\n" );
		}
	}

	numc = atoi( reply->numconnections );
	stat = (struct statusreplyData *) ( reply->details );
	for( i = 0; i < numc; i++, stat++ ) {
		if( bflag ) {
			if( fflag ) {
				printf( "    " );
			}
			printf( "%-16s %-16s user= %-16s time= %s\n",
			       stat->conname, stat->conclass,
			       stat->user, stat->contime );
		}
		else if( fflag ) {
			printf( " %s", stat->conname );
		}
	}
	if( fflag && ! bflag ) {
		printf( "\n" );
	}
}
