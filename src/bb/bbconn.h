/*
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_BBCONN__
#define __RTS2_BBCONN__

#include "rts2script/connexe.h"

namespace rts2bb
{

class ConnBBQueue:public rts2script::ConnExe
{
	public:
		ConnBBQueue (rts2core::Block * _master, const char *_exec, ObservatorySchedule *obs_sched);
		virtual ~ConnBBQueue ()
		{
			delete obs_sched;
		}

		virtual void processCommand (char *cmd);
	
	private:
		ObservatorySchedule *obs_sched;
};

ConnBBQueue *scheduleTarget (int tar_id, int observatory_id, ObservatorySchedule *obs_sched = NULL);

}

#endif // __RTS2_BBCONN__
