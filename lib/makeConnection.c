#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "cserver.h"

#include <time.h>

/*---------------------------------------------------------------------------
 * getMinutes -
 *---------------------------------------------------------------------------
 */
static int getMinutes(char *time)
{
	int hours, minutes, seconds;
	
	sscanf( time, "%d:%d:%d", & hours, & minutes, & seconds );
	if( hours > 0 ) {
		minutes = minutes + ( hours * 60 );
	}
	return( minutes );
}

/*---------------------------------------------------------------------------
 * reqTCPConnection - test if the correct connection and is available
 *---------------------------------------------------------------------------
 */
static int reqTCPConnection(char *connection, char *class, char *host,
			    int steal, struct statusreplyData *stat)
{
	struct reply reply;
	
	if( ( ! steal && stat->active ) || ( steal && ! stat->active ) ) {
		return( -1 );
	}
	
	if( steal ) {
#ifdef RESERVETIME
		int rv;
		
		if( getMinutes( stat->contime ) < RESERVETIME  ) {
			return( -1 );
		}
		/* connection time exceeded */
		if( reqGeneric( stat->conname, stat->conclass, host,
				REQ_BREAKCONNECTION, NULL ) < 0 ) {
			fprintf( stderr, "error: could not break connection\n" );
			return( -1 );
		}
		else
		{
			/* Break connection request sent,
 				 sleep for 500 microseconds */
			usleep(500);
		}
#else
		return( -1 );
#endif
	}
	
    if( reqGeneric( stat->conname, stat->conclass, host,
			REQ_MAKECONNECTION, & reply ) == 0 ) {
		if( connection != NULL ) {
			strcpy( connection, stat->conname );
		}
		if( class != NULL ) {
			strcpy( class, stat->conclass );
		}
		return( connectTCP( host, atoi( reply.details ) ) );
	}
	return( -1 );
}

/*---------------------------------------------------------------------------
 * getTCPConnection - 
 *---------------------------------------------------------------------------
 */
static int getTCPConnection(char *connection, char *class, char *host,
			    int steal, struct reply **replies, int cnt)
{
	int i;
	int free;
	int randgrab;
	
	for( i = 0; i < cnt; i++ ) {
		struct statusreplyData * stat = (struct statusreplyData *)
		    ( replies[ i ]->details );
		char * h = replies[ i ]->hostname;
		int j = 0;
		int numc = atoi( replies[ i ]->numconnections );
   
		/* Declare storage variables for free resources */
	 	int indeces[numc];
		int free = 0;
	


		/* Find all free target pool resources */
		for(j=0; j < numc; j++, stat++)
		{
#ifdef RESERVETIME
			if (steal && stat->active)
			{ 
            	if (getMinutes( stat->contime ) >= RESERVETIME)
                {
					indeces[free] = j;
					free++;
				}
			}
#endif
			if(!stat->active)
			{
				indeces[free] = j;
				free++;
			}
		}

		/* Seed a random number, grab random < free resources */
		srand(time(NULL));
		randgrab = rand() % free;

		for(j=0; j<free; j++)
		{
			/* Reset stat back to the top of reply details */
			stat = (struct statusreplyData *) (replies[i]->details);
			
			/* Use pointer arithmetic to move to proper status struct */
			stat += indeces[randgrab];

			/* Get a connection for the random resource */
			int fd = reqTCPConnection( connection, class,
							   replies[ i ]->hostname,
							   steal, stat );

			/* Ensure a connection was made, copy hostname and return */
			if( fd >= 0 ) {
				if( host != NULL ) {
					strcpy( host, replies[ i ]->hostname );
				}
				return( fd );
			}
			
			/* If failure try next target resource in free array */
			randgrab = (randgrab+1)%free;
		}
	}
	return( -1 );
}

/*---------------------------------------------------------------------------
 * getdfltClass
 *---------------------------------------------------------------------------
 */
char *getdfltClass(char *class, char *connection)
{
	char * sb;

	if( ( class != NULL ) && ( *class != '\0' ) ) 
	  {
	    return( class );
	  }
	if( ( connection != NULL ) && ( *connection != '\0' ) ) 
	  {
	    return( "" );
	  }
#ifdef ENVCLASSNAME
	if( ( sb = getenv( ENVCLASSNAME ) ) != NULL ) 
	  {
	    if( *sb == '\0' ) 
	      {
		return( DEFAULTCLASSNAME );
	      }	
	    return( sb );
	  }
#endif
	return( DEFAULTCLASSNAME );
}

/*---------------------------------------------------------------------------
 * getdfltSecClass
 *---------------------------------------------------------------------------
 */
char *getdfltSecClass()
{
	return( DEFAULTSECCLASSNAME );
}

static int totalAnswers(struct reply **replies, int cnt)
{
	int i = 0;
	int total = 0;
	
	for( ; i < cnt; i++ ) {
		struct statusreplyData * stat = (struct statusreplyData *)
		    ( replies[ i ]->details );
		total += atoi( replies[ i ]->numconnections );
	}
	return( total );
}



char *getuser(void);
/*---------------------------------------------------------------------------
 * findSecond - find the index of a target resource with matching username
 *---------------------------------------------------------------------------
 */
