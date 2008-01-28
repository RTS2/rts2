/* 
 * Dialog boxes for setting values.
 * Copyright (C) 2007 Petr Kubanek <petr@kubanek.net>
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

#include "rts2nvaluebox.h"
#include "nmonitor.h"

Rts2NValueBox::Rts2NValueBox (Rts2NWindow * top, Rts2Value * in_val)
{
	topWindow = top;
	val = in_val;
}


Rts2NValueBox::~Rts2NValueBox (void)
{

}


Rts2NValueBoxBool::Rts2NValueBoxBool (Rts2NWindow * top, Rts2ValueBool * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NSelWindow (top->getX () + in_x, top->getY () + in_y, 10, 4)
{
	maxrow = 2;
	setLineOffset (0);
	if (!in_val->getValueBool ())
		setSelRow (1);
}


keyRet Rts2NValueBoxBool::injectKey (int key)
{
	switch (key)
	{
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		case KEY_EXIT:
		case K_ESC:
			return RKEY_ESC;
	}
	return Rts2NSelWindow::injectKey (key);
}


void
Rts2NValueBoxBool::draw ()
{
	Rts2NSelWindow::draw ();
	werase (getWriteWindow ());
	mvwprintw (getWriteWindow (), 0, 1, "true");
	mvwprintw (getWriteWindow (), 1, 1, "false");
	refresh ();
}


void
Rts2NValueBoxBool::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->
		queCommand (new
		Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=',
		getSelRow () == 0));
}


bool Rts2NValueBoxBool::setCursor ()
{
	return false;
}


Rts2NValueBoxString::Rts2NValueBoxString (Rts2NWindow * top, Rts2Value * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NWindowEdit (top->getX () + in_x, top->getY () + in_y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%s", in_val->getValue ());
}


keyRet
Rts2NValueBoxString::injectKey (int key)
{
	return Rts2NWindowEdit::injectKey (key);
}


void
Rts2NValueBoxString::draw ()
{
	Rts2NWindowEdit::draw ();
	refresh ();
}


void
Rts2NValueBoxString::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	int cx = getCurX ();
	char buf[cx + 1];
	mvwinnstr (getWriteWindow (), 0, 0, buf, cx);
	buf[cx] = '\0';
	connection->queCommand (new Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', std::string (buf)));
}


bool Rts2NValueBoxString::setCursor ()
{
	return Rts2NWindowEdit::setCursor ();
}


Rts2NValueBoxInteger::Rts2NValueBoxInteger (Rts2NWindow * top, Rts2ValueInteger * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NWindowEditIntegers (top->getX () + in_x, top->getY () + in_y, 20, 3, 1,
1, 300, 1)
{
	wprintw (getWriteWindow (), "%i", in_val->getValueInteger ());
}


keyRet
Rts2NValueBoxInteger::injectKey (int key)
{
	return Rts2NWindowEditIntegers::injectKey (key);
}


void
Rts2NValueBoxInteger::draw ()
{
	Rts2NWindowEditIntegers::draw ();
	refresh ();
}


void
Rts2NValueBoxInteger::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 200);
	int tval = strtol (buf, &endptr, 10);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->
		queCommand (new
		Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval));
}


bool Rts2NValueBoxInteger::setCursor ()
{
	return Rts2NWindowEditIntegers::setCursor ();
}


Rts2NValueBoxLongInteger::Rts2NValueBoxLongInteger (Rts2NWindow * top, Rts2ValueLong * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NWindowEditIntegers (top->getX () + in_x, top->getY () + in_y, 20, 3, 1,
1, 300, 1)
{
	wprintw (getWriteWindow (), "%li", in_val->getValueLong ());
}


keyRet
Rts2NValueBoxLongInteger::injectKey (int key)
{
	return Rts2NWindowEditIntegers::injectKey (key);
}


void
Rts2NValueBoxLongInteger::draw ()
{
	Rts2NWindowEditIntegers::draw ();
	refresh ();
}


void
Rts2NValueBoxLongInteger::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 200);
	long tval = strtoll (buf, &endptr, 10);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->
		queCommand (new
		Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval));
}


bool Rts2NValueBoxLongInteger::setCursor ()
{
	return Rts2NWindowEditIntegers::setCursor ();
}


Rts2NValueBoxFloat::Rts2NValueBoxFloat (Rts2NWindow * top, Rts2ValueFloat * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NWindowEditDigits (top->getX () + in_x, top->getY () + in_y, 20, 3, 1, 1,
300, 1)
{
	wprintw (getWriteWindow (), "%f", in_val->getValueFloat ());
}


keyRet
Rts2NValueBoxFloat::injectKey (int key)
{
	return Rts2NWindowEditDigits::injectKey (key);
}


void
Rts2NValueBoxFloat::draw ()
{
	Rts2NWindowEditDigits::draw ();
	refresh ();
}


void
Rts2NValueBoxFloat::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 200);
	float tval = strtof (buf, &endptr);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->
		queCommand (new
		Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval));
}


bool Rts2NValueBoxFloat::setCursor ()
{
	return Rts2NWindowEditDigits::setCursor ();
}


Rts2NValueBoxDouble::Rts2NValueBoxDouble (Rts2NWindow * top, Rts2ValueDouble * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NWindowEditDigits (top->getX () + in_x, top->getY () + in_y, 20, 3, 1, 1,
300, 1)
{
	wprintw (getWriteWindow (), "%f", in_val->getValueDouble ());
}


keyRet
Rts2NValueBoxDouble::injectKey (int key)
{
	return Rts2NWindowEditDigits::injectKey (key);
}


void
Rts2NValueBoxDouble::draw ()
{
	Rts2NWindowEditDigits::draw ();
	refresh ();
}


void
Rts2NValueBoxDouble::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 200);
	double tval = strtod (buf, &endptr);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->queCommand (new  Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval)
		);
}


bool Rts2NValueBoxDouble::setCursor ()
{
	return Rts2NWindowEditDigits::setCursor ();
}


Rts2NValueBoxSelection::Rts2NValueBoxSelection (Rts2NWindow * top, Rts2ValueSelection * in_val, int in_x, int in_y):
Rts2NValueBox (top, in_val),
Rts2NSelWindow (top->getX () + in_x, top->getY () + in_y, 15, 5)
{
	maxrow = 0;
	setLineOffset (0);
	setSelRow (in_val->getValueInteger ());
}


keyRet Rts2NValueBoxSelection::injectKey (int key)
{
	switch (key)
	{
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		case KEY_EXIT:
		case K_ESC:
			return RKEY_ESC;
	}
	return Rts2NSelWindow::injectKey (key);
}


void
Rts2NValueBoxSelection::draw ()
{
	Rts2NSelWindow::draw ();
	werase (getWriteWindow ());
	Rts2ValueSelection *vals = (Rts2ValueSelection *) getValue ();
	maxrow = 0;
	for (std::vector < SelVal >::iterator iter = vals->selBegin (); iter != vals->selEnd (); iter++, maxrow++)
	{
		mvwprintw (getWriteWindow (), maxrow, 1, (*iter).name.c_str ());
	}
	refresh ();
}


void
Rts2NValueBoxSelection::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new  Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=',
		getSelRow ())
		);
}


bool Rts2NValueBoxSelection::setCursor ()
{
	return false;
}
