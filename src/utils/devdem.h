/*! 
 * @file Device deamon header file.
 * $Id$
 *
 * @author petr
 */

#ifndef __RTS_DEVDEM__
#define __RTS_DEVDEM__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "devcli.h"
#include "devser.h"

int devdem_priority_block_start ();
int devdem_priority_block_end ();

int devdem_status_message (int subdevice, char *description);
int devdem_status_mask (int subdevice, int mask, int operand, char *message);

int devdem_init (char **status_names, int status_num_in);
int devdem_register (struct devcli_channel *server_channel, char *device_name,
		     char *server_address, int server_port);
int devdem_run (int port, devser_handle_command_t in_handler);

#include "devser.h"

#endif /* !__RTS_DEVDEM__ */
