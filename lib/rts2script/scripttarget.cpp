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

#include "rts2script/scripttarget.h"

using namespace rts2script;

ScriptTarget::ScriptTarget (ScriptInterface * in_master):Rts2Target ()
{
	master = in_master;
	target_id = -2;
	moveEnded ();
}

ScriptTarget::~ScriptTarget (void)
{
}

bool ScriptTarget::getScript (const char *device_name, std::string & buf)
{
	int ret = master->findScript (std::string (device_name), buf);
	if (ret >= 0)
		return ret > 0;
	
	throw rts2core::Error (std::string ("script for device ") + device_name + " is missing.");
}

void ScriptTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	master->getPosition (pos, JD);
}

int ScriptTarget::setNextObservable (__attribute__ ((unused)) time_t * time_ch)
{
	return 0;
}

void ScriptTarget::setTargetBonus (__attribute__ ((unused)) float new_bonus, __attribute__ ((unused)) time_t * new_time)
{

}

int ScriptTarget::save (__attribute__ ((unused)) bool overwrite)
{
	return 0;
}

int ScriptTarget::saveWithID (__attribute__ ((unused)) bool overwrite, __attribute__ ((unused)) int tar_id)
{
	return 0;
}

moveType ScriptTarget::startSlew (struct ln_equ_posn * position, __attribute__ ((unused)) std::string &p1, __attribute__ ((unused)) std::string &p2, __attribute__ ((unused)) bool update_position, __attribute__ ((unused)) int plan_id)
{
	position->ra = position->dec = 0;
	return OBS_MOVE_FAILED;
}

int ScriptTarget::startObservation ()
{
	return -1;
}

void ScriptTarget::writeToImage (__attribute__ ((unused)) rts2image::Image * image, __attribute__ ((unused)) double JD)
{
}
