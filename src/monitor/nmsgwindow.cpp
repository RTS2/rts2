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

#include "nmonitor.h"
#include "nmsgwindow.h"

using namespace rts2ncurses;

NMsgWindow::NMsgWindow ():NSelWindow (0, LINES - 19, COLS, 18, 1, 300, 500)
{
	msgMask = MESSAGE_CRITICAL | MESSAGE_ERROR | MESSAGE_WARNING | MESSAGE_INFO;
	setLineOffset (0);
	setSelRow (-1);
}

NMsgWindow::~NMsgWindow (void)
{
}

void NMsgWindow::draw ()
{
	NSelWindow::draw ();
	werase (getWriteWindow ());
	maxrow = 0;
	int i = 0;
	for (std::list < rts2core::Message >::iterator iter = messages.begin ();
		iter != messages.end () && maxrow < (padoff_y + getScrollHeight ());
		iter++, i++)
	{
		rts2core::Message msg = *iter;
		if (!msg.passMask (msgMask))
			continue;
		if (maxrow < padoff_y)
		{
			maxrow++;
			continue;
		}
		char mt;
		switch (msg.getType ())
		{
			case MESSAGE_CRITICAL:
				mt = 'C';
				wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
				break;
			case MESSAGE_ERROR:
				mt = 'E';
				wcolor_set (getWriteWindow (), CLR_FAILURE, NULL);
				break;
			case MESSAGE_WARNING:
				mt = 'W';
				wcolor_set (getWriteWindow (), CLR_WARNING, NULL);
				break;
			case MESSAGE_INFO:
				mt = 'i';
				wcolor_set (getWriteWindow (), CLR_OK, NULL);
				break;
			case MESSAGE_DEBUG:
				mt = 'd';
				wcolor_set (getWriteWindow (), CLR_TEXT, NULL);
				break;
		}
		struct tm tmesg;
		time_t t = msg.getMessageTimeSec ();
		localtime_r (&t, &tmesg);

		mvwprintw (getWriteWindow (), maxrow, 0, "%02i:%02i:%02i.%03i %c %s %s",
			tmesg.tm_hour, tmesg.tm_min, tmesg.tm_sec, (int) (msg.getMessageTimeUSec () / 1000),
			mt, msg.getMessageOName (), msg.getMessageString ().c_str ());

		wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
		maxrow++;
	}

	wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
	mvwvline (getWriteWindow (), 0, 12, ACS_VLINE, (maxrow > getHeight ()? maxrow : getHeight ()));
	mvwaddch (window, 0, 13, ACS_TTEE);
	mvwaddch (window, getHeight () - 1, 13, ACS_BTEE);

	mvwvline (getWriteWindow (), 0, 14, ACS_VLINE, (maxrow > getHeight ()? maxrow : getHeight ()));
	mvwaddch (window, 0, 15, ACS_TTEE);
	mvwaddch (window, getHeight () - 1, 15, ACS_BTEE);

	winrefresh ();
}

void NMsgWindow::add (rts2core::Message & msg)
{
	for (int tr = 0; tr < ((int) messages.size () - getScrollHeight ()); tr++)
	{
		messages.pop_front ();
	}
	messages.push_back (msg);
}
