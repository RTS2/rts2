#ifndef __RTS_SERVERD_INFO__
#define __RTS_SERVERD_INFO__

#include <pthread.h>
#include <netinet/in.h>
#include "devhnd.h"
#include "param.h"
#include "status.h"

struct device;

struct client;

typedef void (*response_handler_t) (struct device * dev, char *cmd);

struct serverd_info
{
  struct device *devices;
  struct client *clients;
  int id;
};

union devhnd_info
{
  struct telescope_info telescope;
  struct camera_info camera;
  struct serverd_info server;
};

typedef int (*devcli_handle_response_t) (struct param_status * params,
					 union devhnd_info * info);
typedef int (*devcli_handle_data_t) (int sock, size_t size,
				     struct image_info * image);

struct devcli_channel_handlers
{
  devcli_handle_response_t command_handler;	//! handler to handle responses
  devcli_handle_response_t message_handler;	//! handler to asynchoronous messages
  struct image_info image;	//! data about image data
};

/*!
 * Status hander for all status changes. Used only in client
 * applications.
 */
typedef int (*general_notifier_t) (struct device * dev, char *status_name,
				   int new_value);

struct client
{
  int id;
  char login[CLIENT_LOGIN_SIZE];
  int priority;
  char have_priority;
  char status_txt[MAX_STATUS_TXT];
  struct client *next;
};

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
  pthread_mutex_t channel_socket_lock;	//! lock for channel access 
  struct sockaddr_in address;	//! socket address
  pthread_t read_thread;	//! read thread
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
  int priority;
  response_handler_t response_handler;	//! response callback
  struct dev_channel channel;
  devcli_handle_data_t data_handler;	//! handler to received data
  general_notifier_t status_notifier;	//! status change handler
  void *notifier_data;		//! notifier data
  struct device *next;
};


#endif /* __RTS_SERVERD_INFO__ */
