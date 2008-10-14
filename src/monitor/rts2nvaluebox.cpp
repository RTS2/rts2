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

using namespace rts2ncur;

ValueBox::ValueBox (Rts2NWindow * top, Rts2Value * _val)
{
	topWindow = top;
	val = _val;
}


ValueBox::~ValueBox (void)
{

}


ValueBoxBool::ValueBoxBool (Rts2NWindow * top, Rts2ValueBool * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NSelWindow (top->getX () + _x, top->getY () + _y, 10, 4)
{
	maxrow = 2;
	setLineOffset (0);
	if (!_val->getValueBool ())
		setSelRow (1);
}


keyRet
ValueBoxBool::injectKey (int key)
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
ValueBoxBool::draw ()
{
	Rts2NSelWindow::draw ();
	werase (getWriteWindow ());
	mvwprintw (getWriteWindow (), 0, 1, "true");
	mvwprintw (getWriteWindow (), 1, 1, "false");
	refresh ();
}


void
ValueBoxBool::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', getSelRow () == 0));
}


bool
ValueBoxBool::setCursor ()
{
	return false;
}


ValueBoxString::ValueBoxString (Rts2NWindow * top, Rts2Value * _val, int _x, int _y):
ValueBox (top, _val),
Rts2NWindowEdit (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%s", _val->getValue ());
}


keyRet
ValueBoxString::injectKey (int key)
{
	return Rts2NWindowEdit::injectKey (key);
}


void
ValueBoxString::draw ()
{
	Rts2NWindowEdit::draw ();
	refresh ();
}


void
ValueBoxString::sendValue (Rts2Conn * connection)
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


bool ValueBoxString::setCursor ()
{
	return Rts2NWindowEdit::setCursor ();
}


ValueBoxInteger::ValueBoxInteger (Rts2NWindow * top, Rts2ValueInteger * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NWindowEditIntegers (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	setValueInteger (_val->getValueInteger ());
}


keyRet
ValueBoxInteger::injectKey (int key)
{
	return Rts2NWindowEditIntegers::injectKey (key);
}


void
ValueBoxInteger::draw ()
{
	Rts2NWindowEditIntegers::draw ();
	refresh ();
}


void
ValueBoxInteger::sendValue (Rts2Conn * connection)
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
	connection->queCommand (new Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval));
}


bool ValueBoxInteger::setCursor ()
{
	return Rts2NWindowEditIntegers::setCursor ();
}


ValueBoxLongInteger::ValueBoxLongInteger (Rts2NWindow * top, Rts2ValueLong * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NWindowEditIntegers (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%li", _val->getValueLong ());
}


keyRet
ValueBoxLongInteger::injectKey (int key)
{
	return Rts2NWindowEditIntegers::injectKey (key);
}


void
ValueBoxLongInteger::draw ()
{
	Rts2NWindowEditIntegers::draw ();
	refresh ();
}


void
ValueBoxLongInteger::sendValue (Rts2Conn * connection)
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
	connection->queCommand (new Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval));
}


bool ValueBoxLongInteger::setCursor ()
{
	return Rts2NWindowEditIntegers::setCursor ();
}


ValueBoxFloat::ValueBoxFloat (Rts2NWindow * top, Rts2ValueFloat * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NWindowEditDigits (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%f", _val->getValueFloat ());
}


keyRet
ValueBoxFloat::injectKey (int key)
{
	return Rts2NWindowEditDigits::injectKey (key);
}


void
ValueBoxFloat::draw ()
{
	Rts2NWindowEditDigits::draw ();
	refresh ();
}


void
ValueBoxFloat::sendValue (Rts2Conn * connection)
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
	connection->queCommand (new Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', tval));
}


bool ValueBoxFloat::setCursor ()
{
	return Rts2NWindowEditDigits::setCursor ();
}


ValueBoxDouble::ValueBoxDouble (Rts2NWindow * top, Rts2ValueDouble * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NWindowEditDigits (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%f", _val->getValueDouble ());
}


keyRet
ValueBoxDouble::injectKey (int key)
{
	return Rts2NWindowEditDigits::injectKey (key);
}


void
ValueBoxDouble::draw ()
{
	Rts2NWindowEditDigits::draw ();
	refresh ();
}


void
ValueBoxDouble::sendValue (Rts2Conn * connection)
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
		getValue ()->getName (), '=', tval));
}


bool ValueBoxDouble::setCursor ()
{
	return Rts2NWindowEditDigits::setCursor ();
}


ValueBoxSelection::ValueBoxSelection (Rts2NWindow * top, Rts2ValueSelection * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NSelWindow (top->getX () + _x, top->getY () + _y, 15, 5)
{
	maxrow = 0;
	setLineOffset (0);
	setSelRow (_val->getValueInteger ());
}


keyRet ValueBoxSelection::injectKey (int key)
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
ValueBoxSelection::draw ()
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
ValueBoxSelection::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new  Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', getSelRow ()));
}


bool ValueBoxSelection::setCursor ()
{
	return false;
}


ValueBoxRectangle::ValueBoxRectangle (Rts2NWindow * top, Rts2ValueRectangle * _val, int _x, int _y)
:ValueBox (top, _val),
Rts2NWindowEdit (top->getX () + _x, top->getY () + _y, 44, 4, 1, 1, 300, 2)
{
	edtX = new Rts2NWindowEditIntegers (top->getX () + _x + 1, top->getY () + _y + 1,
		20, 1, 0, 0, 300, 1, false);
	edtY = new Rts2NWindowEditIntegers (top->getX () + _x + 1, top->getY () + _y + 2,
		20, 1, 0, 0, 300, 1, false);
	edtWidth = new Rts2NWindowEditIntegers (top->getX () + _x + 22, top->getY () + _y + 1,
		20, 1, 0, 0, 300, 1, false);
	edtHeight = new Rts2NWindowEditIntegers (top->getX () + _x + 22, top->getY () + _y + 2,
		20, 1, 0, 0, 300, 1, false);

	edtX->setValueInteger (_val->getXInt ());
	edtY->setValueInteger (_val->getYInt ());
	edtWidth->setValueInteger (_val->getWidthInt ());
	edtHeight->setValueInteger (_val->getHeightInt ());

	edtSelected = edtX;
}


ValueBoxRectangle::~ValueBoxRectangle ()
{
	edtSelected = NULL;
	delete edtX;
	delete edtY;
	delete edtWidth;
	delete edtHeight;
}


keyRet
ValueBoxRectangle::injectKey (int key)
{
	return edtSelected->injectKey (key);	
}


void
ValueBoxRectangle::draw ()
{
	// draw border..
	Rts2NWindowEdit::draw ();
	werase (getWriteWindow ());
	// draws entry boxes..
	edtX->draw ();
	edtY->draw ();
	edtWidth->draw ();
	edtHeight->draw ();

	refresh ();
	edtX->refresh ();
	edtY->refresh ();
	edtWidth->refresh ();
	edtHeight->refresh ();
}


void
ValueBoxRectangle::sendValue (Rts2Conn * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new Rts2CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', 1, 1, 20, 20));
}


bool
ValueBoxRectangle::setCursor ()
{
	return false;
}
