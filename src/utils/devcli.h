/*! 
 * @file Client header file.
 * $Id$
 *
 * Defines basic functions for accessing client.
 *
 * @author petr
 */

#ifndef __RTS_DEVCLI__
#define __RTS_DEVCLI__

#include "devconn.h"
#include "param.h"
#include <pthread.h>

typedef int (*devcli_handle_response_t) (struct param_status * params);
typedef int (*devcli_handle_data_t) (int socket, size_t size);

/*! 
 * Parameters for one communication channel.
 * 
 * Chanell is in our terminology one communication link, which goes between
 * client and device server. It has some address, use some socket, have
 * handler for devdem commands returns and messages, space to hold extra 
 * parameters for such a handler, and some other usefull variables.
 */

struct devcli_channel_handlers
{
  devcli_handle_response_t command_handler;	//! handler to handle responses
  devcli_handle_response_t message_handler;	//! handler to asynchoronous messages
  devcli_handle_data_t data_handler;	//! handler to ANY received data
};

int devcli_server_login (const char *hostname,
			 uint16_t port, char *login, char *password);
int devcli_server_register (const char *hostname, uint16_t port,
			    char *device_name, int device_type,
			    uint16_t device_port,
			    struct devcli_channel_handlers *handlers);
void devcli_server_close (int channel_id);
int devcli_connectdev (int *channel_id, const char *dev_name,
		       struct devcli_channel_handlers *handlers);

size_t devcli_read_data (int socket, void *data, size_t size);

int devcli_command (int channel_id, char *cmd, ...);

int devcli_server_command (char *cmd, ...);

#endif // __RTS_DEVCLI__
