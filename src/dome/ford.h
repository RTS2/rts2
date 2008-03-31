/* 
 * Driver for Ford boards.
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

#ifndef __RTS2_DOME_FORD__
#define __RTS2_DOME_FORD__

#include "dome.h"

#include "../utils/rts2connserial.h"

#define PORT_A 0
#define PORT_B 1
#define PORT_C 2

class Rts2DomeFord: public Rts2DevDome
{
	private:
		Rts2ConnSerial *domeConn;
		char *dome_file;
		unsigned char stav_portu[3];
	protected:
		int zjisti_stav_portu ();
		void zapni_pin (unsigned char c_port, unsigned char pin);
		void vypni_pin (unsigned char c_port, unsigned char pin);

		void ZAP (int i);
		void VYP (int i);
		bool getPortState (int c_port);
		int isOn (int c_port);
		virtual int processOption (int in_opt);
		virtual int init ();
	public:
		Rts2DomeFord (int in_argc, char **in_argv);
		virtual ~Rts2DomeFord (void);
};
#endif							 // !__RTS2_DOME_FORD__
