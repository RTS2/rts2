/* 
 * Device window display.
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

#include "nmonitor.h"
#include "ndevicewindow.h"

#include "../utils/timestamp.h"
#include "../utils/rts2displayvalue.h"
#include "../utils/riseset.h"

using namespace rts2ncurses;

NDeviceWindow::NDeviceWindow (Rts2Conn * in_connection):NSelWindow (10, 1, COLS - 10, LINES - 25)
{
	connection = in_connection;
	connection->resetInfoTime ();
	valueBox = NULL;
	valueBegins = 20;
	draw ();
}

NDeviceWindow::~NDeviceWindow ()
{
}

void NDeviceWindow::printState ()
{
	wattron (window, A_REVERSE);
	if (connection->getErrorState ())
		wcolor_set (window, CLR_FAILURE, NULL);
	mvwprintw (window, 0, 2, "%s %s (%x) %x", connection->getName (), connection->getStateString ().c_str (), connection->getState (), connection->getFullBopState ());
	wcolor_set (window, CLR_DEFAULT, NULL);
	wattroff (window, A_REVERSE);
}

void NDeviceWindow::printValue (const char *name, const char *value, bool writeable)
{
	wprintw (getWriteWindow (), "%c %-20s %30s\n", (writeable ? 'W' : ' '), name, value);
}

void NDeviceWindow::printValue (Rts2Value * value)
{
	// customize value display
	std::ostringstream _os;
	if (value->getWriteToFits ())
		wcolor_set (getWriteWindow (), CLR_FITS, NULL);
	else
		wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
	// ultra special handling of SCRIPT value
	if (value->getValueDisplayType () == RTS2_DT_SCRIPT)
	{
		wprintw (getWriteWindow (), "  %-20s ", value->getName ().c_str ());
		wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
		const char *valStart = value->getValue ();
		if (!valStart)
			return;
		const char *valTop = valStart;
		int scriptPosition = connection->getValueInteger ("scriptPosition");
		int scriptEnd = connection->getValueInteger ("scriptLen") + scriptPosition;

		while (*valTop && (valTop - valStart < scriptPosition))
		{
			waddch (getWriteWindow (), *valTop);
			valTop++;
		}
		wcolor_set (getWriteWindow (), CLR_SCRIPT_CURRENT, NULL);
		while (*valTop && (valTop - valStart < scriptEnd))
		{
			waddch (getWriteWindow (), *valTop);
			valTop++;
		}
		wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
		while (*valTop)
		{
			waddch (getWriteWindow (), *valTop);
			valTop++;
		}
		waddch (getWriteWindow (), '\n');
		return;
	}
	switch (value->getValueType ())
	{
		case RTS2_VALUE_RADEC:
			{
				if (value->getValueDisplayType () == RTS2_DT_DEGREES)
				{
					LibnovaDeg v_rd (((Rts2ValueRaDec *) value)->getRa ());
					LibnovaDeg v_dd (((Rts2ValueRaDec *) value)->getDec ());
					_os << v_rd << " " << v_dd;
				}
				else
				{
					LibnovaRaDec v_radec (((Rts2ValueRaDec *) value)->getRa (), ((Rts2ValueRaDec *) value)->getDec ());
					_os << v_radec;
				}
				printValue (value->getName ().c_str (), _os.str().c_str (), value->isWritable ());
			}
			break;
		case RTS2_VALUE_ALTAZ:
			{
				LibnovaHrz hrz (((Rts2ValueAltAz *) value)->getAlt (), ((Rts2ValueAltAz *) value)->getAz ());
				_os << hrz;
				printValue (value->getName ().c_str (), _os.str().c_str (), value->isWritable ());
			}
			break;
		case RTS2_VALUE_SELECTION:
			wprintw (getWriteWindow (), "%c %-20s %5i %24s\n", value->isWritable () ? 'W' : ' ', value->getName ().c_str (), value->getValueInteger (), ((Rts2ValueSelection *) value)->getSelName ());
			break;
		default:
			printValue (value->getName ().c_str (), getDisplayValue (value).c_str (), value->isWritable ());
	}
}

void NDeviceWindow::drawValuesList ()
{
	gettimeofday (&tvNow, NULL);
	now = tvNow.tv_sec + tvNow.tv_usec / USEC_SEC;

	maxrow = 0;

	for (Rts2ValueVector::iterator iter = connection->valueBegin (); iter != connection->valueEnd (); iter++)
	{
		maxrow++;
		printValue (*iter);
	}
}

Rts2Value * NDeviceWindow::getSelValue ()
{
	int s = getSelRow ();
	if (s >= 0)
		return connection->valueAt (s);
	return NULL;
}

void NDeviceWindow::printValueDesc (Rts2Value * val)
{
	wattron (window, A_REVERSE);
	mvwprintw (window, getHeight () - 1, 2, "D: \"%s\"",
		val->getDescription ().c_str ());
	wattroff (window, A_REVERSE);
}

void NDeviceWindow::endValueBox ()
{
	delete valueBox;
	valueBox = NULL;
}

void NDeviceWindow::createValueBox ()
{
	int s = getSelRow ();
	if (s < 0)
		return;
	Rts2Value *val = connection->valueAt (s);
	if (!val || val->isWritable () == false)
		return;
	s -= getPadoffY ();
	switch (val->getValueType ())
	{
		case RTS2_VALUE_BOOL:
			valueBox = new ValueBoxBool (this, (Rts2ValueBool *) val, 21, s - 1);
			break;
		case RTS2_VALUE_STRING:
			valueBox = new ValueBoxString (this, (Rts2ValueString *) val, 21, s - 1);
			break;
		case RTS2_VALUE_INTEGER:
			valueBox = new ValueBoxInteger (this, (Rts2ValueInteger *) val, 21, s);
			break;
		case RTS2_VALUE_LONGINT:
			valueBox = new ValueBoxLongInteger (this, (Rts2ValueLong *) val, 21, s);
			break;
		case RTS2_VALUE_FLOAT:
			valueBox = new ValueBoxFloat (this, (Rts2ValueFloat *) val, 21, s);
			break;
		case RTS2_VALUE_DOUBLE:
		case RTS2_VALUE_MMAX | RTS2_VALUE_DOUBLE:
			valueBox = new ValueBoxDouble (this, (Rts2ValueDouble *) val, 21, s);
			break;
		case RTS2_VALUE_SELECTION:
			valueBox = new ValueBoxSelection (this, (Rts2ValueSelection *) val, 21, s);
			break;
		case RTS2_VALUE_TIME:
			valueBox = new ValueBoxTimeDiff (this, (Rts2ValueTime *) val, 21, s);
			break;
		case RTS2_VALUE_RADEC:
			valueBox = new ValueBoxRaDec (this, (Rts2ValueRaDec *) val, 21, s);
			break;
		default:
			switch (val->getValueExtType ())
			{
				case RTS2_VALUE_RECTANGLE:
					valueBox = new ValueBoxRectangle (this, (Rts2ValueRectangle *) val, 21, s - 1);
					break;
				case RTS2_VALUE_ARRAY:
					valueBox = new ValueBoxArray (this, (rts2core::ValueArray *) val, 21, s);
					break;
				default:
					logStream (MESSAGE_WARNING) << "cannot find box for value '" <<  val->getName () << " type " << val->getValueType () << sendLog;
					valueBox = new ValueBoxString (this, val, 21, s - 1);
					break;

			}
			break;
	}
}

keyRet NDeviceWindow::injectKey (int key)
{
	keyRet
		ret;
	switch (key)
	{
		case KEY_ENTER:
		case K_ENTER:
			// don't create new box if one already exists
			if (valueBox)
				break;
		case KEY_F (6):
			if (valueBox)
				endValueBox ();
			createValueBox ();
			return RKEY_HANDLED;
	}
	if (valueBox)
	{
		ret = valueBox->injectKey (key);
		if (ret == RKEY_ENTER || ret == RKEY_ESC)
		{
			if (ret == RKEY_ENTER)
				valueBox->sendValue (connection);
			endValueBox ();
			return RKEY_HANDLED;
		}
		return ret;
	}
	return NSelWindow::injectKey (key);
}

void NDeviceWindow::draw ()
{
	NSelWindow::draw ();
	werase (getWriteWindow ());
	drawValuesList ();

	wcolor_set (getWriteWindow (), CLR_DEFAULT, NULL);
	mvwvline (getWriteWindow (), 0, valueBegins, ACS_VLINE,	(maxrow > getHeight ()? maxrow + 1 : getHeight ()));
	mvwaddch (window, 0, valueBegins + 1, ACS_TTEE);
	mvwaddch (window, getHeight () - 1, valueBegins + 1, ACS_BTEE);

	printState ();
	Rts2Value *val = getSelValue ();
	if (val != NULL)
		printValueDesc (val);
	winrefresh ();
}

void NDeviceWindow::winrefresh ()
{
	NSelWindow::winrefresh ();
	if (valueBox)
		valueBox->draw ();
}

bool NDeviceWindow::setCursor ()
{
	if (valueBox)
		return valueBox->setCursor ();
	return NSelWindow::setCursor ();
}

NDeviceCentralWindow::NDeviceCentralWindow (Rts2Conn * in_connection):NDeviceWindow (in_connection)
{
}

NDeviceCentralWindow::~NDeviceCentralWindow (void)
{
}

void NDeviceCentralWindow::printValues ()
{
	// print statusChanges

	Rts2Value *nextState = getConnection ()->getValue ("next_state");
	if (nextState && nextState->getValueType () == RTS2_VALUE_SELECTION)
	{
		for (std::vector < FutureStateChange >::iterator iter = stateChanges.begin (); iter != stateChanges.end (); iter++)
		{
			std::ostringstream _os;
			_os << LibnovaDateDouble ((*iter).getEndTime ()) << " (" << TimeDiff (now, (*iter).getEndTime ()) << ")";

			printValue (((Rts2ValueSelection *) nextState)->getSelName ((*iter).getState ()), _os.str ().c_str (), false);
		}
	}
}

void NDeviceCentralWindow::drawValuesList ()
{
	NDeviceWindow::drawValuesList ();

	if (!getConnection ()->infoTimeChanged ())
	{
		printValues ();
		return;
	}

	Rts2Value *valLng = getConnection ()->getValue ("longitude");
	Rts2Value *valLat = getConnection ()->getValue ("latitude");

	if (valLng && valLat && !isnan (valLng->getValueDouble ()) && !isnan (valLat->getValueDouble ()))
	{
		struct ln_lnlat_posn observer;

		observer.lng = valLng->getValueDouble ();
		observer.lat = valLat->getValueDouble ();

		// get next night, or get beginnign of current night

		Rts2Value *valNightHorizon =
			getConnection ()->getValue ("night_horizon");
		Rts2Value *valDayHorizon = getConnection ()->getValue ("day_horizon");

		Rts2Value *valEveningTime = getConnection ()->getValue ("evening_time");
		Rts2Value *valMorningTime = getConnection ()->getValue ("morning_time");

		if (valNightHorizon
			&& valDayHorizon
			&& !isnan (valNightHorizon->getValueDouble ())
			&& !isnan (valDayHorizon->getValueDouble ()) && valEveningTime
			&& valMorningTime)
		{
			stateChanges.clear ();

			int curr_type = -1, next_type = -1;
			time_t t_start;
			time_t t_start_t;
			time_t ev_time = tvNow.tv_sec;

			while (ev_time < (tvNow.tv_sec + 86400))
			{
				t_start = ev_time;
				t_start_t = ev_time + 1;
				next_event (&observer, &t_start_t, &curr_type, &next_type,
					&ev_time, valNightHorizon->getValueDouble (),
					valDayHorizon->getValueDouble (),
					valEveningTime->getValueInteger (),
					valMorningTime->getValueInteger ());
				/**
				if (curr_type == SERVERD_DUSK)
				{
					nightStart->setValueTime (ev_time);
				}
				if (curr_type == SERVERD_NIGHT)
				{
					nightStop->setValueTime (ev_time);
				} **/
				stateChanges.push_back (FutureStateChange (curr_type, t_start));
			}
		}

	}
	printValues ();
}
