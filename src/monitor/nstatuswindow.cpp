/* 
 * RTS2 status window.
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

#include "nmonitor.h"
#include "nstatuswindow.h"

using namespace rts2ncurses;

NStatusWindow::NStatusWindow (NComWin * in_comWin, Rts2Client * in_master):NWindow (0, LINES - 1, COLS, 1, 0)
{
	master = in_master;
	comWin = in_comWin;
}

NStatusWindow::~NStatusWindow (void)
{
}

void NStatusWindow::draw ()
{
	char dateBuf[21];
	time_t now;
	time (&now);

	NWindow::draw ();
	werase (window);

	wcolor_set (window, CLR_STATUS, NULL);
	mvwhline (window, 0, 0, ' ', getWidth ());
	mvwprintw (window, 0, 0, "%s %x %i", master->getMasterStateString ().c_str (), master->getMasterState (),
		comWin->getCurX ());
	strftime (dateBuf, 20, "%Y-%m-%d %H:%M:%S", gmtime (&now));
	mvwprintw (window, 0, COLS - 19, "%19s", dateBuf);
	wcolor_set (window, CLR_DEFAULT, NULL);

	refresh ();
}
