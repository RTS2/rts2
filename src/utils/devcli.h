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

#include "serverd_info.h"

#include <pthread.h>
#include <netinet/in.h>


int devcli_server_login (const char *hostname,
			 uint16_t port, char *login, char *password);
int devcli_server_register (const char *hostname, uint16_t port,
			    char *device_name, int device_type,
			    char *device_host,
			    uint16_t device_port,
			    struct devcli_channel_handlers *handlers,
			    status_notifier_t notifier);
void devcli_server_close (struct device *dev);
void devcli_server_disconnect ();
struct device *devcli_server ();
struct device *devcli_devices ();
struct device *devcli_find (const char *device_name);
ssize_t devcli_read_data (int sock, void *data, size_t size);

int devcli_wait_for_status (struct device *dev, char *status_name,
			    int status_mask, int status, time_t tmeout);
void devcli_set_general_notifier (struct device *dev,
				  general_notifier_t notifier, void *data);
int devcli_set_notifier (struct device *dev, char *status_name,
			 status_notifier_t callback);
int devcli_server_command (int *ret_code, char *cmd, ...);
int devcli_command (struct device *dev, int *ret_code, char *cmd, ...);
int devcli_image_info (struct device *dev, struct image_info *image);
int devcli_execute (char *line, int *ret_code);

#endif // __RTS_DEVCLI__
