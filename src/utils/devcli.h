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
#include "devhnd.h"
#include "image_info.h"
#include "param.h"
#include <pthread.h>
#include <netinet/in.h>

/*! 
 * Parameters for one communication channel.
 * 
 * Chanell is in our terminology one communication link, which goes between
 * client and device server. It has some address, use some socket, have
 * handler for devdem commands returns and messages, space to hold extra 
 * parameters for such a handler, and some other usefull variables.
 */

int devcli_server_login (const char *hostname,
			 uint16_t port, char *login, char *password);
int devcli_server_register (const char *hostname, uint16_t port,
			    char *device_name, int device_type,
			    char *device_host,
			    uint16_t device_port,
			    struct devcli_channel_handlers *handlers);
void devcli_server_close (int channel_id);
void devcli_server_disconnect ();
int devcli_connectdev (int *channel_id, const char *dev_name,
		       devcli_handle_data_t data_handler);

ssize_t devcli_read_data (int sock, void *data, size_t size);

int devcli_wait_for_status (char *device_name, char *status_name,
			    int status_mask, int status, time_t tmeout);
int devcli_server_command (int *ret_code, char *cmd, ...);
int devcli_command (int channel_id, int *ret_code, char *cmd, ...);
int devcli_image_info (int channel_id, struct image_info *image);
int devcli_execute (char *line, int *ret_code);
int devcli_getinfo (int channel_id, union devhnd_info *info);

#endif // __RTS_DEVCLI__
