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

int devcli_init_sockaddr (struct sockaddr_in *name, const char *hostname,
			  uint16_t port);

devcli_handle_response_t devcli_set_response_handler (devcli_handle_response_t
						      new_handler);
int devcli_write_read (int sock, char *message, int *ret_code);
int devcli_read_data (struct sockaddr *server_addr, pthread_t * thread,
		      devcli_handle_data_t handler, void *handler_arg);

#endif // __RTS_DEVCLI__
