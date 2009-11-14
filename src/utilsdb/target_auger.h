/* 
 * Auger cosmic rays showers follow-up target.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TARGET_AUGER__
#define __RTS2_TARGET_AUGER__

#include "target.h"

namespace rts2targetauger
{
struct vec
{
	double x;
	double y;
	double z;
};	

}

class TargetAuger:public ConstTarget
{
	public:
		TargetAuger () {}
		TargetAuger (int in_tar_id, struct ln_lnlat_posn *_obs, int in_augerPriorityTimeout);
		/**
		 * Crate target if targets fields are know. You should not call load ont target
		 * created with this contructor.
		 */
		TargetAuger (int _auger_t3id, double _auger_date, int _auger_npixels, double _auger_ra, double _auger_dec, double _northing, double _easting, double _altitude, struct ln_lnlat_posn *_obs);
		virtual ~ TargetAuger (void);

		virtual int load ();
		/**
		 * Load target from given target_id.
		 *
		 * @param auger_id  Auger id.
		 */
		int load (int auger_id);
		virtual int getScript (const char *device_name, std::string & buf);
		virtual float getBonus (double JD);
		virtual moveType afterSlewProcessed ();
		virtual int considerForObserving (double JD);
		virtual int changePriority (int pri_change, time_t * time_ch)
		{
			// do not drop priority
			return 0;
		}
		virtual int isContinues ()
		{
			return 1;
		}

		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		virtual void printHTMLRow (std::ostringstream &_os, double JD);

		virtual void writeToImage (Rts2Image * image, double JD);

		/**
		 * Return fiels for observations.
		 */
		void getEquPositions (std::vector <struct ln_equ_posn> &positions);

		double getShowerJD () { time_t a = auger_date; return ln_get_julian_from_timet (&a); }

	private:
		int t3id;
		double auger_date;
		int npixels;
		int augerPriorityTimeout;
		// core coordinates
		struct rts2targetauger::vec cor;

		// valid positions
		std::vector <struct ln_equ_posn> showerOffsets;

		void updateShowerFields ();
		void addShowerOffset (struct ln_equ_posn &pos);
};
#endif							 /* !__RTS2_TARGET_AUGER__ */
