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

#include "rts2value.h"

/**
 * Rectangle value. Rectangle have four points, each of Rts2Value, which
 * represents X, Y coordinates and width and height.
 *
 * @ingroup RTS2Value
 */
class Rts2ValueRectangle: public Rts2Value
{
	private:
		Rts2Value *x;
		Rts2Value *y;
		Rts2Value *w;
		Rts2Value *h;

		void createValues ();
		void checkChange ();
	public:
		Rts2ValueRectangle (std::string in_val_name, int32_t baseType);
		Rts2ValueRectangle (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags);
		virtual ~Rts2ValueRectangle (void);

		/**
		 * Get value of X coordinate of the rectangle.
		 *
		 * @return Rts2Value class with value of X coordinate.
		 */
		Rts2Value * getX ()
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
		 * @return Rts2Value class with value of Y coordinate.
		 */
		Rts2Value * getY ()
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
		 * @return Rts2Value class with rectangle width.
		 */
		Rts2Value * getWidth ()
		{
			return w;
		}

		/**
		 * Return width integer value.
		 *
		 * @return width as integer.
		 */
		int getWidthInt ()
		{
			return w->getValueInteger ();
		}

		/**
		 * Get height of the rectangle.
		 *
		 * @return Rts2Value class with rectangle height.
		 */
		Rts2Value * getHeight ()
		{
			return h;
		}

		/**
		 * Return height integer value.
		 *
		 * @return height as integer.
		 */
		int getHeightInt ()
		{
			return h->getValueInteger ();
		}

		void setInts (int in_x, int in_y, int in_w, int in_h);

		virtual int setValue (Rts2Conn *connection);

		virtual int setValueCharArr (const char *in_value);
		virtual int setValueInteger (int in_value);

		virtual const char *getValue ();

		virtual void setFromValue(Rts2Value * new_value);

		virtual bool isEqual (Rts2Value * other_value);

		virtual void resetValueChanged ();
};
#endif							 // !__RTS2_RECTANGLE__
