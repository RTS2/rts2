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

struct devcli_channel
{
  devcli_handle_response_t command_handler;	//! handler to handle responses
  devcli_handle_response_t message_handler;	//! handler to asynchoronous messages
  devcli_handle_data_t data_handler;	//! handler to ANY received data
  int socket;			//! socket for connection
  struct sockaddr *address;	//! socket address
  pthread_t read_thread;	//! read thread
  pthread_mutex_t ret_lock;	/*! 
				 * return lock
				 * <ul>
				 *      <li>locked</li> waiting for ret 
				 *      <li>unlocked</li> not used ret
				 *                      received
				 * </ul>
				 */
  pthread_t data_thread;	//! data readout thread
  int ret_code;			//! to store last return code
};


int devcli_connect (struct devcli_channel *channel, const char *hostname,
		    uint16_t port);

int devcli_command (struct devcli_channel *channel, char *message);

size_t devcli_read_data (int socket, void *data, size_t size);

#endif // __RTS_DEVCLI__
