/** 
* @file Functions to work with hms format.
*
* hms string is defined as follow:
* <ul> 
* 	<li>hms ::= decimal | decimal + ':' + hms 
* 	<li>decimal ::= unsigneddec | sign + unsigneddec
* 	<li>sign ::= '+' | '-'
* 	<li>unsigneddec ::= integer | integer + '.' + integer
* 	<li>integer ::= [0-9] | [0-9] + integer
* </ul>
*
* Arbitary number of : could be included, but 2 are reasonable
* maximum.
*
* @author skub
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

/** Convert hms (hour:minutes:seconds) string to its double 
* representation.
* 
* @param hptr Pointer to string to convert
* @return Float value of hms, if fails set errno to E
* @exception ERANGE  When some value is out of range for float number.
* @exception EINVAL  When format doesn't match.
*/
double
hmstod (const char *hptr)
{
  char *endptr;
  double ret;			//to store return value
  double mul;			//multiplier

  if (*hptr == '-')
    {
      hptr++;
      mul = -1;
    }
  else
    {
      mul = 1;
    }

  endptr = hptr;
  ret = 0;

  while (*endptr)
    {
      // convert test
      ret += strtod (hptr, &endptr) * mul;
      if ((errno == ERANGE) || (!*endptr))
	{
	  return ret;
	}
      // if we have error in translating first string..
      if ((hptr == endptr) || (*endptr != ':'))
	{
	  errno = EINVAL;
	  return 0;
	}

      mul /= 60;
      hptr = endptr + 1;
    }

  return ret;
}

/* Opposite to hmstod.
*
* @see hmstod
* @param value Value to convert
* @param hptr 
*/

int
dtohms (double value, char *hptr)
{
  char sign;
  double m;
  if (value < 0)
    {
      value = -value;
      sign = '-';
    }
  else
    {
      sign = '+';
    }
  m = (value - floor (value)) * 60;

  return sprintf (hptr, "%c%02.0f:%02.0f:%05.02f", sign, floor (value),
		  floor (m), (m - floor (m)) * 60);
}
