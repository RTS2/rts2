/*! 
 * @file Device deamon header file.
 * $Id$
 * @author petr
 */

#ifndef __RTS_DEVDEM__
#define __RTS_DEVDEM__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

typedef int (*devdem_handle_command_t) (char *, size_t);

int devdem_run (int port, devdem_handle_command_t handler);
int devdem_write_command_end (int retc, char *msg_format, ...);
int devdem_dprintf (const char *format, ...);
int devdem_send_data (struct in_addr *client_addr, void *data_ptr,
		      size_t data_size);
extern pid_t devdem_parent_pid;

#endif // __RTS_DEVDEM__
