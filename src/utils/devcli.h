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
#include <pthread.h>

typedef int (*devcli_handle_response_t) (char *, size_t);
typedef int (*devcli_handle_data_t) (void *, size_t, void *);

/*! 
 * Parameters for one communication channel
 * 
 * Chanell is in our terminology one communication link, which goes between
 * client and device server. It has some address, use some socket, have
 * handler for devdem commands returns, space to hold extra parameters for
 * such a handler, and some other usefull variables.
 */

struct devcli_channel_t
{
  int socket;			//! socket for connection
  struct sockaddr *address;	//! socket address
  devcli_handle_response_t handler;	//! handler to handle responses
  void *response_data;		//! attribute to send to response handler
  // following are internal variables
  char buf[MAXMSG + 1];		//! start buffer for commands
  char *endptr;			//! buffer end ptr
};


int devcli_init_channel (struct devcli_channel_t *channel,
			 const char *hostname, uint16_t port);

int devcli_write_read (struct devcli_channel_t *channel, char *message,
		       int *ret_code);
int devcli_read_data (struct sockaddr *server_addr, pthread_t * thread,
		      devcli_handle_data_t handler, void *handler_arg);

#endif // __RTS_DEVCLI__
