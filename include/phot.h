/* 
 * Photometer daemon.
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

/**
 * @file Photometer deamon.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#ifndef __RTS2_PHOT__
#define __RTS2_PHOT__

#include "scriptdevice.h"
#include "status.h"

#include <sys/time.h>

#define PHOT_EVENT_CHECK    RTS2_LOCAL_EVENT + 1250

namespace rts2phot
{

/**
 * Abstract photometer class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Photometer:public rts2core::ScriptDevice
{
	public:
		Photometer (int argc, char **argv);
		// return time till next getCount call in usec, or -1 when failed
		virtual long getCount ()
		{
			return -1;
		}
		virtual int initValues ();

		virtual int idle ();

		virtual int deleteConnection (rts2core::Connection * conn)
		{
			if (integrateConn == conn)
				integrateConn = NULL;
			return ScriptDevice::deleteConnection (conn);
		}

	protected:
		rts2core::ValueInteger *req_count;

		rts2core::ValueSelection *filter;
		float req_time;
		void setReqTime (float in_req_time);

		const char *photType;
		char *serial;

		virtual void postEvent (rts2core::Event *event);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);

		void sendCount (int in_count, float in_exp, bool in_is_ov);
		virtual int startIntegrate ();
		virtual int endIntegrate ();

		virtual int homeFilter ();

		void checkFilterMove ();

		virtual int setExposure (float _exp);

		virtual int startFilterMove (int new_filter);
		virtual long isFilterMoving ();
		virtual int endFilterMove ();
		virtual int enableMove ();
		virtual int disableMove ();

		int startIntegrate (rts2core::Connection * conn, float in_req_time, int _req_count);
		virtual int stopIntegrate ();

		int homeFilter (rts2core::Connection * conn);
		int moveFilter (int new_filter);
		int enableFilter (rts2core::Connection * conn);

		virtual int scriptEnds ();

		virtual void changeMasterState (rts2_status_t old_state, rts2_status_t new_state);

		virtual int commandAuthorized (rts2core::Connection * conn);

		float getExposure () { return exp->getValueFloat (); }

	private:
		struct timeval nextCountDue;
		rts2core::ValueInteger *count;
		rts2core::ValueFloat *exp;
		rts2core::ValueBool *is_ov;
		rts2core::Connection * integrateConn;
};

}
#endif							 /* !__RTS2_PHOT__ */
