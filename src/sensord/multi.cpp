/* 
 * Source demonstrating how to use multiple sensors.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "multidev.h"
#include "sensord.h"

using namespace rts2sensord;

class MultiSensor: public SensorWeather
{
	public:
		MultiSensor (int argc, char **argv, const char *sn);
	
	private:
		rts2core::ValueDouble *testDouble;
		rts2core::ValueString *testString;
};

MultiSensor::MultiSensor (int argc, char **argv, const char *sn):SensorWeather (argc, argv, 120, sn)
{
	createValue (testDouble, "test_double", "test double variable", true, RTS2_VALUE_WRITABLE);
	createValue (testString, "test_string", "test string variable", true);

	char buf[10];
	random_salt (buf, 9);
	buf[9] = '\0';
	testString->setValueCharArr (buf);
}

int main (int argc, char ** argv)
{
	rts2core::MultiDev md;
	md.push_back (new MultiSensor (argc, argv, "MS1"));
	md.push_back (new MultiSensor (argc, argv, "MS2"));

	return md.run ();
}
