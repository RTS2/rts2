/* 
 * Baader planetarium All-sky dome driver.
 * Copyright 2018 Edward Emelianov <edward.emelianoff@gmail.com>.
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
class BaaderAllSky:public Cupola
{
	private:
        //rts2core::ConnSerial *Conn;
        //rts2core::ValueInteger *shuttersStatus;
	protected:
        // ADD isGoodWeather !!!
        virtual int idle (){
            // send IDLE command
            return Cupola::idle();
        }
		virtual int moveStart ()
		{
			return 0;
		}
		virtual int moveEnd ()
		{
			return 0;
		}
		virtual long isMoving ()
		{
			return 60*USEC_SEC;
		}

		virtual int startOpen () // @return 0 if OK, -1 if error
		{
			if ((getState () & DOME_DOME_MASK) == DOME_OPENING)
				return 0;
			// send OPEN command here
			return 0;
		}

        virtual long isOpened () // @return >=0 for timeout; -1 if error, -2 if opened
		{
			if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
				return 0;
			// check state & return
			return -1;
		}

		virtual int endOpen () // @return 0 if opened, -1 if error
		{
			return 0;
		}

		virtual int startClose () // @return 0 if OK, -1 if error
		{
			if ((getState () & DOME_DOME_MASK) == DOME_CLOSING)
				return 0;
			// send CLOSE command
			return 0;
		}

		virtual long isClosed () // @return >=0 for timeout; -1 if error, -2 if closed
		{
			if ((getState () & DOME_DOME_MASK) == DOME_OPENED)
				return 0;
		 	if ((getState () & DOME_DOME_MASK) == DOME_CLOSED)
				return -2;
			// chk & return -2 or timeout
			return -1;
		}

		virtual int endClose () // @return 0 if OK, -1 if error
		{
			return 0;
		}

	public:
		BaaderAllSky (int argc, char **argv):Cupola (argc, argv)
		{
            ;
		}

        virtual ~BaaderAllSky()
        {
            startClose();
        }

		virtual int initValues ()
		{
			setCurrentAz (0);
			return Cupola::initValues ();
		}

		virtual double getSlitWidth (__attribute__ ((unused)) double alt)
		{
			return -1;
		}
};
}

int main (int argc, char **argv)
{
	BaaderAllSky device (argc, argv);
	return device.run ();
}
