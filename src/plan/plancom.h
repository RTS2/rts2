/*!
 * @file Header file for plancomm.
 * $Id$
 *
 * @author petr
 */

#ifndef __RTS_PLANCOM__
#define __RTS_PLANCOM__

#include <pthread.h>

int plancom_add_namespace (char *namespace, const char *hostname, int port);
int plancom_process_command (char *ns_command);

#endif /* !__RTS_PLANCOM__ */
