/* 
 * Dialog boxes for setting values.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
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
#include "nvaluebox.h"

using namespace rts2ncurses;

ValueBox::ValueBox (NWindow * top, rts2core::Value * _val)
{
	topWindow = top;
	val = _val;
}

ValueBox::~ValueBox (void)
{

}

ValueBoxBool::ValueBoxBool (NWindow * top, rts2core::ValueBool * _val, int _x, int _y):ValueBox (top, _val),NSelWindow (top->getX () + _x, top->getY () + _y, 10, 4)
{
	maxrow = 2;
	setLineOffset (0);
	if (!_val->getValueBool ())
		setSelRow (1);
}

keyRet ValueBoxBool::injectKey (int key)
{
	switch (key)
	{
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		case KEY_EXIT:
		case K_ESC:
			return RKEY_ESC;
		case ' ':
			changeSelRow (+1);
			return RKEY_HANDLED;
	}
	return NSelWindow::injectKey (key);
}

void ValueBoxBool::draw ()
{
	NSelWindow::draw ();
	werase (getWriteWindow ());
	if (getValue ()->getValueDisplayType () & RTS2_DT_ONOFF)
	{
		mvwprintw (getWriteWindow (), 0, 1, "on");
		mvwprintw (getWriteWindow (), 1, 1, "off");
	}
	else
	{
		mvwprintw (getWriteWindow (), 0, 1, "true");
		mvwprintw (getWriteWindow (), 1, 1, "false");
	}
	winrefresh ();
}

void ValueBoxBool::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', getSelRow () == 0));
}

bool ValueBoxBool::setCursor ()
{
	return false;
}

ValueBoxString::ValueBoxString (NWindow * top, rts2core::Value * _val, int _x, int _y):ValueBox (top, _val),NWindowEdit (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	setSize (strlen (_val->getValue ()) + 5, 3);
	wprintw (getWriteWindow (), "%s", _val->getValue ());
}

keyRet ValueBoxString::injectKey (int key)
{
	return NWindowEdit::injectKey (key);
}

void ValueBoxString::draw ()
{
	NWindowEdit::draw ();
	winrefresh ();
}

void ValueBoxString::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	int cx = getCurX ();
	char buf[cx + 1];
	mvwinnstr (getWriteWindow (), 0, 0, buf, cx);
	buf[cx] = '\0';
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', std::string (buf)));
}

bool ValueBoxString::setCursor ()
{
	return NWindowEdit::setCursor ();
}

ValueBoxInteger::ValueBoxInteger (NWindow * top, rts2core::ValueInteger * _val, int _x, int _y):ValueBox (top, _val),NWindowEditIntegers (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1, true, _val->getValueDisplayType () & RTS2_DT_HEX)
{
	setValueInteger (_val->getValueInteger ());
}

keyRet ValueBoxInteger::injectKey (int key)
{
	return NWindowEditIntegers::injectKey (key);
}

void ValueBoxInteger::draw ()
{
	NWindowEditIntegers::draw ();
	winrefresh ();
}

void ValueBoxInteger::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', getValueInteger ()));
}

bool ValueBoxInteger::setCursor ()
{
	return NWindowEditIntegers::setCursor ();
}

ValueBoxLongInteger::ValueBoxLongInteger (NWindow * top, rts2core::ValueLong * _val, int _x, int _y):ValueBox (top, _val),NWindowEditIntegers (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%li", _val->getValueLong ());
}

keyRet ValueBoxLongInteger::injectKey (int key)
{
	return NWindowEditIntegers::injectKey (key);
}

void ValueBoxLongInteger::draw ()
{
	NWindowEditIntegers::draw ();
	winrefresh ();
}

void ValueBoxLongInteger::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 199);
	long tval = strtoll (buf, &endptr, 10);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', tval));
}

bool ValueBoxLongInteger::setCursor ()
{
	return NWindowEditIntegers::setCursor ();
}

ValueBoxFloat::ValueBoxFloat (NWindow * top, rts2core::ValueFloat * _val, int _x, int _y):ValueBox (top, _val),NWindowEditDigits (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%f", _val->getValueFloat ());
}

keyRet ValueBoxFloat::injectKey (int key)
{
	return NWindowEditDigits::injectKey (key);
}

void ValueBoxFloat::draw ()
{
	NWindowEditDigits::draw ();
	winrefresh ();
}

void ValueBoxFloat::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 199);
#ifdef RTS2_HAVE_STRTOF
	float tval = strtof (buf, &endptr);
#else
	float tval = strtod (buf, &endptr);
#endif
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', tval));
}

bool ValueBoxFloat::setCursor ()
{
	return NWindowEditDigits::setCursor ();
}

ValueBoxDouble::ValueBoxDouble (NWindow * top, rts2core::ValueDouble * _val, int _x, int _y):ValueBox (top, _val),NWindowEditDigits (top->getX () + _x, top->getY () + _y, 20, 3, 1, 1, 300, 1)
{
	wprintw (getWriteWindow (), "%f", _val->getValueDouble ());
}

keyRet ValueBoxDouble::injectKey (int key)
{
	return NWindowEditDigits::injectKey (key);
}

void ValueBoxDouble::draw ()
{
	NWindowEditDigits::draw ();
	winrefresh ();
}

void ValueBoxDouble::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	char buf[200];
	char *endptr;
	mvwinnstr (getWriteWindow (), 0, 0, buf, 199);
	double tval = strtod (buf, &endptr);
	if (*endptr != '\0' && *endptr != ' ')
	{
		// log error;
		return;
	}
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', tval));
}

bool ValueBoxDouble::setCursor ()
{
	return NWindowEditDigits::setCursor ();
}

AbstractBoxSelection::AbstractBoxSelection (NWindow * top, rts2core::Value * _val, int _x, int _y):ValueBox (top, _val),NSelWindow (top->getX () + _x, top->getY () + _y, 15, 5)
{
	maxrow = 0;
	setLineOffset (0);
}

keyRet AbstractBoxSelection::injectKey (int key)
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
	return NSelWindow::injectKey (key);
}

bool AbstractBoxSelection::setCursor ()
{
	return false;
}

void AbstractBoxSelection::drawRow (const char *_text)
{
	mvwprintw (getWriteWindow (), maxrow++, 1, _text);
}

ValueBoxSelection::ValueBoxSelection (NWindow * top, rts2core::ValueSelection * _val, int _x, int _y):AbstractBoxSelection (top, _val, _x, _y)
{
	setSelRow (_val->getValueInteger ());
}

void ValueBoxSelection::draw ()
{
	NSelWindow::draw ();
	werase (getWriteWindow ());
	rts2core::ValueSelection *vals = (rts2core::ValueSelection *) getValue ();
	maxrow = 0;
	for (std::vector < rts2core::SelVal >::iterator iter = vals->selBegin (); iter != vals->selEnd (); iter++)
	{
		drawRow ((*iter).name.c_str ());
	}
	winrefresh ();
}

void ValueBoxSelection::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', getSelRow ()));
}

ValueBoxTimeDiff::ValueBoxTimeDiff (NWindow * top, rts2core::ValueTime *_val, int _x, int _y):AbstractBoxSelection (top, _val, _x, _y)
{
}

void ValueBoxTimeDiff::draw ()
{
	NSelWindow::draw ();
	werase (getWriteWindow ());
	for (maxrow = 0; maxrow < 5;)
	{
		std::ostringstream _os;
		_os << "+" << ((maxrow + 1) * 2) << " min";
		drawRow (_os.str ().c_str ());
	}
	winrefresh ();
}

void ValueBoxTimeDiff::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', time (NULL) + (getSelRow () + 1) * 120));
}

ValueBoxRectangle::ValueBoxRectangle (NWindow * top, rts2core::ValueRectangle * _val, int _x, int _y):ValueBox (top, _val), NWindowEdit (top->getX () + _x, top->getY () + _y, 29, 4, 1, 1, 300, 2)
{
	edt[0] = new NWindowEditIntegers (top->getX () + _x + 3, top->getY () + _y + 1, 10, 1, 0, 0, 300, 1, false);
	edt[1] = new NWindowEditIntegers (top->getX () + _x + 16, top->getY () + _y + 1, 10, 1, 0, 0, 300, 1, false);
	edt[2] = new NWindowEditIntegers (top->getX () + _x + 3, top->getY () + _y + 2, 10, 1, 0, 0, 300, 1, false);
	edt[3] = new NWindowEditIntegers (top->getX () + _x + 16, top->getY () + _y + 2, 10, 1, 0, 0, 300, 1, false);

	edt[0]->setValueInteger (_val->getXInt ());
	edt[1]->setValueInteger (_val->getYInt ());
	edt[2]->setValueInteger (_val->getWidthInt ());
	edt[3]->setValueInteger (_val->getHeightInt ());

	edtSelected = 0;
}

ValueBoxRectangle::~ValueBoxRectangle ()
{
	edtSelected = -1;
	for (int i = 0; i < 4; i++)
		delete edt[i];
}

keyRet ValueBoxRectangle::injectKey (int key)
{
	switch (key)
	{
		case '\t':
		case KEY_STAB:
			edt[edtSelected]->setNormal ();
			edtSelected = (edtSelected + 1) % 4;
			draw ();
			return RKEY_HANDLED;
		case KEY_BTAB:
			edt[edtSelected]->setNormal ();
			if (edtSelected == 0)
				edtSelected = 3;
			else
				edtSelected--;
			draw ();
			return RKEY_HANDLED;
		case KEY_UP:
		case KEY_DOWN:
			edt[edtSelected]->setNormal ();
			edtSelected = (edtSelected > 1 ? 0 : 2) + (edtSelected % 2);
			draw ();
			return RKEY_HANDLED;
	}

	return edt[edtSelected]->injectKey (key);	
}

void ValueBoxRectangle::draw ()
{
	// draw border..
	NWindowEdit::draw ();
	werase (getWriteWindow ());
	// draws entry boxes..
	for (int i = 0; i < 4; i++)
		edt[i]->draw ();

	edt[edtSelected]->setUnderline ();

	// draws labels..
	mvwprintw (getWriteWindow (), 0, 0, "x:");
	mvwprintw (getWriteWindow (), 0, 13, "y:");
	mvwprintw (getWriteWindow (), 1, 0, "w:");
	mvwprintw (getWriteWindow (), 1, 13, "h:");

	winrefresh ();
	for (int i = 0; i < 4; i++)
		edt[i]->winrefresh ();
}

void ValueBoxRectangle::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (),
		getValue ()->getName (), '=', edt[0]->getValueInteger (), edt[1]->getValueInteger (),
		edt[2]->getValueInteger (), edt[3]->getValueInteger ()));
}

bool ValueBoxRectangle::setCursor ()
{
	return edt[edtSelected]->setCursor ();
}

ValueBoxArray::ValueBoxArray (NWindow * top, rts2core::ValueArray * _val, int _x, int _y):ValueBox (top, _val), NWindowEdit (top->getX () + _x, top->getY () + _y, 29, 3, 1, 1, 300, 1)
{
	int w = 0;
	for (size_t i = 0; i < _val->size (); i++)
	{
		switch (_val->getValueBaseType ())
		{
			case RTS2_VALUE_DOUBLE:
				{
					NWindowEditDigits *digit = new NWindowEditDigits (top->getX () + _x + 1 + w, top->getY () + _y + 1, 10, 1, 0, 0, 300, 1, false);
					digit->setValueDouble ((*((rts2core::DoubleArray *) _val))[i]);
					edt.push_back (digit);
					w += 11;
				}
				break;
			case RTS2_VALUE_INTEGER:
				{
					NWindowEditIntegers *inte = new NWindowEditIntegers (top->getX () + _x + 1 + w, top->getY () + _y + 1, 10, 1, 0, 0, 300, 1, false);
					inte->setValueInteger ((*((rts2core::IntegerArray *) _val))[i]);
					edt.push_back (inte);
					w += 11;
				}
				break;
			case RTS2_VALUE_BOOL:
				{
					int cw = ((i + 1) == _val->size () ) ? (_val->getValueDisplayType () & RTS2_DT_ONOFF ? 3 : 5) : 6;
					NWindowEditBool *winbool = new NWindowEditBool (_val->getValueDisplayType (), top->getX () + _x + 1 + w, top->getY () + _y + 1, cw, 1, 0, 0, 300, 1, false);
					winbool->setValueBool ((*((rts2core::BoolArray *) _val))[i]);
					edt.push_back (winbool);
					w += 6;
				}
				break;
		}
	}
	setWidth (w + 2);

	edtSelected = 0;
}

ValueBoxArray::~ValueBoxArray ()
{
	for (std::vector <NWindowEdit *>::iterator iter = edt.begin (); iter != edt.end (); iter++)
		delete *iter;
}

keyRet ValueBoxArray::injectKey (int key)
{
	switch (key)
	{
		case '\t':
		case KEY_STAB:
			edt[edtSelected]->setNormal ();
			edtSelected = (edtSelected + 1) % edt.size ();
			draw ();
			return RKEY_HANDLED;
		case KEY_BTAB:
			edt[edtSelected]->setNormal ();
			edtSelected = (edtSelected - 1) % edt.size ();
			draw ();
			return RKEY_HANDLED;
		case KEY_LEFT:
		case KEY_RIGHT:
			edt[edtSelected]->setNormal ();
			edtSelected = (edtSelected + (key == KEY_RIGHT ? 1 : -1 )) % edt.size ();
			draw ();
			return RKEY_HANDLED;
		case ' ':
			if (getValue ()->getFlags () & RTS2_VALUE_BOOL)
			{
				NWindowEditBool *winbool = (NWindowEditBool *) (edt[edtSelected]);
				winbool->setValueBool (!winbool->getValueBool ());
				draw ();
				return RKEY_HANDLED;
			}
			break;
	}
	if (edtSelected < edt.size ())
		return edt[edtSelected]->injectKey (key);
	return NWindowEdit::injectKey (key);
}

void ValueBoxArray::draw ()
{
	// draw border..
	NWindowEdit::draw ();
	werase (getWriteWindow ());

	if (edt.size () > (size_t) edtSelected)
		edt[edtSelected]->setReverse ();

	// draws entry boxes..
	for (size_t i = 0; i < edt.size (); i++)
		edt[i]->draw ();

	winrefresh ();
	for (size_t i = 0; i < edt.size (); i++)
		edt[i]->winrefresh ();
}

void ValueBoxArray::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	std::ostringstream os;
	for (size_t i = 0; i < edt.size (); i++)
	{
		switch (getValue ()->getValueBaseType ())
		{
			case RTS2_VALUE_DOUBLE:
				os << ((NWindowEditDigits *)edt[i])->getValueDouble ();
				break;
			case RTS2_VALUE_INTEGER:
				os << ((NWindowEditIntegers *)edt[i])->getValueInteger ();
				break;
			case RTS2_VALUE_BOOL:
				os << ((NWindowEditBool *)edt[i])->getValueBool ();
				break;
		}
		os << " ";
	}
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', os.str (), true));
}

bool ValueBoxArray::setCursor ()
{
	if (edtSelected < edt.size ())
		return edt[edtSelected]->setCursor ();
	return false;
}

ValueBoxPair::ValueBoxPair (NWindow * top, rts2core::ValueRaDec * _val, int _x, int _y, const char *p1, const char *p2):ValueBox (top, _val),NWindowEdit (top->getX () + _x, top->getY () + _y, 35, 3, 1, 1, 300, 1)
{
	edt[0] = new NWindowEditDigits (top->getX () + _x + 5, top->getY () + _y + 1, 10, 1, 0, 0, 300, 1, false);
	edt[1] = new NWindowEditDigits (top->getX () + _x + 20, top->getY () + _y + 1, 10, 1, 0, 0, 300, 1, false);

	edt[0]->setValueDouble (_val->getRa ());
	edt[1]->setValueDouble (_val->getDec ());

	p1name = p1;
	p2name = p2;

	edtSelected = 0;
}

ValueBoxPair::~ValueBoxPair ()
{
	edtSelected = -1;
	for (int i = 0; i < 2; i++)
		delete edt[i];
}

keyRet ValueBoxPair::injectKey (int key)
{
	switch (key)
	{
		case '\t':
		case KEY_STAB:
			edt[edtSelected]->setNormal ();
			edtSelected = (edtSelected + 1) % 2;
			draw ();
			return RKEY_HANDLED;
		case KEY_BTAB:
			edt[edtSelected]->setNormal ();
			if (edtSelected == 0)
				edtSelected = 1;
			else
				edtSelected--;
			draw ();
			return RKEY_HANDLED;
	}

	return edt[edtSelected]->injectKey (key);	
}

void ValueBoxPair::draw ()
{
	// draw border..
	NWindowEdit::draw ();
	werase (getWriteWindow ());
	// draws entry boxes..
	for (int i = 0; i < 2; i++)
		edt[i]->draw ();

	edt[edtSelected]->setUnderline ();

	// draws labels..
	mvwprintw (getWriteWindow (), 0, 0, "%-3s:", p1name);
	mvwprintw (getWriteWindow (), 0, 15, "%-3s:", p2name);

	winrefresh ();
	for (int i = 0; i < 2; i++)
		edt[i]->winrefresh ();
}

void ValueBoxPair::sendValue (rts2core::Connection * connection)
{
	if (!connection->getOtherDevClient ())
		return;
	connection->queCommand (new rts2core::CommandChangeValue (connection->getOtherDevClient (), getValue ()->getName (), '=', edt[0]->getValueDouble (), edt[1]->getValueDouble ()));
}

bool ValueBoxPair::setCursor ()
{
	return edt[edtSelected]->setCursor ();
}
