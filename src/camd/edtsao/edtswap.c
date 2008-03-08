#include "edtswap.h"

void
swap2 (to, from, nbytes)
     char *to;
     char *from;
     int nbytes;
{
  char c;
  int i;
  for (i = 0; i < nbytes; i += 2, (to += 2), (from += 2))
    {
      c = *from;
      *(to) = *(from + 1);
      *(to + 1) = c;
    }
}

void
swap4 (to, from, nbytes)
     char *to;
     char *from;
     int nbytes;
{
  char c;
  int i;
  for (i = 0; i < nbytes; i += 4, (to += 4), (from += 4))
    {
      c = *from;
      *to = *(from + 3);
      *(to + 3) = c;
      c = *(from + 1);
      *(to + 1) = *(from + 2);
      *(to + 2) = c;
    }
}

void
swap8 (to, from, nbytes)
     char *to;
     char *from;
     int nbytes;
{
  char c;
  int i;
  for (i = 0; i < nbytes; i += 8, (to += 8), (from += 8))
    {
      c = *(from + 0);
      *(to + 0) = *(from + 7);
      *(to + 7) = c;
      c = *(from + 1);
      *(to + 1) = *(from + 6);
      *(to + 6) = c;
      c = *(from + 2);
      *(to + 2) = *(from + 5);
      *(to + 5) = c;
      c = *(from + 3);
      *(to + 3) = *(from + 4);
      *(to + 4) = c;
    }
}
