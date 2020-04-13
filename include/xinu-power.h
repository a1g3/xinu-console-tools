/**
 * @file xinu-power.h
 *
 * Definitions needed by both the XINU Power Daemon and its client.
 */

#define VERSION "v0.0.4 [BETA]"

#define REBOOTDPORT 2023 // IANA reserved port: xinuexpansion3
#define REBOOTDIP "134.48.6.10"

#define NUM_OUTLETS 48
#define OFF 0
#define ON  1

#define VALID_CMD_LEN 3 // valid commands are "u##", "d##", "r##", "o"
#define BACKLOG 10     // how many pending cons socket subsystem will hold
#define RECVBUFLEN 128
#define EVER ;;        // for cute while(1) loops
#define MSG_HELLO   "Welcome to the XINU Console Power Daemon\n"
#define MSG_INVALID "Invalid command. Sorry!\n"
#define MSG_SUCCESS "Command executed successfully.\n"
#define CONNECT_RETRIES 5

/* used by valid_command() */
#define is_numeric(c)   ((c) >= '0' && (c) <= '9')

/* Prototypes for functions used in both files. */
int send_msg(int connected_sockfd, char *msg);
int valid_command(char *cmd);
void success_string(char *cmd, char *buf);
