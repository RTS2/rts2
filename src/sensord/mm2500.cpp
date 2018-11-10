/* 
 * Driver for Newport MM2500 multi axis controller.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#include "sensorgpib.h"

#include <iomanip>

namespace rts2sensord
{

class MM2500:public Gpib
{
	public:
		MM2500 (int argc, char **argv);
		virtual ~MM2500 (void);

		virtual int info ();

	protected:
		virtual int init ();
		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

	private:
		rts2core::ValueInteger *ax1;
		rts2core::ValueInteger *ax2;

		rts2core::ValueInteger *dp1;
		rts2core::ValueInteger *dp2;

		rts2core::ValueInteger *moveCount;

		int moveAxis (int axis, int newVal);

		/**
		 * @throw rts2core::Error on error.
		 */
		int getAxis (int axis, const char *cmd);
};

}

using namespace rts2sensord;

MM2500::MM2500 (int argc, char **argv):Gpib (argc, argv)
{
	createValue (ax1, "AX1", "first axis", true, RTS2_VALUE_WRITABLE);
	createValue (dp1, "dp_ax1", "desired position of the first axis", true);
	
	createValue (ax2, "AX2", "second axis", true, RTS2_VALUE_WRITABLE);
	createValue (dp2, "dp_ax2", "desired position of the second axis", true);

	createValue (moveCount, "moveCount", "number of axis movements", false);
	moveCount->setValueInteger (0);
}

MM2500::~MM2500 (void)
{
}

int MM2500::info ()
{
	try
	{
		ax1->setValueInteger (getAxis (1, "TP"));
		dp1->setValueInteger (getAxis (1, "DP"));
		ax2->setValueInteger (getAxis (2, "TP"));
		dp2->setValueInteger (getAxis (2, "DP"));
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_ERROR) << er << sendLog;
		return -1;
	}
	return Gpib::info ();
}

int MM2500::init ()
{
	int ret;
	ret = Gpib::init ();
	if (ret)
		return ret;

	return ret;
}

int MM2500::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == ax1)
	{
		return moveAxis (1, new_value->getValueDouble ());
	}
	if (old_value == ax2)
	{
		return moveAxis (2, new_value->getValueDouble ());
	}
	return Gpib::setValue (old_value, new_value);
}

int MM2500::moveAxis (int axis, int newVal)
{
	try
	{
		std::ostringstream _os;
		_os.fill ('0');
		_os << std::setw (2) << axis << "PA" << newVal << '\r';
		gpibWriteBuffer (_os.str ().c_str (), _os.str ().length ());
		moveCount->inc ();
		sendValueAll (moveCount);
	}
	catch (rts2core::Error &er)
	{
		logStream (MESSAGE_DEBUG) << er << sendLog;
		return -2;
	}
	return 0;
}

int MM2500::getAxis (int axis, const char *cmd)
{
	std::ostringstream _os;
	_os.fill ('0');
	_os << std::setw (2) << axis << cmd << "\r";
	char buf[41];
	gpibWriteRead (_os.str ().c_str (), buf, 40);
	return (atoi (buf + 4));
}

int main (int argc, char **argv)
{
	MM2500 device = MM2500 (argc, argv);
	return device.run ();
}
