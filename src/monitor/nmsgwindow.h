/* 
 * RTS2 messages window.
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_NMSGWINDOW__
#define __RTS2_NMSGWINDOW__

#include "daemonwindow.h"
#include "../utils/rts2message.h"

#include <list>

namespace rts2ncurses
{

/**
 * Message window class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class NMsgWindow:public NSelWindow
{
	public:
		NMsgWindow ();
		virtual ~ NMsgWindow (void);
		virtual void draw ();
		friend NMsgWindow & operator << (NMsgWindow & msgwin, Rts2Message & msg)
		{
			msgwin.add (msg);
			return msgwin;
		}

		void add (Rts2Message & msg);
		void setMsgMask (int new_msgMask) { msgMask = new_msgMask; }
	private:
		std::list < Rts2Message > messages;
		int msgMask;
};

}

#endif							 /* !__RTS2_NMSGWINDOW__ */
