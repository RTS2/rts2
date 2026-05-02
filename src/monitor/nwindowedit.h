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

#include "nwindow.h"

namespace rts2ncurses
{

/**
 * Window with single edit box to edit values.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NWindowEdit:public NWindow
{
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
		NWindowEdit (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border = true);
		virtual ~ NWindowEdit (void);

		virtual keyRet injectKey (int key);
		virtual void winrefresh ();

		/**
		 * Set window size, shift it to left if its too big to fit on screen.
		 *
		 * @param w new window width (in characters)
		 * @param h new window height (in characters)
		 */
		void setSize (int w, int h);

		virtual bool setCursor ();

		virtual WINDOW *getWriteWindow () { return comwin; }

		int getLength ();

		void setValue (std::string _val) { wprintw (getWriteWindow (), _val.c_str ()); }

		std::string getValueString ();
	protected:
		/**
		 * Returns true if key should be wadded to comwin.
		 * Returns false if the key is not supported (e.g. numeric
		 * edit box don't want to get alnum). Default is to return isalnum (key).
		 */
		virtual bool passKey (int key);
	private:
		WINDOW * comwin;
		int ex, ey, ew, eh;
		// current cursor position
		int x, y;
		// actual visible width and height
		int width, height;
};

/**
 * Window with edit box for integers.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NWindowEditIntegers:public NWindowEdit
{
	public:
		NWindowEditIntegers (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border = true, bool _hex = false);

		/**
		 * Sets edit window value.
		 */
		void setValueInteger (int _val) { wprintw (getWriteWindow (), "%i", _val); }

		/**
		 * Returns integer value of the field. Returns 0 if field is empty.
		 */
		int getValueInteger ();
	protected:
		virtual bool passKey (int key);
	private:
		bool hex;
};

/**
 * Window with edit box for digits..
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NWindowEditDigits:public NWindowEdit
{
	public:
		NWindowEditDigits (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border = true);

		/**
		 * Sets edit window value.
		 */
		virtual void setValueDouble (double _val);

		/**
		 * Returns integer value of the field. Returns 0 if field is empty.
		 */
		virtual double getValueDouble ();
	protected:
		virtual bool passKey (int key);
};

/**
 * Window with edit box for degrees (d,' and ")
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NWindowEditDegrees:public NWindowEditDigits
{
	public:
		NWindowEditDegrees (int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border = true);

		/**
		 * Sets edit window value.
		 */
		virtual void setValueDouble (double _val);

		/**
		 * Returns degrees value of the field. Returns 0 if field is empty.
		 */
		virtual double getValueDouble ();
	protected:
		virtual bool passKey (int key);
};

/**
 * Window with edit box for boolean.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NWindowEditBool:public NWindowEdit
{
	public:
		NWindowEditBool (int _type, int _x, int _y, int w, int h, int _ex, int _ey, int _ew, int _eh, bool border = true);

		virtual void draw () { NWindowEdit::draw (); setValueBool (getValueBool ()); }

		/**
		 * Sets edit window value.
		 */
		void setValueBool (bool _val);

		/**
		 * Returns boolean value of the field.
		 */
		bool getValueBool ();
	protected:
		virtual bool passKey (int key);
	private:
		int dt;
};

}
#endif							 /* !__RTS2_NWINDOWEDIT__ */
