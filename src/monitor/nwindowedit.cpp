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

#include "nmonitor.h"
#include "nwindowedit.h"

#include <ctype.h>

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)

NWindowEdit::NWindowEdit (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border):NWindow (_x, _y, w, h, border)
{
	ex = _ex;
	ey = _ey;
	eh = _eh;
	ew = _ew;

	// These values will be updated on first repaint
	width = 0;
	height = 0;

	comwin = newpad (_eh, _ew);
}

NWindowEdit::~NWindowEdit (void)
{
	delwin (comwin);
}

bool NWindowEdit::passKey (int key)
{
	return isprint (key);
}

int NWindowEdit::getLength ()
{
	// TODO: cache the value until next insert/delete command?
	int x0, y0;

	// Current cursor position
	getyx (getWriteWindow (), x0, y0);

	int length = MAX (0, getWriteWidth ());

	while (length > 0 && isblank (mvwinch (getWriteWindow (), getCurY (), length - 1) & A_CHARTEXT))
		length --;

	// Restore cursor position
	wmove (getWriteWindow (), x0, y0);

	return length;
}

keyRet NWindowEdit::injectKey (int key)
{
	switch (key)
	{
		case KEY_BACKSPACE:
		case 127:
			getyx (getWriteWindow (), y, x);
			mvwdelch (getWriteWindow (), y, x - 1);
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
			if (x < MIN (getLength (), getWriteWidth () - 1))
				wmove (getWriteWindow (), getCurY (), x + 1);
			break;
		case KEY_HOME:
		case KEY_CTRL ('A'):
			wmove (getWriteWindow (), getCurY (), 0);
			break;
		case KEY_END:
		case KEY_CTRL ('E'):
			wmove (getWriteWindow (), getCurY (), MIN (getWriteWidth () - 1, getLength ()));
			break;
		case KEY_CTRL ('W'):
			// Delete word backward
			getyx (getWriteWindow (), y, x);

			while (x > 0  && isblank (mvwinch (getWriteWindow (), y, x - 1) & A_CHARTEXT))
			{
				mvwdelch (getWriteWindow (), y, x - 1);
				x --;
			}

			while (x > 0  && !iswblank (mvwinch (getWriteWindow (), y, x - 1) & A_CHARTEXT))
			{
				mvwdelch (getWriteWindow (), y, x - 1);
				x --;
			}
			break;
		case KEY_CTRL ('K'):
			// Delete rest of like
			wclrtoeol (getWriteWindow ());
			break;
		case KEY_CTRL ('P'):
		case KEY_CTRL ('N'):
			// Do not propagate these to command window
			break;
		default:
			if (isprint (key))
			{
				if (passKey (key) && getCurX () < getWriteWidth ())
				{
					// TODO: make it configurable / togglable ?
					if (true)
					{
						// Insert mode behaviour
						char buf[getWriteWidth () + 1];
						getyx (getWriteWindow (), y, x);
						mvwinnstr (getWriteWindow (), y, x, buf, getWriteWidth () - x);
						mvwaddnstr (getWriteWindow (), y, x + 1, buf, getWriteWidth () - x - 1);
						mvwaddch (getWriteWindow (), y, x, key);
					}
					else
					{
						// Overwrite mode, old default behaviour
						waddch (getWriteWindow (), key);
					}

					if (getCurX () == getWriteWidth ())
						// Keep the cursor inside the window
						wmove (getWriteWindow (), getCurY (), getCurX () - 1);
				}
				// alfanum key, and it does not passed..
				else
				{
					// beep ();
					// flash ();
				}
				return RKEY_HANDLED;
			}
			return NWindow::injectKey (key);
	}
	return RKEY_HANDLED;
}

void NWindowEdit::winrefresh ()
{
	NWindow::winrefresh ();

	int x0 = getX () + ex;
	int y0 = getY () + ey;

	width = getWidth () - ex;
	height = getHeight () - ey;

	if (haveBox ())
	{
		// Upper-left coordinate is user-defined, so we have to worry only about lower and right border
		width -= 1;
		height -= 1;
	}

	width = MIN (ew, width);
	height = MIN (ew, height);

	if (x0 + width > COLS)
		width -= COLS - x0 - width;

	if (y0 + height > LINES)
		height -= LINES - y0 - height;

	if (pnoutrefresh (getWriteWindow (), 0, 0, y0, x0, y0 + height - 1, x0 + width - 1) == ERR)
		errorMove ("pnoutrefresh comwin", y0, x0, height, width);
}

