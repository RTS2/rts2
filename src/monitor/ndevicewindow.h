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

#ifndef __RTS2_NDEVICEWINDOW__
#define __RTS2_NDEVICEWINDOW__

#include "daemonwindow.h"
#include "nvaluebox.h"

namespace rts2ncurses
{

class NDeviceWindow:public NSelWindow
{
	public:
		NDeviceWindow (rts2core::Connection * in_connection, bool _hide_debug);
		virtual ~ NDeviceWindow (void);

		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void winrefresh ();
		virtual bool setCursor ();
		virtual bool hasEditBox () { return valueBox != NULL; }

	protected:
		double now;
		struct timeval tvNow;

		rts2core::Connection *getConnection ()
		{
			return connection;
		}

		/**
		 * Prints value name and value. Adds newline, so next value will be printed
		 * on next line.
		 *
		 * @param name       name of variable
		 * @param value      value of variable
		 * @param writable   if value is writable
		 */
		void printValue (const char *name, const char *value, bool writable);

		/**
		 * Prints out one value. Adds newline, so next value will be
		 * printed on next line.
		 *
		 * @param value  Value to print.
		 */
		void printValue (rts2core::Value * value);

		virtual void drawValuesList ();

	private:
		WINDOW * valueList;
		rts2core::Connection *connection;
		void printState ();
		rts2core::Value *getSelValue ();
		void printValueDesc (rts2core::Value * val);
		void endValueBox ();
		void createValueBox ();
		ValueBox *valueBox;
		/** Index from which start value box */
		int valueBegins;
		bool hide_debug;
		
		// draw only those values
		std::vector <rts2core::Value *> displayValues;
};

/**
 * Helper class, which holds informations about future state changes.
 */
class FutureStateChange
{
	public:
		FutureStateChange (int in_state, double in_endTime) { state = in_state; endTime = in_endTime; }

		int getState () { return state; }

		double getEndTime () { return endTime; }

	private:
		int state;
		double endTime;
};

/**
 * Class used to display extra centrald values, which are calculated on client side.
 */
class NDeviceCentralWindow:public NDeviceWindow
{
	public:
		NDeviceCentralWindow (rts2core::Connection * in_connection);
		virtual ~ NDeviceCentralWindow (void);

	protected:
		virtual void drawValuesList ();

	private:
		std::vector < FutureStateChange > stateChanges;

		void printValues ();
};

}
#endif							 /* ! __RTS2_NDEVICEWINDOW__ */
