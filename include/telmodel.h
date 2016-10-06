/* 
 * Telescope T-Point pointing model.
 * Copyright (C) 2006-2015 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TELMODEL__
#define __RTS2_TELMODEL__

/**
 * @file
 * Basic TPoint routines.
 *
 * @defgroup RTS2TPoint TPoint interface
 */

namespace rts2telmodel
{
};

#include "teld.h"
#include "configuration.h"

#include "tpointmodelterm.h"

#include <libnova/libnova.h>

#include <vector>
#include <iostream>


namespace rts2telmodel
{

/**
 * Telescope pointing model abstract class.
 * Child class should implement abstract model, and can be loaded
 * from teld.cpp source file for different pointing model corrections.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TelModel
 */
class TelModel
{
	public:
		TelModel (double in_latitude)
		{
			tel_latitude = in_latitude;
			tel_latitude_r = ln_deg_to_rad (in_latitude);
		}

		virtual ~ TelModel (void)
		{
		}

		virtual int load (const char *modelFile) = 0;
		/**
		 * Apply model to coordinates. Pos.ra is hour angle, not RA.
		 */
		virtual int apply (struct ln_equ_posn *pos) = 0;
		virtual int applyVerbose (struct ln_equ_posn *pos) = 0;

		virtual int reverse (struct ln_equ_posn *pos) = 0;
		virtual int reverseVerbose (struct ln_equ_posn *pos) = 0;
		virtual int reverse (struct ln_equ_posn *pos, double sid) = 0;

		virtual void getErrAltAz (struct ln_hrz_posn *hrz, struct ln_hrz_posn *err) { }

                virtual double getRMS () { return -1; }

		virtual std::istream & load (std::istream & is) = 0;
		virtual std::ostream & print (std::ostream & os, char frmt = 'r') = 0;

		double getLatitudeRadians () { return tel_latitude_r; }

	protected:
		double tel_latitude;
		double tel_latitude_r;
};

};
#endif							 /* !__RTS2_TELMODEL__ */
