/**
 * RTS2 Big Brother structure for observatory record.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_BB_OBSERVATORY__
#define __RTS2_BB_OBSERVATORY__

#include <time.h>
#include <map>
#include "xmlrpc++/XmlRpc.h"

using namespace XmlRpc;

namespace rts2bb
{

/**
 * Caches values from single observatory connection.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Observatory
{
	public:
		Observatory () { _lastUpdate = 0; }

		Observatory (XmlRpcValue &value) { _values = value; time (&_lastUpdate); }

		void update (XmlRpcValue &value);

		XmlRpcValue *getValues () { return &_values; }

		time_t getUpdateTime () { return _lastUpdate; }
	private:
		time_t _lastUpdate;
		XmlRpcValue _values;
};

/**
 * Holds observatories cached values. Key is observatory name, value is Observatory object.
 *
 * @see Observatory
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ObservatoryMap:public std::map <std::string, Observatory>
{
	public:
		ObservatoryMap () {}

		void update (XmlRpcValue &value);
};

}

#endif // __RTS2_BB_OBSERVATORY__
