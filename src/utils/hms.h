/*! Hour, minutes, seconds manipulation
* $Id$
* @author petr
*/
#ifndef __RTS_HMS__
#define __RTS_HMS__

double hmstod (const char *hptr);
int dtohms (double value, char *hptr);
int dtoints (double value, int *h, int *m, int *s);

#endif // __RTS_HMS__
