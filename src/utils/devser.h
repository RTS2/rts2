/*! @file Device deamon header file.
* $Id$
* @author petr
*/
#ifndef __RTS_DEVDEM__
#define __RTS_DEVDEM__

typedef int (*devdem_handle_command_t) (char *, int);

int devdem_run (int port, devdem_handle_command_t handler);
int devdem_write_command_end (char *msg, int retc);
int devdem_dprintf (int fd, const char *format, ...);

#endif // __RTS_DEVDEM__
