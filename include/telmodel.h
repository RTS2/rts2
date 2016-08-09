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
 * Conditions for model calculation.
 *
 * Holds only conditions which are static, e.g. it will not hold alt&az, as
 * those will be changed in course of model calculation. Holds mount
 * geographics position, current time etc.
 *
 * @ingroup RTS2TPoint
 */
class ObsConditions
{
	private:
		rts2teld::Telescope * telescope;
	public:
		ObsConditions (rts2teld::Telescope * in_telescope)
		{
			telescope = in_telescope;
		}
		double getLatitude ()
		{
			return telescope->getLatitude ();
		}
		double getLatitudeRadians ()
		{
			return ln_deg_to_rad (getLatitude ());
		}
};

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
		TelModel (rts2teld::Telescope * in_telescope, const char *in_modelFile)
		{
			cond = new ObsConditions (in_telescope);
			modelFile = in_modelFile;
		}

		virtual ~ TelModel (void)
		{
			delete cond;
		}

		virtual int load () = 0;
		/**
		 * Apply model to coordinates. Pos.ra is hour angle, not RA.
		 */
		virtual int apply (struct ln_equ_posn *pos) = 0;
		virtual int applyVerbose (struct ln_equ_posn *pos) = 0;

		virtual int reverse (struct ln_equ_posn *pos) = 0;
		virtual int reverseVerbose (struct ln_equ_posn *pos) = 0;
		virtual int reverse (struct ln_equ_posn *pos, double sid) = 0;

		virtual int applyAltAz (struct ln_hrz_posn *hrz) { return 0; }
		virtual int reverseAltAz (struct ln_hrz_posn *hrz) { return 0; }

                virtual double getRMS () { return -1; }

		virtual std::istream & load (std::istream & is) = 0;
		virtual std::ostream & print (std::ostream & os) = 0;

	protected:
		ObsConditions * cond;
		const char *modelFile;
};

};
#endif							 /* !__RTS2_TELMODEL__ */
