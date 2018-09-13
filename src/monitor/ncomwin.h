/*
 * RTS2 communication window
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

#ifndef __RTS2_NCOMWIN__
#define __RTS2_NCOMWIN__

#include "daemonwindow.h"

namespace rts2ncurses
{

class NComWin:public NWindow
{
	private:
		WINDOW * comwin;
		WINDOW *statuspad;
		std::vector <std::string> history;
		int historyPos;
	public:
		NComWin ();
		virtual ~ NComWin (void);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void winrefresh ();

		virtual void winclear ()
		{
			werase (comwin);
			NWindow::winclear ();
		}

		virtual bool setCursor ();

		virtual WINDOW *getWriteWindow () { return comwin; }

		void getWinString (char *buf, int n) { mvwinnstr (comwin, 0, 0, buf, n); }
		void printCommand (char *cmd)
		{
			mvwprintw (statuspad, 0, 0, "%s", cmd);
			int y, x;
			getyx (statuspad, y, x);
			for (; x < getWidth (); x++)
				mvwaddch (statuspad, y, x, ' ');
		}

		void commandReturn (rts2core::Command * cmd, int cmd_status);

		void addHistory (std::string cmd) { history.insert (history.begin (), cmd); historyPos = 0; }
		void showHistory (int dir);
};

}
#endif							 /* !__RTS2_NCOMWIN__ */
