/* 
 * Dummy cupola driver.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include "cupola.h"

using namespace rts2dome;

namespace rts2dome
{

/**
 * Dummy copula for testing.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Dummy:public Cupola
{
	private:
		rts2core::ValueInteger * mcount;
		rts2core::ValueInteger *moveCountTop;
	protected:
		virtual int moveStart ()
		{
			mcount->setValueInteger (0);
			sendValueAll (mcount);
			return Cupola::moveStart ();
		}
		virtual int moveEnd ()
		{
			struct ln_hrz_posn hrz;
			getTargetAltAz (&hrz);
			setCurrentAz (hrz.az);
			return Cupola::moveEnd ();
		}
		virtual long isMoving ()
		{
			if (mcount->getValueInteger () >= moveCountTop->getValueInteger ())
				return -2;
			mcount->inc ();
			sendValueAll (mcount);
			return USEC_SEC;
		}

		virtual int startOpen ()
		{
			if ((getState () & DOME_DOME_MASK) == DOME_OPENING)
				return 0;
			mcount->setValueInteger (0);
			sendValueAll (mcount);
			return 0;
		}

		virtual long isOpened ()
		{
			if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
				return 0;
			return isMoving ();
		}

		virtual int endOpen ()
		{
			return 0;
		}

		virtual int startClose ()
		{
			if ((getState () & DOME_DOME_MASK) == DOME_CLOSING)
				return 0;
			mcount->setValueInteger (0);
			sendValueAll (mcount);
			return 0;
		}

		virtual long isClosed ()
		{
			if ((getState () & DOME_DOME_MASK) == DOME_OPENED)
				return 0;
		 	if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
				return -2;
			return isMoving ();
		}

		virtual int endClose ()
		{
			return 0;
		}

	public:
		Dummy (int argc, char **argv):Cupola (argc, argv)
		{
			createValue (mcount, "mcount", "moving count", false);
			createValue (moveCountTop, "moveCountTop", "move count top", false, RTS2_VALUE_WRITABLE);
			moveCountTop->setValueInteger (100);
		}

		virtual int initValues ()
		{
			setCurrentAz (0);
			return Cupola::initValues ();
		}

		virtual double getSlitWidth (__attribute__ ((unused)) double alt)
		{
			return 1;
		}
};

}

int main (int argc, char **argv)
{
	Dummy device (argc, argv);
	return device.run ();
}
