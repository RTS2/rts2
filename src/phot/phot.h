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

#include "../utils/rts2scriptdevice.h"
#include "status.h"

#include <sys/time.h>

/**
 * Abstract photometer class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevPhot:public Rts2ScriptDevice
{
	private:
		struct timeval nextCountDue;
		Rts2ValueInteger *count;
		Rts2ValueFloat *exp;
		Rts2ValueBool *is_ov;
		Rts2Conn * integrateConn;

	protected:
		Rts2ValueInteger *req_count;

		Rts2ValueSelection *filter;
		float req_time;
		void setReqTime (float in_req_time);

		const char *photType;
		char *serial;

		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

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

		int startIntegrate (Rts2Conn * conn, float in_req_time, int _req_count);
		virtual int stopIntegrate ();

		int homeFilter (Rts2Conn * conn);
		int moveFilter (int new_filter);
		int enableFilter (Rts2Conn * conn);

		virtual int scriptEnds ();

		virtual int changeMasterState (int new_state);

		virtual int commandAuthorized (Rts2Conn * conn);

		float getExposure () { return exp->getValueFloat (); }

	public:
		Rts2DevPhot (int argc, char **argv);
		// return time till next getCount call in usec, or -1 when failed
		virtual long getCount ()
		{
			return -1;
		}
		virtual int initValues ();

		virtual int idle ();

		virtual int deleteConnection (Rts2Conn * conn)
		{
			if (integrateConn == conn)
				integrateConn = NULL;
			return Rts2ScriptDevice::deleteConnection (conn);
		}
};
#endif							 /* !__RTS2_PHOT__ */