static int findSecond(struct reply **replies, int cnt, char *secname)
{
	int i = 0;
    int j = 0;
    int n = 0;
    int q = 0;
    int r = 0;
    int nmlen = -1;
    int listnum = -1;
    int n2 = 0;
    int cnt2 = -1;
    int total = 0;
    int taken = 0;
	char username[MAXUSERNAME];
    struct statusreplyData * stat;
    struct statusreplyData * stat2;
	struct reply * replies2[ MAXSERVERS ];

    /* Acquire the name of the user */
    memcpy(username, getuser(), MAXUSERNAME);
	
	for( ; i < cnt; i++ ) 
    {
        /* Cast the details to reply data   */
		stat = (struct statusreplyData *) ( replies[ i ]->details );

        /* Get the number of connection     */
        n = atoi(replies[i]->numconnections);
		total += n;
        
        for(j=0; j < n; j++)
        {  
            /* Does the user currently own this target resource */
            if(0 == strncmp(stat->user, username, MAXUSERNAME))
            {            
                /* Grab the name legnth of the connection target */
                nmlen = strnlen(stat->conname, MAXCONNECTIONNAME);

                /* Ensure that the name doesn't violate length constraints */
                if(nmlen >= MAXCONNECTIONNAME-2)
                {
                    fprintf(stderr, 
                    "Connection name violates length for second serial\r\n");
                    listnum = -1;
                    break;
                }            

                /* Create name of the secondary target based on primary */
                strncpy(secname, stat->conname, MAXCONNECTIONNAME);
                secname[nmlen] = '2';
                secname[nmlen+1] = '\0';

                /* Obtain the state of attempted secondary target */ 
    		    cnt2 = obtainStatus(secname, getdfltSecClass(), '\0', 
                                    replies2, cnt);

                /* Loop through the status reply for what should be */
                /* a single target in the reply.                    */
                for(q=0; q<cnt2 && 0==taken; q++)
                {
                    stat2 = (struct statusreplyData *) (replies2[i]->details);
                    n2 = atoi(replies2[i]->numconnections);
                    for(r=0; r<n2; r++)
                    {
                        /* Check if the target is already active */
                        if (1 == stat2->active)
                        {
                            taken = 1;
                            break;
                        }                     
                    }
                }

                /* Secondary target was taken, check remaining targets */
                if (1 == taken)
                {
                    taken = 0;
                    stat++;
                    continue;
                }

                /* Inactive secondary target discovered */                
                listnum = j; 
                break;
            }

            /* Advance to next status structure */
            stat++;
        }
    }
    
    /* Return the index of the found target, -1 if not found */
	return listnum;
}




/*---------------------------------------------------------------------------
 * grabSecond - find the name of second serial given an index
 *---------------------------------------------------------------------------
 */
static void grabSecond(struct reply **replies, int cnt, int sindex, 
                       char *connection)
{
    struct statusreplyData * stat;

    /* Cast the data of the replies to status structures    */	
	stat = (struct statusreplyData *) ( replies[0]->details );
            
    /* Advance to the matching stat structure */
    stat += sindex;

    /* Zero out the memory in the connection variable */
    bzero(connection, MAXCONNECTIONNAME);

    /* Copy over the secondary connection name of target resource */
    strncpy(connection, stat->conname, MAXCONNECTIONNAME);
        
	return;
}



/*---------------------------------------------------------------------------
 * makeConnection - make a tcp connection to the specific connection
 *---------------------------------------------------------------------------
 */
int makeConnection(char *connection, char *class, char *host, int getserial2)
{
	int fd;
	int cnt;
    int sindex;
	struct reply * replies[ MAXSERVERS ];
	char dclass[ MAXCLASSNAME ];
	
	if( ( host != NULL ) && ( *host != '\0' ) &&
	    ( connection != NULL ) && ( *connection != '\0' ) &&
	    ( class != NULL ) && ( *class != '\0' ) ) {
		struct reply reply;
		if( reqGeneric( connection, class, host, REQ_MAKECONNECTION,
				& reply ) == 0 ) {
			return( connectTCP( host, atoi( reply.details ) ) );
		}
		fprintf( stderr, "error: connection not available\n" );
		return( -1 );
	}
	else {
		if( class == NULL ) {
			strncpy( dclass, getdfltClass( class, connection ),
				 MAXCLASSNAME - 1 );
			class = dclass;
		}
		else {
			strncpy( class, getdfltClass( class, connection ),
				 MAXCLASSNAME );
		}
		class[ MAXCLASSNAME - 1 ] = '\0';


        /* Check if command line option specified use of a second serial */
        if (1 == getserial2)
        {
            /* Set the default class name */
            strncpy(dclass, getdfltClass('\0', '\0'), MAXCLASSNAME);
            class = dclass;

            /* Obtain the states of resources with default class name */
		    cnt = obtainStatus( '\0', class, host, replies,
				    MAXSERVERS );

            /* Find the name of the second serial based on the first */
            /* discovered first serial port with user's name.        */
            /* sindex is -1 if not found, otherwise target was found */
            sindex = findSecond(replies, cnt, connection);

            if(sindex != -1)
            {
                /* Set the class of the default secondary class name */
                *class = '\0';
                strncpy(dclass, getdfltSecClass(), MAXCLASSNAME);
                class = dclass;

                /* Obtain states of resources with secondary name */
                //cnt = obtainStatus( '\0', class, host, replies,
                //        MAXSERVERS );
            }

            /* Attempt to acquire second resource matching first failed */
            if (-1 == sindex)
            {                
                strncpy(dclass, getdfltClass('\0', '\0'), MAXCLASSNAME);
                class = dclass;
            }
        }

		cnt = obtainStatus( connection, class, host, replies,
                           MAXSERVERS );

		if(totalAnswers( replies, cnt ) <= 0 ) {
			fprintf( stderr, "error: connection not found\n" );
			return( -1 );
		}
		
		if( ( fd = getTCPConnection( connection, class, host, 0,
					     replies, cnt ) ) >= 0 ) {
			return( fd );
		}
		if( ( fd = getTCPConnection( connection, class, host, 1,
					     replies, cnt ) ) >= 0 ) {
			return( fd );
		}
		
		fprintf( stderr, "error: connection not available\n" );
		return( -1 );
	}
}
