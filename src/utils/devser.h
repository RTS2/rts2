/*! 
 * @file Device deamon header file.
 * $Id$
 *
 * @author petr
 */

#ifndef __RTS_DEVDEM__
#define __RTS_DEVDEM__

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define STATUSNAME	8
#define MSG_SIZE	50

typedef int (*devdem_handle_command_t) (char *, size_t);

struct devdem_msg
{
  long mtype;
  char mtext[MSG_SIZE];
};

int devdem_run (int port, devdem_handle_command_t in_handler,
		char **status_names, int status_num, size_t shm_data_size);
int devdem_dprintf (const char *format, ...);
int devdem_send_data (struct in_addr *client_addr, void *data_ptr,
		      size_t data_size);
int devdem_write_command_end (int retc, char *msg_format, ...);

int devdem_thread_create (void *(*start_routine) (void *), void *arg,
			  size_t arg_size, int *id);
int devdem_thread_cancel (int id);
int devdem_thread_cancel_all (void);

int devdem_status_message (int subdevice, char *description);
int devdem_status_mask (int subdevice, int mask, int operand, char *message);

void *devdem_shm_data_at ();
void devdem_shm_data_lock ();
void devdem_shm_data_unlock ();
void devdem_shm_data_dt ();

void devdem_msg_set_handler (devdem_handle_command_t handler);
int devdem_msg_snd (struct devdem_msg *msg);

extern pid_t devdem_parent_pid;
extern pid_t devdem_child_pid;

#endif /* !__RTS_DEVDEM__ */
