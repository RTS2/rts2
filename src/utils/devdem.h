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

#define DEVDEM_WITH_CLIENT

#include "devcli.h"

int devdem_priority_block_start ();
int devdem_priority_block_end ();

int devdem_register (struct devcli_channel *server_channel, char *device_name,
		     char *server_address, int server_port);

#include "devser.h"

#endif /* !__RTS_DEVDEM__ */
