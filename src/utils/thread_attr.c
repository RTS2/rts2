#include <pthread.h>
#include <limits.h>

#include "thread_attr.h"

void
thread_attrs_set (pthread_attr_t * attrs)
{
  pthread_attr_init (attrs);
  pthread_attr_setdetachstate (attrs, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize (attrs, PTHREAD_STACK_MIN);
}
