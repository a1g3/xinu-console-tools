
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

#include "cserver.h"
#include "connect.h"

/* getlogin does not work when odt, or download is used within a 	*/
/* 'script' session - so we must use getpwuid like whoami does		*/

static char * g_getuser = NULL;

/*---------------------------------------------------------------------------
 * getuser - get the user's name
 *---------------------------------------------------------------------------
 */
char *getuser(void)
{
	if( g_getuser == NULL ) {
		struct passwd * pp;

		pp = getpwuid( geteuid() );
		if( pp == (struct passwd *) NULL ) {
			fprintf( stderr, "getuser() failed\n" );
			exit( 1 );
		}
		g_getuser = (char *) xmalloc( MAXUSERNAME );
		strncpy( g_getuser, pp->pw_name, MAXUSERNAME - 1 );
		g_getuser[ MAXUSERNAME - 1 ] = '\0';
	}
	return( g_getuser );
}
