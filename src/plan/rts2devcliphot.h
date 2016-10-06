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

#include "rts2script/devscript.h"

namespace rts2script
{

class DevClientPhotExec:public rts2core::DevClientPhot, public DevScript
{
	public:
		DevClientPhotExec (rts2core::Connection * in_connection);
		virtual ~ DevClientPhotExec (void);
		virtual void postEvent (rts2core::Event * event);
		virtual void integrationFailed (int status);

		virtual void filterMoveFailed (int status);

		virtual void nextCommand (rts2core::Command *triggerCommand = NULL);


	protected:
		virtual void unblockWait ()
		{
			rts2core::DevClientPhot::unblockWait ();
		}
		virtual void unsetWait ()
		{
			rts2core::DevClientPhot::unsetWait ();
		}

		virtual void clearWait ()
		{
			rts2core::DevClientPhot::clearWait ();
		}

		virtual int isWaitMove ()
		{
			return rts2core::DevClientPhot::isWaitMove ();
		}

		virtual void setWaitMove ()
		{
			rts2core::DevClientPhot::setWaitMove ();
		}

		virtual int queCommandFromScript (rts2core::Command * com)
		{
			return queCommand (com);
		}

		virtual int getFailedCount ()
		{
			return rts2core::DevClient::getFailedCount ();
		}

		virtual void clearFailedCount ()
		{
			rts2core::DevClient::clearFailedCount ();
		}

		virtual void idle ()
		{
			DevScript::idle ();
			rts2core::DevClientPhot::idle ();
		}

		virtual void filterMoveEnd ();

		virtual void integrationStart ();
		virtual void integrationEnd ();
		virtual void addCount (int count, float exp, bool is_ov);

		virtual int getNextCommand ();

	private:
		// minFlux to be considered as success
		float minFlux;

};

}
#endif							 /* !__RTS2_DEVCLIPHOT__ */
