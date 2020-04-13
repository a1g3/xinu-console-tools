/* 
 * connect.h - function prototypes for the connection library.
 */
#ifndef __connect_h__
#define __connect_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "cserver.h"

int breakConnection(char *, char *, char *);
int recvfromReply(int, struct sockaddr_in *, int, struct reply *);
int recvReply(int, int, struct reply *);
int sendtoRequest(int, struct sockaddr_in *, struct request *);
int sendRequest(int, struct request *);
void initRequest(struct request *, char, char *, char *);
void initReply(struct reply *, char, char *, int);
int reqGeneric(char *, char *, char *, int, struct reply *);
void connectto(int, int (*)(int, char *, int), char *, int, int);
char *getuser(void);

char *getdfltClass(char *, char *);
int makeConnection(char *, char *, char *, int);

int readTCP(int);
int connectUDP(char *, int);
int connectTCP(char *, int);
int passiveUDP(int);
int passiveTCP(int, int);
int acceptTCP(int);
char *sockgetstr(int, char *, int);
int connectsock(char *, int, char *);
int passivesock(int, char *, int);
int bcastUDP(int, char *, int, int);
int readDelay(int, int);
int writeDelay(int, int);

int statusrequest(char *, char *, char *);
int statusrecv(int, struct reply *);
int obtainStatus(char *, char *, char *, struct reply **, int);

void initttys(void);
void rawtty(int);
void cbreakmode(int);
void restoretty(int);
void tty_atexit(void);

int OpenTTYLine(char *, char *);
int SendTTYBreak(int);

char *xmalloc(int);

#endif
