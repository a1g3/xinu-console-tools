#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

/* Include file control header */
#ifdef DEC
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#include <ctype.h>

static int flushdata();

static char * prog = 0;
static int g_dev = -1;

/**
 * Closes tty and exits application.
 * @param i unused
 */
static void quit(int i)
{
	restoretty(fileno(stdin));
	exit(0);
}

/**
 * Sets actions to take when signals are received.
 */
static setSignals()
{
#ifdef SOLARIS
	{
		/* Set signal flags and handlers in a Solaris environment */
		struct sigaction sa;
		
		sigemptyset(&(sa.sa_mask));
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = quit;
		(void) sigaction(SIGQUIT, & sa, 0);
		(void) sigaction(SIGINT, & sa, 0);
		sa.sa_handler = SIG_IGN;
		(void) sigaction(SIGTSTP, & sa,  );
		(void) sigaction(SIGPIPE, & sa,  );
		sa.sa_handler = SIG_DFL;
		(void) sigaction(SIGALRM, & sa,  );
	}
#else
	/* Set signal handlers in all other environments */
	signal(SIGQUIT, quit);
	signal(SIGINT, quit);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
#endif
}

/**
 * Prints the usage of tty-connect.
 * @param sb
 */
static void printusage(char *sb)
{
	fprintf(stderr, "usage: %s [-t] [-b] [-f] [-r baudrate] tty\n", sb);
	exit(1);
}

#define FLUSH_TIMEOUT 2

static int transparent = 0;
static int cook = 0;

/**
 * Connects to a tty at a specified buadrate.
 * @param argc number of command line arguments
 * @param argv command line arguments
 */
main(int argc, char **argv)
{
	char *ttyname = NULL;   /* name of the tty                  */
	char *ttyrate = "9600"; /* tty connection baud rate, default is 9600 */
	int i;
	int flushc;
	int sbrk;

	prog = argv[0];
	flushc = 0;
	sbrk = 0;
	cook = 0;

	/* Adjust settings based on command line options */
	for (i = 1; i < argc; i++) 
	{
		/* Enable transparent mode */
		if (strcmp(argv[i], "-t") == 0)
		{
			transparent = 1;
		}
		/* Enable send break immediately after opening the tty */
		else if (strcmp(argv[i], "-b") == 0) 
		{
			sbrk = 1;
		}
		/* Enable flush timeout, deprecated */
		else if (strcmp(argv[ i ], "-f") == 0) 
		{
			flushc = FLUSH_TIMEOUT;
		}
		/* Enable cooked mode */
		else if (strcmp(argv[ i ], "-c") == 0)
		{
			cook = 1;
		}
		/* Specify baud rate */
		else if (strcmp( argv[i], "-r" ) == 0) 
		{
			i++;
			/* Ensure baud rate is given after flag */
			if (i >= argc) 
			{
				fprintf(stderr,	"error: missing baudrate\n");
				printusage(prog);
			}
			ttyrate = argv[i];
		}
		/* Invalid argument */
		else if (argv[i][0] == '-') 
		{
			fprintf( stderr, "error: unexpected argument '%s'\n",
				 argv[i]);
			printusage(prog);
		}
		/* Set tty name */
		else 
		{
			ttyname = argv[ i ];
		}
	}
	
	/* Ensure tty name was provided */
	if (ttyname == NULL) {
		fprintf(stderr, "missing terminal name\n");
		printusage(prog);
	}

	setSignals();

	/* Check if standard input is a tty */
	if (isatty(fileno(stdin))) 
	{
		/* Ensure standard in allows c-break mode */
		if (cbreakmode(fileno(stdin)) != 0) 
		{
			fprintf( stderr, "%s: cannot set terminal modes\n",
				 argv[0] );
			exit(1);
		}
	}

	/* Open tty */
	if ((g_dev = OpenTTYLine(ttyname, ttyrate)) < 0) 
	{
		fprintf(stderr, "%s: cannot set tty line\n", argv[ 0 ]);
		exit(1);
	}

	/* Send a break across the tty */
	if (sbrk) 
	{
		SendTTYBreak(g_dev);
		sleep( 1 );
	}
	else 
	{
		connectto(g_dev, flushdata, prog, flushc, 0);
	}

	quit(0);
}

#define PC_LEN 1024              /* Output buffer length                  */

static int pc_dev = -1;          /* Device to output characters to        */
static int pc_rv = 0;
static int pc_len = 0;           /* Current number of chars in the buffer */
static char pc_buf[ PC_LEN ];    /* Output buffer                         */

/**
 * Prepares to output characters to a secified device.
 * @param dev device to output characters
 */
static void PutCharStart(int dev)
{
	pc_dev = dev;
	pc_rv = 0;
	pc_len = 0;
}

/**
 * Flushes the output buffer to the device.
 * @return 1 if the buffer was flushed successfully, otherwise 0
 */
static int PutCharFlush()
{
	if (pc_len > 0) {
		int rv = write(pc_dev, pc_buf, pc_len);
		if (rv <= 0) 
		{
			pc_rv -= pc_len;
			return 0;
		}
		else if (rv < pc_len) 
		{
			pc_rv -= (pc_len - rv);
			return 0;
		}
	}
	pc_len = 0;
	return 1;
}

/**
 * Puts a character into the output buffer.
 * @param ch character to output
 * @return 1 if the character was placed in the buffer, otherwise 0
 */
static int PutChar(char ch)
{
	if (pc_len >= PC_LEN) 
	{
		if (!PutCharFlush()) 
		{ return 0; }
	}
	pc_buf[pc_len] = ch;
	pc_len++;
	pc_rv++;
	return 1;
}

/**
 * Stops outputing characters to the device.
 */
static int PutCharEnd()
{
	int rv;
	
	PutCharFlush();
	
	rv = pc_rv;
	
	pc_dev = -1;
	pc_rv = 0;
	
	return rv;
}

/**
 * Flushes the output buffer to the device.
 * @param devout output device
 * @param *buf character buffer
 * @param len number of characters in the buffer
 * @return result of output to device
 */
static int flushdata(int devout, char *buf, int len)
{
	if (transparent) 
	{
		/* writing transparently */
		
		return write(devout, buf, len);
	}
	else if (devout == fileno(stdout)) 
	{
		/* writing to stdout */
		
		return write(devout, buf, len );
	}
	else 
	{
		/* writing to tty - process escaped commands */
		
		static int esc = 0;
	
		int e = 0;

		PutCharStart(devout); 

		while (len-- > 0) 
		{
			char ch = *buf++;

			if (!esc) 
			{
				if (ch == '\\') 
				{
					esc = 1;
					e++;
					continue;
				}
				else if (cook)
				{
					/* Replace \n with \r\n */
					if (ch == '\n')
					{ PutChar('\r'); }
				}
			}
			else if (esc) 
			{
				esc = 0;
				if (ch == 'b') 
				{
					SendTTYBreak(devout);
					e++;
					continue;
				}
				/* fall through and output character */
			}
			
			if (!PutChar(ch)) 
			{
				return (PutCharEnd() + e);
			}
		}
		return (PutCharEnd() + e);
	}
}
