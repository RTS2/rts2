/*! 
 * @file Client header file.
 * $Id$
 *
 * Defines basic functions for accessing client.
 *
 * @author petr
 */

typedef int (*devcli_handle_response_t) (char *, size_t);

int devcli_init_sockaddr (struct sockaddr_in *name, const char *hostname,
			  uint16_t port);

devcli_handle_response_t devcli_set_response_handler (devcli_handle_response_t
						      new_handler);
int devcli_write_read (int sock, char *message, int *ret_code);
int devcli_read_data (char *hostname, uint16_t port, void **result,
		      size_t * size);
