/*! 
 * @file Device deamon header file.
 * $Id$
 *
 * @author petr
 */

#ifndef __RTS_DEVSER__
#define __RTS_DEVSER__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdarg.h>

#define MSG_SIZE		50

#define MAX_CLIENT		10

// data memory types

typedef int (*devser_handle_command_t) (char *command);
typedef int (*devser_handle_msg_t) (char *message);

typedef void (*devser_thread_cleaner_t) (void *arg);

int devser_init (size_t shm_data_size);
int devser_run (int port, devser_handle_command_t in_handler);
int devser_dprintf (const char *format, ...);
int devser_write_command_end (int retc, const char *msg_format, ...);

int devser_data_init (size_t buffer_size, size_t data_size, int *id);
void devser_data_done (int id);
int devser_data_put (int id, void *data, size_t size);
int devser_data_invalidate (int id);

int devser_thread_create (void *(*start_routine) (void *), void *arg,
			  size_t arg_size, int *id,
			  devser_thread_cleaner_t clean_cancel);
int devser_thread_cancel (int id);
int devser_thread_cancel_all (void);

int devser_thread_wait (void);

int devser_message (const char *format, ...);

void *devser_shm_data_at ();
void devser_shm_data_lock ();
void devser_shm_data_unlock ();
void devser_shm_data_dt (void *mem);

void devser_get_client_ip (struct sockaddr_in *client_ip);
int devser_set_server_id (int server_id_in,
			  devser_handle_msg_t msg_handler_in);

int devser_2devser_message (int recv_id, void *message);
int devser_2devser_message_va (int recv_id, char *format, va_list ap);
int devser_2devser_message_format (int recv_id, char *format, ...);

int devser_param_test_length (int npars);

int devser_param_next_string (char **ret);
int devser_param_next_integer (int *ret);
int devser_param_next_float (float *ret);
int devser_param_next_hmsdec (double *ret);
int devser_param_next_ip_address (char **hostname, unsigned int *port);

extern pid_t devser_parent_pid;
extern pid_t devser_child_pid;

#endif /* !__RTS_DEVSER__ */
