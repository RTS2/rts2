/* 
 * Client for filter wheel attached to the camera.
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

#ifndef __RTS2_DEVCLI_CAM_WHEEL__
#define __RTS2_DEVCLI_CAM_WHEEL__

#include "../utils/rts2devclient.h"

#define EVENT_FILTER_START_MOVE RTS2_LOCAL_EVENT + 650
#define EVENT_FILTER_MOVE_END RTS2_LOCAL_EVENT + 651
#define EVENT_FILTER_GET  RTS2_LOCAL_EVENT + 652

namespace rts2camd
{

struct filterStart
{
	char *filterName;
	int filter;
};

class ClientFilterCamera:public Rts2DevClientFilter
{
	protected:
		virtual void filterMoveEnd ();
	public:
		ClientFilterCamera (Rts2Conn * conn);
		virtual ~ ClientFilterCamera (void);
		virtual void filterMoveFailed (int status);
		virtual void postEvent (Rts2Event * event);
		virtual void valueChanged (Rts2Value * value);
};

};

#endif							 /* !__RTS2_DEVCLI_CAM_WHEEL__ */
