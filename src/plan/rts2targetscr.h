/*
 * Target for script executor.
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

#ifndef __RTS2_TARGESCR__
#define __RTS2_TARGESCR__

#include "rts2scriptinterface.h"
#include "../utils/rts2target.h"
#include "../writers/image.h"

class Rts2ScriptExec;

/**
 * This target is used in Rts2ScriptExec to fill role of current
 * target, so executor logic will work properly.
 */
class Rts2TargetScr:public Rts2Target
{
	private:
		Rts2ScriptInterface * master;
	public:
		Rts2TargetScr (Rts2ScriptInterface * in_master);
		virtual ~ Rts2TargetScr (void);

		// target manipulation functions
		virtual bool getScript (const char *device_name, std::string & buf);

		// return target position at given julian date
		virtual void getPosition (struct ln_equ_posn *pos, double JD);

		virtual int setNextObservable (time_t * time_ch);
		virtual void setTargetBonus (float new_bonus, time_t * new_time = NULL);

		virtual int save (bool overwrite);
		virtual int save (bool overwrite, int tar_id);

		virtual moveType startSlew (struct ln_equ_posn *position, bool update_position = true);
		virtual int startObservation ();
		virtual void writeToImage (rts2image::Image * image, double JD);
};
#endif							 /* !__RTS2_TARGESCR__ */