void NWindowEdit::setSize (int w, int h)
{
	ew = w;
	eh = h;
	wresize (window, eh, ew);
	winrefresh ();
}

bool NWindowEdit::setCursor ()
{
	getbegyx (getWriteWindow (), y, x);
	x += getCurX ();
	y += getCurY ();
	setsyx (y, x);
	return true;
}

NWindowEditIntegers::NWindowEditIntegers (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border, bool _hex):NWindowEdit (_x, _y, w, h, _ex, _ey, _ew, _eh, border)
{
	hex = _hex;
}

bool NWindowEditIntegers::passKey (int key)
{
	if (isdigit (key) || key == '+' || key == '-' || (hex && (key == 'X' || key == 'x')))
		return true;
	return false;
}

int NWindowEditIntegers::getValueInteger ()
{
	char buf[getLength () + 1];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, getLength ());
	int tval = strtol (buf, &endptr, hex ? 0 : 10);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return 0;
	}
	return tval;
}

NWindowEditDigits::NWindowEditDigits (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border):NWindowEdit (_x, _y, w, h, _ex, _ey, _ew, _eh, border)
{
}

void NWindowEditDigits::setValueDouble (double _val)
{
	wprintw (getWriteWindow (), "%f", _val);
}

bool NWindowEditDigits::passKey (int key)
{
	if (isdigit (key) || key == '.' || key == ',' || key == '+' || key == '-')
		return true;
	return false;
}

double NWindowEditDigits::getValueDouble ()
{
	char buf[getLength () + 1];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, getLength ());
	double tval = strtod (buf, &endptr);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return 0;
	}
	return tval;
}

NWindowEditDegrees::NWindowEditDegrees (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border):NWindowEditDigits (_x, _y, w, h, _ex, _ey, _ew, _eh, border)
{
}

void NWindowEditDegrees::setValueDouble (double _val)
{
	struct ln_dms dms;
	ln_deg_to_dms (_val, &dms);
	wprintw (getWriteWindow (), dms.neg ? "-" : "+");
	if (dms.degrees > 0)
		wprintw (getWriteWindow (), "%id", dms.degrees);
	if (dms.minutes > 0)
		wprintw (getWriteWindow (), "%i'", dms.minutes);
	if (dms.seconds > 0)
		wprintw (getWriteWindow (), "%g\"", dms.seconds);

	if (dms.degrees == 0 && dms.minutes == 0 && dms.seconds == 0)
		wprintw (getWriteWindow (), "0");
}

bool NWindowEditDegrees::passKey (int key)
{
	if (key == '"' || key == '\'' || key == 'd' || key == 'm' || key == 's')
		return true;
	return NWindowEditDigits::passKey (key);
}

double NWindowEditDegrees::getValueDouble ()
{
	char buf[getLength () + 1];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, getLength ());
	char *p = buf;
	double tval = 0;
	while (*p != '\0')
	{
		double v = strtod (p, &endptr);
		switch (*endptr)
		{
			case 'd':
			case '\0':
			case ' ':
				tval += v;
				break;
			case 'm':
			case '\'':
				tval += v / 60.0;
				break;
			case 's':
			case '"':
				tval += v / 3600.0;
				break;
			default:
				return tval;
		}
		p = endptr;
		if (*p != '\0')
			p++;
	}
	return tval;
}


NWindowEditBool::NWindowEditBool (int _type, int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border):NWindowEdit (_x, _y, w, h, _ex, _ey, _ew, _eh, border)
{
	dt = _type;
}

bool NWindowEditBool::passKey (int key)
{
	return false;
}

void NWindowEditBool::setValueBool (bool _val)
{
	if (dt & RTS2_DT_ONOFF)
		wprintw (getWriteWindow (), (_val ? "on " : "off"));
	else
		wprintw (getWriteWindow (), (_val ? "true " : "false"));
}

bool NWindowEditBool::getValueBool ()
{
	char buf[getLength ()];
	mvwinnstr (getWriteWindow (), 0, 0, buf, getLength ());
	if (dt & RTS2_DT_ONOFF)
		return strncasecmp (buf, "on", 2) ? false : true;
	else
		return strncasecmp (buf, "true", 4) ? false : true;
}
