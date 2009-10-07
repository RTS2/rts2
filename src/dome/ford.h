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

#include "../utils/connford.h"

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
	public:
		Ford (int argc, char **argv);
		virtual ~Ford ();

	protected:
	 	/**
		 * Refresh status information on all ports.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int zjisti_stav_portu () { return domeConn->zjisti_stav_portu (); }

		/**
		 * Switch on pin.
		 *
		 * @param i Port addresss.
		 * @return 0 on success, -1 on error.
		 */
		int ZAP (int i) { return domeConn->ZAP (i); }

		/**
		 * Switch off pin.
		 * 
		 * @param i Port address.
		 * @return 0 on success, -1 on error.
		 */
		int VYP (int i) { return domeConn->VYP (i); }

		/**
		 * Switch off two pins.
		 *
		 * @param pin1      First pin address (specified as pointer to adress array)
		 * @param pin2	    Second pin address 
		 */
		int switchOffPins (int pin1, int pin2) { return domeConn->switchOffPins (pin1, pin2); }

		/**
		 * Switch off two three pins.
		 *
		 * @param pin1      First pin address (specified as pointer to adress array).
		 * @param pin2	    Second pin address. 
		 * @param pin3	    Third pin address.
		 */
		int switchOffPins (int pin1, int pin2, int pin3) { return domeConn->switchOffPins (pin1, pin2, pin3); }

		/**
		 * Switch off two three pins.
		 *
		 * @param pin1      First pin address (specified as pointer to adress array).
		 * @param pin2	    Second pin address. 
		 * @param pin3	    Third pin address.
		 * @param png4      Forth pin address.
		 */
		int switchOffPins (int pin1, int pin2, int pin3, int pin4) { return domeConn->switchOffPins (pin1, pin2, pin3, pin4); }

		bool getPortState (int c_port) { return domeConn->getPortState (c_port); }

		int getPortA () { return domeConn->getPortA (); };
		int getPortB () { return domeConn->getPortB (); };
		int getPortC () { return domeConn->getPortC (); };

		/**
		 * Check if given pin is on.
		 *
		 * @param c_port  Pin address.
		 * @return -1 on error, 0 if pin is off, 1 if it's on.
		 */
		int isOn (int c_port) { return domeConn->isOn (c_port); }

		virtual int processOption (int in_opt);
		virtual int init ();

		void flushPort ()
		{
			domeConn->flushPortIO ();
		}

	private:
		rts2core::FordConn *domeConn;
		const char *dome_file;
};

}

#endif	 // !__RTS2_DOME_FORD__
