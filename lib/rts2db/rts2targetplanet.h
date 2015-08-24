/* 
 * Class for planetary targets.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGETPLANET__
#define __RTS2_TARGETPLANET__

#include "rts2db/target.h"

namespace rts2db
{

typedef void (*get_equ_t) (double, struct ln_equ_posn *);
typedef double (*get_double_val_t) (double);

struct planet_info_t
{
	const char *name;
	get_equ_t rst_func;
	get_double_val_t earth_func;
	get_double_val_t sun_func;
	get_double_val_t mag_func;
	get_double_val_t sdiam_func;
	get_double_val_t phase_func;
	get_double_val_t disk_func;
};

class TargetPlanet:public Target
{
	private:
		planet_info_t * planet_info;
		void getPosition (struct ln_equ_posn *pos, double JD, struct ln_equ_posn *parallax);
	public:
		TargetPlanet (int tar_id, struct ln_lnlat_posn *in_obs, double in_altitude);
		virtual ~ TargetPlanet (void);

		virtual void load ();
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int getRST (struct ln_rst_time *rst, double JD, double horizon);

		virtual int isContinues ();
		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		double getEarthDistance (double JD);
		double getSolarDistance (double JD);
		double getMagnitude (double JD);
		double getSDiam (double JD);
		double getPhase (double JD);
		double getDisk (double JD);
};

}
#endif							 /*! __RTS2_TARGETPLANET__ */
