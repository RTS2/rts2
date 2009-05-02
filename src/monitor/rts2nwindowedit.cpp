/* 
 * Windows for edditing various variables.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
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

#include "rts2nwindowedit.h"
#include "nmonitor.h"

#include <ctype.h>

#define MIN(x,y) ((x < y) ? x : y)

Rts2NWindowEdit::Rts2NWindowEdit (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border)
:Rts2NWindow (_x, _y, w, h, border)
{
	ex = _ex;
	ey = _ey;
	eh = _eh;
	ew = _ew;
	comwin = newpad (_eh, _ew);
}


Rts2NWindowEdit::~Rts2NWindowEdit (void)
{
	delwin (comwin);
}


bool
Rts2NWindowEdit::passKey (int key)
{
	return isalnum (key) || isspace (key) || key == '.'  || key == ',';
}


keyRet
Rts2NWindowEdit::injectKey (int key)
{
	switch (key)
	{
		case KEY_BACKSPACE:
		case 127:
			getyx (comwin, y, x);
			mvwdelch (comwin, y, x - 1);
			return RKEY_HANDLED;
		case KEY_EXIT:
		case K_ESC:
			return RKEY_ESC;
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		case KEY_LEFT:
			x = getCurX ();
			if (x > 0)
				wmove (getWriteWindow (), getCurY (), x - 1);
			break;
		case KEY_RIGHT:
			x = getCurX ();
			if (x < getWidth () - 3)
				wmove (getWriteWindow (), getCurY (), x + 1);
			break;
		default:
			if (isalnum (key) || isspace (key) || key == '+' || key == '-'
				|| key == '.' || key == ',')
			{
				if (passKey (key))
				{
					waddch (getWriteWindow (), key);
				}
				// alfanum key, and it does not passed..
				else
				{
					beep ();
					flash ();
				}
				return RKEY_HANDLED;
			}
			return Rts2NWindow::injectKey (key);
	}
	return RKEY_HANDLED;
}


void
Rts2NWindowEdit::refresh ()
{
	int w, h;
	Rts2NWindow::refresh ();
	getbegyx (window, y, x);
	getmaxyx (window, h, w);
	// window coordinates
	int mwidth = x + w;
	int mheight = y + h;
	if (haveBox ())
	{
		mwidth -= 2;
		mheight -= 1;
	}

	if (pnoutrefresh (getWriteWindow (), 0, 0, y + ey, x + ex,
		MIN (y + ey + eh, mheight),
		MIN (x + ex + ew, mwidth)) == ERR)
	{
		errorMove ("pnoutrefresh comwin", y + ey, x + ey,
			MIN (y + ey + eh, mheight), MIN (x + ex + ew, mwidth));
	}
}


bool
Rts2NWindowEdit::setCursor ()
{
	getbegyx (getWriteWindow (), y, x);
	x += getCurX ();
	y += getCurY ();
	setsyx (y, x);
	return true;
}


Rts2NWindowEditIntegers::Rts2NWindowEditIntegers (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border)
:Rts2NWindowEdit (_x, _y, w, h, _ex, _ey, _ew, _eh, border)
{
}


bool
Rts2NWindowEditIntegers::passKey (int key)
{
	if (isdigit (key) || key == '+' || key == '-')
		return true;
	return false;
}


int
Rts2NWindowEditIntegers::getValueInteger ()
{
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 200);
	int tval = strtol (buf, &endptr, 10);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return 0;
	}
	return tval;
}


Rts2NWindowEditDigits::Rts2NWindowEditDigits (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border)
:Rts2NWindowEdit (_x, _y, w, h, _ex, _ey, _ew, _eh, border)
{
}


bool
Rts2NWindowEditDigits::passKey (int key)
{
	if (isdigit (key) || key == '.' || key == ',' || key == '+' || key == '-')
		return true;
	return false;
}


int
Rts2NWindowEditDigits::getValueDouble ()
{
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 200);
	double tval = strtod (buf, &endptr);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return 0;
	}
	return tval;
}
