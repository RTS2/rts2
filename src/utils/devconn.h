/*! 
 * @file Constants for data connection.
 *
 * @author petr
 */

#include <sys/time.h>

#ifndef __RTS_DEVCONN__
#define __RTS_DEVCONN__

#define MAXMSG  		512
#define MAXDATACONS		10
//! Data port range
#define MINDATAPORT		5556
#define MAXDATAPORT		5656
#define DATA_BLOCK_SIZE		1000

#define STATUSNAME		9

/*! 
 * Status change handler.
 *
 */
typedef int (*status_notifier_t) (int new_status, int old_status);

//! holds status informations
struct devconn_status
{
  char name[STATUSNAME + 1];
  int status;
  status_notifier_t notifier;	//! hook to do something, when status is changed
  time_t last_update;
};

#endif // __RTS_DEVCONN__
