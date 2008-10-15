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

#ifndef __RTS2_NWINDOWEDIT__
#define __RTS2_NWINDOWEDIT__

#include "rts2nwindow.h"

/**
 * This is window with single edit box to edit values
 */
class Rts2NWindowEdit:public Rts2NWindow
{
	private:
		WINDOW * comwin;
		int ex, ey, ew, eh;
		// current cursor position
		int x, y;
	protected:
		/**
		 * Returns true if key should be wadded to comwin.
		 * Returns false if the key is not supported (e.g. numeric 
		 * edit box don't want to get alnum). Default is to return isalnum (key).
		 */
		virtual bool passKey (int key);
	public:
		/**
		 * Creates edit window.
		 *
		 * @param _x Screen X coordinate of the window.
		 * @param _y Screen Y coordinate of the window.
		 * @param w  Width of the window in characters.
		 * @param h  Height of the window in characters.
		 * @param ex, ey, ew and eh are position of (single) edit box within this window.
		 * @param border  True if border will be presented.
		 */
		Rts2NWindowEdit (int _x, int _y, int w, int h,
			int _ex, int _ey, int _ew, int _eh, bool border = true);
		virtual ~ Rts2NWindowEdit (void);

		virtual keyRet injectKey (int key);
		virtual void refresh ();

		virtual bool setCursor ();

		virtual WINDOW *getWriteWindow ()
		{
			return comwin;
		}
};

/**
 * This is window with edit box for integers
 */
class Rts2NWindowEditIntegers:public Rts2NWindowEdit
{
	protected:
		virtual bool passKey (int key);
	public:
		Rts2NWindowEditIntegers (int _x, int _y, int w, int h,
			int _ex, int _ey, int _ew, int _eh, bool border = true);
		
		/**
		 * Sets edit window value.
		 */
		void setValueInteger (int _val)
		{
			wprintw (getWriteWindow (), "%i", _val);
		}

		/**
		 * Returns integer value of the field. Returns 0 if field is empty.
		 */
		int getValueInteger ();
};

/**
 * This is window with edit box for digits..
 */
class Rts2NWindowEditDigits:public Rts2NWindowEdit
{
	protected:
		virtual bool passKey (int key);
	public:
		Rts2NWindowEditDigits (int _x, int _y, int w, int h,
			int _ex, int _ey, int _ew, int _eh, bool border = true);
};
#endif							 /* !__RTS2_NWINDOWEDIT__ */
