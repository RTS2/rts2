/**
 * Executor client for photometer.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DEVCLIPHOT__
#define __RTS2_DEVCLIPHOT__

#include "devscript.h"

using namespace rts2script;

class Rts2DevClientPhotExec:public Rts2DevClientPhot, public DevScript
{
	private:
		// minFlux to be considered as success
		float minFlux;

	protected:
		virtual void unblockWait ()
		{
			Rts2DevClientPhot::unblockWait ();
		}
		virtual void unsetWait ()
		{
			Rts2DevClientPhot::unsetWait ();
		}

		virtual void clearWait ()
		{
			Rts2DevClientPhot::clearWait ();
		}

		virtual int isWaitMove ()
		{
			return Rts2DevClientPhot::isWaitMove ();
		}

		virtual void setWaitMove ()
		{
			Rts2DevClientPhot::setWaitMove ();
		}

		virtual void queCommandFromScript (Rts2Command * com)
		{
			queCommand (com);
		}

		virtual int getFailedCount ()
		{
			return Rts2DevClient::getFailedCount ();
		}

		virtual void clearFailedCount ()
		{
			Rts2DevClient::clearFailedCount ();
		}

		virtual void idle ()
		{
			DevScript::idle ();
			Rts2DevClientPhot::idle ();
		}

		virtual void filterMoveEnd ();

		virtual void integrationStart ();
		virtual void integrationEnd ();
		virtual void addCount (int count, float exp, bool is_ov);

		virtual int getNextCommand ();

	public:
		Rts2DevClientPhotExec (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientPhotExec (void);
		virtual void postEvent (Rts2Event * event);
		virtual void integrationFailed (int status);

		virtual void filterMoveFailed (int status);

		virtual void nextCommand ();
};
#endif							 /* !__RTS2_DEVCLIPHOT__ */
