/* 
 * Basic catalogue class.
 * Copyright (C) 2016 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CATD__
#define __RTS2_CATD__

#include "device.h"

/**
 * Abstract sensors, SensorWeather with functions to set weather state, and various other sensors.
 */
namespace rts2catd
{

/**
 * Class for a catalogue. Sensor can be any device which produce some information
 * which RTS2 can use.
 *
 * For special devices, which are ussually to be found in an observatory,
 * please see special classes (Dome, Camera, Telescope etc..).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Catd:public rts2core::Device
{
	public:
		Catd (int argc, char **argv, const char *cn = "CAT1");
		virtual ~Catd (void);

	protected:
		virtual int searchCataloge (struct ln_equ_posn *c1, struct ln_equ_posn *c2, int num) = 0;

	private:
		rts2core::ValueRaDec *corner1;
		rts2core::ValueRaDec *corner2;
		rts2core::ValueInteger *numStars;
};

};

#endif							 /* !__RTS2_CATD__ */
