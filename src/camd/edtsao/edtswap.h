#ifdef __STDC__
#include <stdarg.h>
#define PROTOTYPE(X)     X
#else
#include <varargs.h>
#define PROTOTYPE(X)    ( )
#endif

#define ft_byteswap()                   (*(short *)"\001\000" & 0x0001)
void swap2 PROTOTYPE ((char *to, char *from, int nbytes));
void swap4 PROTOTYPE ((char *to, char *from, int nbytes));
void swap8 PROTOTYPE ((char *to, char *from, int nbytes));
