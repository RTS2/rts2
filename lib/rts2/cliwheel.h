/* 
 * Client for filter wheel attached to the camera.
 * Copyright (C) 2005-2008,2012 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_DEVCLI_CAM_WHEEL__
#define __RTS2_DEVCLI_CAM_WHEEL__

#include "devclient.h"

#define EVENT_FILTER_START_MOVE RTS2_LOCAL_EVENT + 650
#define EVENT_FILTER_MOVE_END RTS2_LOCAL_EVENT + 651
#define EVENT_FILTER_GET  RTS2_LOCAL_EVENT + 652

namespace rts2camd
{

class FilterVal;

struct filterStart
{
	const char *filterName;
	int filter;
	void *arg;
};

class ClientFilterCamera:public rts2core::DevClientFilter
{
	public:
		ClientFilterCamera (rts2core::Rts2Connection * conn, FilterVal *fv);
		virtual ~ ClientFilterCamera (void);
		virtual void filterMoveFailed (int status);
		virtual void postEvent (rts2core::Event * event);
		virtual void valueChanged (rts2core::Value * value);
	protected:
		virtual void filterMoveEnd ();
	private:
		FilterVal *filterVal;
};

}

#endif							 /* !__RTS2_DEVCLI_CAM_WHEEL__ */
