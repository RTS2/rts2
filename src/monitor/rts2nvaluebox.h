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

#ifndef __RTS2_NVALUEBOX__
#define __RTS2_NVALUEBOX__

#include "../utils/rts2value.h"
#include "../utils/rts2valuerectangle.h"

#include "rts2nwindow.h"
#include "rts2nwindowedit.h"
#include "rts2daemonwindow.h"

namespace rts2ncur
{

/**
 * Holds edit box for value.
 */
class ValueBox
{
	private:
		Rts2NWindow * topWindow;
		Rts2Value * val;
	protected:
		Rts2Value * getValue ()
		{
			return val;
		}
	public:
		ValueBox (Rts2NWindow * top, Rts2Value * _val);
		virtual ~ValueBox (void);
		virtual keyRet injectKey (int key) = 0;
		virtual void draw () = 0;
		virtual void sendValue (Rts2Conn * connection) = 0;
		virtual bool setCursor () = 0;
};

/**
 * Holds edit box for boolean value.
 */
class ValueBoxBool:public ValueBox, Rts2NSelWindow
{
	public:
		ValueBoxBool (Rts2NWindow * top, Rts2ValueBool * _val, int _x, int _y);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Holds edit box for string value.
 */
class ValueBoxString:public ValueBox, Rts2NWindowEdit
{
	public:
		ValueBoxString (Rts2NWindow * top, Rts2Value * _val, int _x, int _y);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Holds edit box for integer value.
 */
class ValueBoxInteger:public ValueBox, Rts2NWindowEditIntegers
{
	public:
		ValueBoxInteger (Rts2NWindow * top, Rts2ValueInteger * _val, int _x, int _y);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Holds edit box for long integer value.
 */
class ValueBoxLongInteger:public ValueBox, Rts2NWindowEditIntegers
{
	public:
		ValueBoxLongInteger (Rts2NWindow * top, Rts2ValueLong * _val, int _x, int _y);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Holds edit box for float value.
 */
class ValueBoxFloat:public ValueBox, Rts2NWindowEditDigits
{
	public:
		ValueBoxFloat (Rts2NWindow * top, Rts2ValueFloat * _val, int _x, int _y);

		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Holds edit box for double value.
 */
class ValueBoxDouble:public ValueBox, Rts2NWindowEditDigits
{
	public:
		ValueBoxDouble (Rts2NWindow * top, Rts2ValueDouble * _val, int _x, int _y);

		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Holds edit box for selection value.
 */
class ValueBoxSelection:public ValueBox, Rts2NSelWindow
{
	public:
		ValueBoxSelection (Rts2NWindow * top, Rts2ValueSelection * _val, int _x, int _y);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};


/**
 * Provides edit box for editting rectangle (4 numbers)
 */
class ValueBoxRectangle:public ValueBox, Rts2NWindowEdit
{
	private:
		Rts2NWindowEditIntegers * edt[4];
		int edtSelected;
	public:
		ValueBoxRectangle (Rts2NWindow * top, Rts2ValueRectangle * _val, int _x, int _y);
		virtual ~ValueBoxRectangle ();
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};

/**
 * Provides edit box for editting  RA DeC
 */
class ValueBoxRaDec:public ValueBox, Rts2NWindowEdit
{
	private:
		Rts2NWindowEditDigits * edt[2];
		int edtSelected;
	public:
		ValueBoxRaDec (Rts2NWindow * top, Rts2ValueRaDec * _val, int _x, int _y);
		virtual ~ValueBoxRaDec ();
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void sendValue (Rts2Conn * connection);
		virtual bool setCursor ();
};


}

#endif // !__RTS2_NVALUEBOX__ 
