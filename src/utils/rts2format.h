/* 
 * Rts2 formatting functions.
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

#ifndef __RTS2_OSTREAM_FORMAT__
#define __RTS2_OSTREAM_FORMAT__

#include <ostream>

/**
 * Stream output will separated hours and minutes with space.
 */
std::ostream & spaceDegSep (std::ostream & _os);


/**
 * Returns state of space formating flag.
 *
 * @return True if degrees, minutes and seconds should be separated by space.
 */
bool formatSpaceDegSep (std::ostream & _os);

/**
 * Stream output will always contains pure numbers. Degreess
 * pretty print will be disabled.
 */
std::ostream & pureNumbers (std::ostream & _os);

/**
 * Returns state of pure number formating flag.
 *
 * @return True if numbers shall be always printed as numbers, e.g. not pretty printed as degrees, minutes etc.
 */
bool formatPureNumbers (std::ostream & _os);

std::ostream & localTime (std::ostream & _os);

/**
 * If print times in local time.
 */
bool formatLocalTime (std::ostream & _os);

#endif /* !__RTS2_OSTREAM_FORMAT__ */
