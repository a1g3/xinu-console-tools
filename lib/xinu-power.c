/**
 * @file xinu-power.c
 * Definitions used in both client and server for XINU Power Daemon.
 */
#include <xinu-power.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>

/**
 * Checks to see whether a given string represents a recognized command.
 * Commands should look like: [a-z]([0-9])([0-9]) or "off".
 * @arg cmd string to check, must be null terminated
 * @return 1 if valid, 0 if invalid
 */
int valid_command(char *cmd)
{
	char c;
	int num;

	/* All commands have same length: VALID_CMD_LEN */
	if(strlen(cmd) != VALID_CMD_LEN) { return 0; }

	/* Check for special case "off" command */
	if(strncmp(cmd, "off", VALID_CMD_LEN) == 0) return 1;

	c = cmd[0];
	if(c == 'u' || c == 'd' || c == 'r')
	{
		/* second two characters must be numeric */
		if( is_numeric(cmd[1]) && is_numeric(cmd[2]) )
		{
			num = atoi(cmd+1);
			/* they must form a number within NUM_OUTLETS */
			if( 1 <= num && num <= NUM_OUTLETS )
			{
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Send a message through a connected socket.
 * @arg connected_sockfd a properly initialized and connected socket file
 *                       descriptor (use init_socket and accept)
 * @arg msg a string to send as a message, must be null terminated
 * @return number of bytes sent on success, -1 on error
 */
int send_msg(int connected_sockfd, char *msg)
{
	int len, bytes_sent; 
	
	len = strlen(msg);
	bytes_sent = send(connected_sockfd, msg, len, 0);
	if( bytes_sent < 0 )
	{
		perror("send()");
		return -1;
	}
	return bytes_sent;
}

/**
 * Given a command, return a human readable success string.
 * @arg cmd command that was successfully executed, must be valid
 * @arg buf pointer to buffer to fill with success string
 */
void success_string(char *cmd, char *buf)
{
	if(strncmp(cmd, "off", VALID_CMD_LEN) == 0)
	{
		sprintf(buf, "All backends successfully powered off.");
	}
	else if( strncmp( "d", cmd, 1 ) == 0 )
	{
		sprintf(buf, "Backend %d powered off", atoi(cmd+1));
	}
	else if( strncmp( "u", cmd, 1 ) == 0 )
	{
		sprintf(buf, "Backend %d powered on", atoi(cmd+1));
	}
	else if( strncmp( "r", cmd, 1 ) == 0 )
	{
		sprintf(buf, "Backend %d powered off\r\nBackend %d powered on", atoi(cmd+1), atoi(cmd+1));
	}
}
