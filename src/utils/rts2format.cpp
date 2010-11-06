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

#include "rts2format.h"

int flagSpace = -1;

std::ostream & spaceDegSep (std::ostream & _os)
{
	if (flagSpace == -1)
		flagSpace = _os.xalloc ();
	_os.iword (flagSpace) = 1;
	return _os;
}

bool formatSpaceDegSep (std::ostream & _os)
{
	return flagSpace != -1 && _os.iword (flagSpace) == 1;
}

int flagPureNumbers = -1;

std::ostream & pureNumbers (std::ostream & _os)
{
	if (flagPureNumbers == -1)
		flagPureNumbers = _os.xalloc ();
	_os.iword (flagPureNumbers) = 1;
	return _os;
}

bool formatPureNumbers (std::ostream & _os)
{
	return flagPureNumbers != -1 && _os.iword (flagPureNumbers) == 1;
}

int flagLocalTime = -1;

std::ostream & localTime (std::ostream & _os)
{
	if (flagLocalTime == -1)
		flagLocalTime = _os.xalloc ();
	_os.iword (flagLocalTime) = 1;
	tzset ();
	return _os;
}

bool formatLocalTime (std::ostream & _os)
{
	return flagLocalTime != -1 && _os.iword (flagLocalTime) == 1;
}
