/**
 * XINU Remote Power Control functions
 *
 * These functions provide an interface to the remote power control device used
 * in the Embedded XINU Lab, which is currently a Baytech RPC22D-20NC.
 *
 * Currently, the implementation is based on invoking existing GNU expect
 * scripts which actually handle the real work of the serial communication with
 * the device.  The goal of this file is to provide a clean API such that the
 * underlying implementation can be improved in the future.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <connect.h>
#include <syslog.h>

/* buffer to build system commands to run expect scripts */
#define CMDBUFLEN 128

#define TTY_DEV     "/dev/tty115"
#define BAUD_RATE   "38400"

static int g_dev = -1;

/**
 * Initialize the RPC device and associated variables to a known stat.
 * @return -1 on error, 0 or more on success
 */
int rpc_init(void)
{
	g_dev = OpenTTYLine(TTY_DEV, BAUD_RATE);
	if (-1 == g_dev) return -1;

	return rpc_alloff();
}

/**
 * Turn off all outlets on device.
 * @return return value of powerdown-backends script TODO: what is this?
 */
int rpc_alloff(void)
{
	char commandbuf[CMDBUFLEN];
	sprintf(commandbuf, "off all\r\n");	
	syslog(LOG_INFO, "executing command %s\n", commandbuf);
	return write(g_dev, commandbuf, strlen(commandbuf));
}

/**
 * Turn off a specified outlet.
 * @arg num zero-based outlet number
 * @return return value of expect script TODO: what is this?
 */
int rpc_off(int num)
{
    char commandbuf[CMDBUFLEN];
	sprintf(commandbuf, "off %d\r\n", num);	
	syslog(LOG_INFO, "executing command %s\n", commandbuf);
	return write(g_dev, commandbuf, strlen(commandbuf));
}

/**
 * Turn off a specified outlet.
 * @arg num zero-based outlet number
 * @return return value of expect script TODO: what is this?
 */
int rpc_on(int num)
{
    char commandbuf[CMDBUFLEN];
	sprintf(commandbuf, "on %d\r\n", num);	
	syslog(LOG_INFO, "executing command %s\n", commandbuf);
	return write(g_dev, commandbuf, strlen(commandbuf));
}

/**
 * Reboot a specified outlet.
 * @arg num zero-based outlet number
 * @return return value of expect script TODO: what is this?
 */
int rpc_reset(int num)
{
    char commandbuf[CMDBUFLEN];
	sprintf(commandbuf, "off %d\r\n", num);
	syslog(LOG_INFO, "executing command %s\n", commandbuf);
	write(g_dev, commandbuf, strlen(commandbuf));
	sleep(1);
	sprintf(commandbuf, "on %d\r\n", num);
	syslog(LOG_INFO, "executing command %s\n", commandbuf);
	return write(g_dev, commandbuf, strlen(commandbuf));
}
