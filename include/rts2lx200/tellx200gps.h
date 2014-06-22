/* 
 * Driver for LX200 GPS telescopes.
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_TELLX200GPS__
#define __RTS2_TELLX200GPS__

#include "rts2lx200/tellx200.h"

namespace rts2teld
{

class TelLX200GPS:public TelLX200
{
	public:
		TelLX200GPS (int argc, char **argv);
		virtual ~TelLX200GPS (void);

		virtual int initHardware ();
		virtual int initValues ();
		virtual int info ();
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int setTo (double set_ra, double set_dec);
		virtual int correct (double cor_ra, double cor_dec, double real_ra, double real_dec);

		virtual int startResync ();
		virtual int isMoving ();
		virtual int stopMove ();

		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();

		virtual int startDir (char *dir);
		virtual int stopDir (char *dir);

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);


	private:
		int motors;

		time_t move_timeout;

		int tel_set_rate (char new_rate);
		int tel_slew_to (double ra, double dec);

		int tel_check_coords (double ra, double dec);

		void set_move_timeout (time_t plus_time);

        int sleepTelescope();
        int wakeTelescope();

        int setTrackingSpeed(double trac_speed);
        rts2core::ValueDouble *trackingSpeed;


};

};

#endif // __RTS2_TELLX200GPS__
