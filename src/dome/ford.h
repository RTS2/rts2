/* 
 * Driver for Ford boards.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2004-2008 Martin Nekola
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

#ifndef __RTS2_DOME_FORD__
#define __RTS2_DOME_FORD__

#include "dome.h"

#include "../utils/rts2connserial.h"

#define PORT_A 0
#define PORT_B 1
#define PORT_C 2

namespace rts2dome
{

/**
 * This is an abstract class which provides support 
 * functions for dome control board, designed and manufactured
 * by Martin Nekola (ford@asu.cas.cz).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 * @author Martin Nekola
 */
class Ford: public Dome
{
	private:
		Rts2ConnSerial *domeConn;
		const char *dome_file;
		unsigned char stav_portu[3];

		int zapni_pin (unsigned char c_port, unsigned char pin);
		int vypni_pin (unsigned char c_port, unsigned char pin);

	protected:
		int zjisti_stav_portu ();

		int ZAP (int i);
		int VYP (int i);

		/**
		 * Switch off two pins.
		 *
		 * @param _pin1      First pin address (specified as pointer to adress array)
		 * @param _pin2	     Second pin address 
		 */
		int switchOffPins (int pin1, int pin2);
		int switchOffPins (int pin1, int pin2, int pin3);
		int switchOffPins (int pin1, int pin2, int pin3, int pin4);

		bool getPortState (int c_port);
		int isOn (int c_port);
		virtual int processOption (int in_opt);
		virtual int init ();

		void flushPort ()
		{
			domeConn->flushPortIO ();
		}
	public:
		Ford (int argc, char **argv);
		virtual ~Ford ();
};

}

#endif	 // !__RTS2_DOME_FORD__
