/* 
 * Class for rectangle.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_RECTANGLE__
#define __RTS2_RECTANGLE__

#include "value.h"

namespace rts2core
{

/**
 * Rectangle value. Rectangle have four points, each of Value, which
 * represents X, Y coordinates and width and height.
 *
 * @ingroup RTS2Value
 */
class ValueRectangle: public Value
{
	private:
		Value *x;
		Value *y;
		Value *w;
		Value *h;

		void createValues ();
		void checkChange ();
	public:
		ValueRectangle (std::string in_val_name, int32_t baseType);
		ValueRectangle (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags);
		virtual ~ValueRectangle (void);

		/**
		 * Get value of X coordinate of the rectangle.
		 *
		 * @return Value class with value of X coordinate.
		 */
		Value * getX ()
		{
			return x;
		}

		/**
		 * Return X integer value.
		 *
		 * @return X as integer.
		 */
		int getXInt ()
		{
			return x->getValueInteger ();
		}

		/**
		 * Get value of Y coordinate of the rectangle.
		 *
		 * @return Value class with value of Y coordinate.
		 */
		Value * getY ()
		{
			return y;
		}

		/**
		 * Return Y integer value.
		 *
		 * @return Y as integer.
		 */
		int getYInt ()
		{
			return y->getValueInteger ();
		}

		/**
		 * Get width of the rectangle.
		 *
		 * @return Value class with rectangle width.
		 */
		Value * getWidth () { return w; }

		/**
		 * Return width integer value.
		 *
		 * @return width as integer.
		 */
		int getWidthInt () { return w->getValueInteger (); }

		void setWidth (int _w) { w->setValueInteger (_w); }

		/**
		 * Get height of the rectangle.
		 *
		 * @return Value class with rectangle height.
		 */
		Value * getHeight () { return h; }

		/**
		 * Return height integer value.
		 *
		 * @return height as integer.
		 */
		int getHeightInt () { return h->getValueInteger (); }

		void setHeight (int _h) { h->setValueInteger (_h); }

		void setInts (int in_x, int in_y, int in_w, int in_h);

		virtual int setValue (Connection *connection);

		virtual int setValueCharArr (const char *in_value);
		virtual int setValueInteger (int in_value);

		virtual const char *getValue ();

		virtual void setFromValue(Value * new_value);

		virtual bool isEqual (Value * other_value);

		virtual const char *getDisplayValue ();

		virtual void resetValueChanged ();
};

}
#endif							 // !__RTS2_RECTANGLE__
