/*!
 * @file Functions to work with hms format.
 * $Id$
 * hms string is defined as follow:
 * <ul> 
 * 	<li>hms ::= decimal | decimal + [!0-9] + hms 
 * 	<li>decimal ::= unsigneddec | sign + unsigneddec
 * 	<li>sign ::= '+' | '-'
 * 	<li>unsigneddec ::= integer | integer + '.' + integer
 * 	<li>integer ::= [0-9] | [0-9] + integer
 * </ul>
 *
 * Arbitary number of : could be included, but 2 are reasonable
 * maximum.
 *
 * @author petr 
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>

/*!
 * Convert hms (hour:minutes:seconds) string to its double 
 * representation.
 * 
 * @param hptr		pointer to string to convert
 * 
 * @return float value of hms, if fails set errno to error and returns
 * NAN 
 * 
 * @exception ERANGE	when some value is out of range for float number.
 * @exception EINVAL	when format doesn't match.
 */
double
hmstod (const char *hptr)
{
  char *locptr;
  char *endptr;
  double ret;			//to store return value
  double mul;			//multiplier 

  if (!(locptr = strdup (hptr)))
    return -1;

  if (*locptr == '-')
    {
      locptr++;
      mul = -1;
    }
  else
    {
      mul = 1;
    }

  endptr = locptr;
  ret = 0;

  while (*endptr)
    {
      // convert test
      ret += strtod (locptr, &endptr) * mul;
      if (errno == ERANGE)
	return ret;
      // we get sucessfuly to end
      if (!*endptr)
	{
	  errno = 0;
	  return ret;
	}

      // if we have error in translating first string..
      if (locptr == endptr)
	{
	  errno = EINVAL;
	  return NAN;
	}

      mul /= 60;
      locptr = endptr + 1;
    }

  errno = 0;
  return ret;
}

/*!
 * Gives h, m and s values from double.
 *
 * @see hmstod
 * 
 * @param value		value to convert
 * @param h		set to hours
 * @param m		set to minutes
 * @param s		set to seconds
 * 
 * @return -1 and set errno on failure, 0 otherwise 
 */
int
dtoints (double value, int *h, int *m, int *s)
{
  int sign;
  double m_fraction;
  if (value < 0)
    {
      value = -value;
      sign = -1;
    }
  else
    {
      sign = 1;
    }
  *h = floor (value) * sign;
  m_fraction = (value - floor (value)) * 60;
  *m = floor (m_fraction);
  *s = round ((m_fraction - *m) * 60);
  // unification
  if (*s == 60)
    {
      *s = 0;
      (*m)++;
    }
  if (*m >= 60)
    {
      *m -= 60;
      (*h)++;
    }
  return 0;
}

/*!
 * Opposite to hmstod.
 * 
 * @see hmstod
 * @param value		value to convert
 * @param hptr		should be allocated to minimal 10 characters
 * 
 * @return -1 and set errno on error, number of writen bits otherwise
 */
int
dtohms (double value, char *hptr)
{
  int h, m, s;
  dtoints (value, &h, &m, &s);
  return sprintf (hptr, "%+02i:%02i:%02i", h, m, s);
}
