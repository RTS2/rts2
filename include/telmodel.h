/* 
 * Telescope model reader.
 * Copyright (C) 2006-2007 Petr Kubanek <petr@kubanek.net>
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

#include "modelterm.h"

#include <libnova/libnova.h>

#include <vector>
#include <iostream>


namespace rts2telmodel
{

class ModelTerm;

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
		int getFlip ()
		{
			return telescope->getFlip ();
		}
		double getLatitude ()
		{
			return telescope->getLatitude ();
		}
};

/**
 * Holds telescope model.
 * Performs on terms apply and reverse calls.
 *
 * When we pass ln_equ_posn, ra is hour angle (in degrees), not RA value.
 *
 * Supported terms can be seen from #RTS2TpointTerm listing.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup RTS2TPoint
 */
class Model:public std::vector < ModelTerm * >
{
	public:
		Model (rts2teld::Telescope * in_telescope, const char *in_modelFile);
		virtual ~ Model (void);
		int load ();
		/**
		 * Apply model to coordinates. Pos.ra is hour angle, not RA.
		 */
		int apply (struct ln_equ_posn *pos);
		int applyVerbose (struct ln_equ_posn *pos);

		int reverse (struct ln_equ_posn *pos);
		int reverseVerbose (struct ln_equ_posn *pos);
		int reverse (struct ln_equ_posn *pos, double sid);

		/**
		 * Return RMS in degrees.
		 *
		 * @return RMS error in degrees.
		 */
		double getRMS ()
		{
			return rms / 3600;
		}

		std::istream & load (std::istream & is);
		std::ostream & print (std::ostream & os);

	private:
		ObsConditions * cond;
		const char *modelFile;

		char caption[81];		 // Model description: 80 chars + NULL
		char method;			 // method: T or S
		int num;			 // Number of active observations
		double rms;			 // sky RMS (arcseconds)
		double refA;			 // refraction constant A (arcseconds)
		double refB;			 // refraction constant B (arcseconds)
};


std::istream & operator >> (std::istream & is, Model * model);
std::ostream & operator << (std::ostream & os, Model * model);

};
#endif							 /* !__RTS2_TELMODEL__ */
