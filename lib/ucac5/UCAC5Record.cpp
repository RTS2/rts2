/* 
 * UCAC5 Record class
 * Copyright (C) 2018 Petr Kubanek <petr@kubanek.net>
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


#include "UCAC5Record.hpp"
#include "libnova_cpp.h"

#include <libnova/libnova.h>
#include <string.h>
#include <sstream>
#include <iomanip>

UCAC5Record::UCAC5Record (struct ucac5 *_data)
{
	memcpy(&data, _data, sizeof (data));
}

void UCAC5Record::getXYZ (double c[3])
{
	eraS2c(getRARad(), getDecRad(), c);
}

std::string UCAC5Record::getString ()
{
	std::ostringstream os;
	os << std::setw(8) << std::setfill('0') << data.srcid << " " << LibnovaRa(getRADeg()) << " " << LibnovaDec(getDecDeg());
	return os.str();
}
