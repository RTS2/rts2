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

struct device;

/*!
 * Status hander for all status changes. Used only in client
 * applications.
 */
typedef int (*general_notifier_t) (struct device * dev, char *status_name,
				   int new_value);

/*! 
 * Parameters for one communication channel.
 * 
 * Channel is in our terminology one communication link, which goes between
 * client and device server. It has some address, use some socket, have
 * handler for devdem commands returns and messages, space to hold extra 
 * parameters for such a handler, and some other usefull variables.
 */
struct dev_channel
{
  int socket;			//! socket for connection
  struct sockaddr_in address;	//! socket address
  pthread_t read_thread;	//! read thread
  pthread_mutex_t used;		//! when locked, that channel is used
  pthread_mutex_t ret_lock;	//! return lock, for ret_cond
  pthread_cond_t ret_cond;	//! used to signal command return
  int ret_code;			//! to store last return code
  struct devcli_channel_handlers handlers;	//! response handlers
};

struct device
{
  char name[DEVICE_NAME_SIZE];
  char hostname[DEVICE_URI_SIZE];
  unsigned int port;
  int type;
  int key;			// authorization key
  int status_num;		//! hold status number
  union devhnd_info info;	//! info structure
  struct devconn_status *statutes;	//! holds status informations
  pthread_mutex_t status_lock;	//! lock status change informations
  pthread_cond_t status_cond;	//! signalize status change
  pthread_mutex_t priority_lock;
  pthread_cond_t priority_cond;
  int priority;
  struct dev_channel *channel;
  devcli_handle_data_t data_handler;	//! handler to received data
  general_notifier_t status_notifier;	//! status change handler
  void *notifier_data;		//! notifier data
  struct device *next;
};

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
