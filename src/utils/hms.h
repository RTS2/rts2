/*! Hour, minutes, seconds manipulation
* $Id$
* @author petr
*/
#ifndef __RTS_HMS__
#define __RTS_HMS__

#ifdef _cplusplus
extern "C"
{
#endif

  double hmstod (const char *hptr);
  int dtohms (double value, char *hptr);
  int dtoints (double value, int *h, int *m, int *s);

#ifdef _cplusplus
}
#endif

#endif				// __RTS_HMS__
