/* 
 * Dummy filter driver.
 * Copyright (C) 2003-2014 Petr Kubanek <petr@kubanek.net>
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

#include "filterd.h"

namespace rts2filterd
{

class Dummy:public Filterd
{
	public:
		Dummy (int argc, char **argv):Filterd (argc, argv)
		{
			filterNum = 0;
			filterSleep = 3;
			addOption ('s', "filter_sleep", 1, "how long wait for filter change");

			createValue (filterNames, "filter_names", "filter names (will be parsed)", false, RTS2_VALUE_WRITABLE);
		}

		virtual int processOption (int in_opt)
		{
			switch (in_opt)
			{
				case 's':
					filterSleep = atoi (optarg);
					break;
				default:
					return Filterd::processOption (in_opt);
			}
			return 0;
		}

	protected:
		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value)
		{
			if (old_value == filterNames)
			{
				if (setFilters ((char *) (new_value->getValue ())))
					return -2;
				updateMetaInformations (old_value);
				return 0;
			}
			return Filterd::setValue (old_value, new_value);
		}

		virtual int getFilterNum (void)
		{
			return filterNum;
		}

		virtual int setFilterNum (int new_filter)
		{
			filterNum = new_filter;

			sleep (filterSleep);
			return Filterd::setFilterNum (new_filter);
		}

	private:
		int filterNum;
		int filterSleep;

		rts2core::ValueString *filterNames;
};

};

using namespace rts2filterd;

int main (int argc, char **argv)
{
	Dummy device = Dummy (argc, argv);
	return device.run ();
}
