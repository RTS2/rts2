/* 
 * Parser for RA DEC string.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_RADECPARSER__
#define __RTS2_RADECPARSER__

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
double parseDMS (const char *hptr, double *mul);

/**
 * Get Ra and Dec from string.
 *
 * @param radec String with RA DEC. DEC is separated from RA by + or - string, seximal va
 */
int parseRaDec (const char *radec, double &ra, double &dec);
#endif							 /* !__RTS2_RADECPARSER__ */
