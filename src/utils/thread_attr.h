#ifndef __RTS2_THREAD_ATTRS__
#define __RTS2_THREAD_ATTRS__

#include <pthread.h>

#ifdef _cplusplus
extern "C"
{
#endif

  void thread_attrs_set (pthread_attr_t * attrs);

#ifdef _cplusplus
}
#endif

#endif				/* __RTS2_PTHREAD_ATTRS__ */
