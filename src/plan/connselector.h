/*
 * Connections handling selection of targets.
 * Copyright (C) 2010 Petr Kubanek, Institute of Physics
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

#ifndef __RTS2CONNSELECTOR__
#define __RTS2CONNSELECTOR__

#include "connimgprocess.h"

#define EVENT_SELECT_NEXT       RTS2_LOCAL_EVENT + 5077

namespace rts2plan
{

/**
 * Class for scheduling targets.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnSelector:public ConnProcess
{
	public:
		ConnSelector (rts2core::Block * in_master, const char *in_exe, int in_timeout);

		virtual void postEvent (Rts2Event *event);

	protected:
		virtual void processCommand (char *cmd);

	private:
		bool waitNext;
};

}
#endif							 /* !__RTS2CONNSELECTOR__ */
