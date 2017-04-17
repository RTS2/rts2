/* 
 * Configuration file read routines.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "displayvalue.h"
#include "timestamp.h"
#include "configuration.h"

#include <sstream>
#include <iomanip>

std::string rts2core::getDisplayValue (rts2core::Value * value)
{
	std::ostringstream _os;
	const char *tmp_val;
	const char sunits[] = {'B', 'K', 'M', 'G', 'T', 'P'};
	const char munits[] = {' ', 'k', 'M', 'G', 'T', 'P'};
	int sind = 0;
	double sval;
	// always display composite values with their own routine
	if (value->getValueExtType () == RTS2_VALUE_STAT)
	{
			tmp_val = value->getDisplayValue ();
			if (tmp_val)
				return std::string (tmp_val);
			return std::string ("");
	}
		
	switch (value->getValueDisplayType ())
	{
		case RTS2_DT_RA:
			_os << LibnovaRa (value->getValueDouble ());
			break;
		case RTS2_DT_DEC:
			_os << LibnovaDeg90 (value->getValueDouble ());
			break;
		case RTS2_DT_DEGREES:
			_os << LibnovaDeg (value->getValueDouble ());
			break;
		case RTS2_DT_DEG_DIST:
			_os << LibnovaDegDist (value->getValueDouble ());
			break;
		case RTS2_DT_DEG_DIST_180:
			_os << LibnovaDeg180 (value->getValueDouble ());
			break;
		case RTS2_DT_PERCENTS:
			_os << std::setw (6) << value->getValueDouble () << "%";
			break;
		case RTS2_DT_ARCSEC:
			_os << std::setprecision (3) << value->getValueDouble () * 3600.0 << "\"";
			break;
		case RTS2_DT_HEX:
			_os << "0x" << std::setw (8) << std::hex << value->getValueInteger ();
			break;
		case RTS2_DT_BYTESIZE:
			sval = value->getValueDouble ();
			while (sval > 1.5 * 1024)
			{
				sval /= 1024;
				sind++;
			}
			_os << std::setiosflags (std::ios_base::fixed) << std::setprecision (2) << sval << sunits[sind];
			break;
		case RTS2_DT_KMG:
			sval = value->getValueDouble ();
			while (sval > 1.5 * 1000)
			{
				sval /= 1000;
				sind++;
			}
			_os << std::setiosflags (std::ios_base::fixed) << std::setprecision (2) << sval << munits[sind];
			break;
		case RTS2_DT_TIMEINTERVAL:
			_os << TimeDiff (value->getValueDouble (), rts2core::Configuration::instance ()->getShowMilliseconds ());
			break;
		case RTS2_DT_ONOFF:
			if (!(value->getFlags () & RTS2_VALUE_ARRAY))
			{
				_os << (value->getValueInteger () == 1 ? "on" : "off");
				break;
			}
		default:
			tmp_val = value->getDisplayValue ();
			if (tmp_val)
				return std::string (tmp_val);
			return std::string ("");
	}
	return _os.str ();
}
