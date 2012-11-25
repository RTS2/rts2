/* 
 * Two functions to work with HMS format.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/**
 * @file HMS parser.
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
 * @author Petr Kubanek <petr@kubanek.net>
 */

#ifndef __RTS_HMS__
#define __RTS_HMS__

#ifdef __cplusplus
extern "C"
{
	#endif

	/**
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
	extern double hmstod (const char *hptr);

	/**
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
	extern int dtoints (double value, int *h, int *m, int *s);

	#ifdef __cplusplus
};
#endif
#endif							 // __RTS_HMS__
