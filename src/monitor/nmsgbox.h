/* 
 * Ncurses message box.
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

#ifndef __RTS2_NMSGBOX__
#define __RTS2_NMSGBOX__

#include "nwindow.h"

namespace rts2ncurses
{

class NMsgBox:public NWindow
{
	private:
		const char *query;
		const char **buttons;
		int butnum;
	public:
		NMsgBox (const char *in_query,
			const char *in_buttons[], int in_butnum);
		virtual ~ NMsgBox (void);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		/**
		 * -1 when exited with KEY_ESC, >=0 when exited with enter, it's
		 *  index of selected button
		 */
		int exitState;
};

}
#endif							 /* !__RTS2_NMSGBOX__ */
