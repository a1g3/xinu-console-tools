#include <stdio.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------
 * xmalloc - test return value on malloc
 *---------------------------------------------------------------------------
 */
char *xmalloc(int size)
{
	char * p;
	
	if( ( p = malloc( size ) ) == NULL ) {
		fprintf( stderr, "malloc failed\n" );
		exit( 1 );
	}
	return( p );
}

