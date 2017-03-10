/* 
 * Client for focuser attached to camera.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DEVCLI_FOCUSER__
#define __RTS2_DEVCLI_FOCUSER__

#define EVENT_FOCUSER_SET        RTS2_LOCAL_EVENT + 750
#define EVENT_FOCUSER_OFFSET     RTS2_LOCAL_EVENT + 751
#define EVENT_FOCUSER_FILTEROFFSET     RTS2_LOCAL_EVENT + 752
#define EVENT_FOCUSER_END_MOVE   RTS2_LOCAL_EVENT + 753
#define EVENT_FOCUSER_GET        RTS2_LOCAL_EVENT + 754

#include "devclient.h"

namespace rts2camd
{

struct focuserMove
{
	char *focuserName;
	float value;
	void *conn;
};

class ClientFocusCamera:public rts2core::DevClientFocus
{
	public:
		ClientFocusCamera (rts2core::Rts2Connection * in_connection);
		virtual ~ ClientFocusCamera (void);
		virtual void postEvent (rts2core::Event * event);
		virtual void focusingFailed (int status);
	protected:
		virtual void focusingEnd ();
	private:
		void *activeConn;
};

}

#endif							 /*! __RTS2_DEVCLI_FOCUSER__ */
