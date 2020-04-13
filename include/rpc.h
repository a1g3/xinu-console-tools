/**
 * @file rpc.h
 *
 * Function prototypes for the remote power control library.
 */

int rpc_alloff(void);
int rpc_off(int num);
int rpc_on(int num);
int rpc_reset(int num);
