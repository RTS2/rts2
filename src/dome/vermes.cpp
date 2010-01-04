/* 
 * 2010-01-04: This is a copy of dummy_cup.cpp
 * Obs. Vermes cupola driver.
 * Copyright (C) 2010 Markus Wildi <markus.wildi@one-arcsec.org>
 * based on  Petr Kubanek <petr@kubanek.net> dummy_cup.cpp
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
 * Obs. Vermes cupola driver.
 *
 * @author Markus Wildi <markus.wildi@one-arcsec.org>
 */
class Vermes:public Cupola
{
	private:
		Rts2ValueInteger * mcount;
		Rts2ValueInteger *moveCountTop;
	protected:
		virtual int moveStart ()
		{
			mcount->setValueInteger (0);
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
			return USEC_SEC;
		}

		virtual int startOpen ()
		{
			if ((getState () & DOME_DOME_MASK) == DOME_OPENING)
				return 0;
			mcount->setValueInteger (0);
			return 0;
		}

		virtual long isOpened ()
		{
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
			return 0;
		}

		virtual long isClosed ()
		{
		 	if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
				return -2;
			return isMoving ();
		}

		virtual int endClose ()
		{
			return 0;
		}

	public:
		Vermes (int argc, char **argv):Cupola (argc, argv)
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

		virtual double getSplitWidth (double alt)
		{
			return 1;
		}
};

}

int main (int argc, char **argv)
{
	Vermes device (argc, argv);
	return device.run ();
}
