#ifndef __RTS_MKPATH__
#define __RTS_MKPATH__

#include <sys/types.h>

#ifdef _cplusplus
extern "C"
{
#endif

  int mkpath (const char *path, mode_t mode);

#ifdef _cplusplus
}
#endif

#endif				/* !__RTS_MKPATH__ */
