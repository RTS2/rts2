/*! Hour, minutes, seconds manipulation
* $Id$
* @author petr
*/
#ifndef __RTS_HMS__
#define __RTS_HMS__

#ifdef __cplusplus
extern "C"
{
#endif

  extern double hmstod (const char *hptr);
  extern int dtohms (double value, char *hptr);
  extern int dtoints (double value, int *h, int *m, int *s);

#ifdef __cplusplus
};
#endif

#endif // __RTS_HMS__
