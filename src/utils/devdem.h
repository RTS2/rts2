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

#ifdef _cplusplus
extern "C"
{
#endif

  int devdem_priority_block_start ();
  int devdem_priority_block_end ();

  int devdem_status_message (int subdevice, char *description);
  int devdem_status_mask (int subdevice, int mask, int operand,
			  char *message);

  int devdem_init (char **status_names, int status_num_in,
		   status_notifier_t server_notifier, int deamonize);
  int devdem_register (char *server_address, uint16_t server_port,
		       char *in_device_name, int device_type,
		       char *device_host, uint16_t device_port);
  void devdem_done (void);
  int devdem_run (uint16_t port, devser_handle_command_t in_handler);

#include "devser.h"

#ifdef _cplusplus
}
#endif

#endif				/* !__RTS_DEVDEM__ */
