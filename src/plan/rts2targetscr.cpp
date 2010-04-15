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

#include "rts2targetscr.h"

Rts2TargetScr::Rts2TargetScr (Rts2ScriptInterface * in_master):Rts2Target ()
{
	master = in_master;
	target_id = 1;
	moveEnded ();
}

Rts2TargetScr::~Rts2TargetScr (void)
{
}

bool Rts2TargetScr::getScript (const char *device_name, std::string & buf)
{
	int ret = master->findScript (std::string (device_name), buf);
	if (ret >= 0)
		return ret > 0;
	
	throw rts2core::Error (std::string ("script for device ") + device_name + " is missing.");
}

void Rts2TargetScr::getPosition (struct ln_equ_posn *pos, double JD)
{
	master->getPosition (pos, JD);
}

int Rts2TargetScr::setNextObservable (time_t * time_ch)
{
	return 0;
}

void Rts2TargetScr::setTargetBonus (float new_bonus, time_t * new_time)
{

}

int Rts2TargetScr::save (bool overwrite)
{
	return 0;
}

int Rts2TargetScr::save (bool overwrite, int tar_id)
{
	return 0;
}

moveType Rts2TargetScr::startSlew (struct ln_equ_posn * position)
{
	position->ra = position->dec = 0;
	return OBS_MOVE_FAILED;
}

int Rts2TargetScr::startObservation (Rts2Block * in_master)
{
	return -1;
}

void Rts2TargetScr::writeToImage (Rts2Image * image, double JD)
{
}
