/* 
 * NCurses layout engine
 * Copyright (C) 2003-2007,2010 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_NLAYOUT__
#define __RTS2_NLAYOUT__
#include "utf8chars.h"

namespace rts2ncurses
{

/**
 * Holds informations which are used for NCURSER windows layout.
 */
class Layout
{
	protected:
		static Utf8Chars utf8Chars;
	public:
		Layout ()
		{
		}
		virtual ~ Layout (void)
		{
		}
		virtual void resize (int x, int y, int w, int h) = 0;
};

class LayoutBlock:public Layout
{
	private:
		Layout * layoutA;
		Layout *layoutB;
		// if true, represents vertical windows
		bool vertical;
		// windows ratio, in percentages for first window
		int ratio;
	public:
		LayoutBlock (Layout * in_laA, Layout * in_laB,
			bool in_vertical, int in_ratio)
		{
			layoutA = in_laA;
			layoutB = in_laB;
			vertical = in_vertical;
			ratio = in_ratio;
		}
		virtual ~ LayoutBlock (void)
		{
			delete layoutA;
			delete layoutB;
		}
		void setLayoutA (Layout * in_laA)
		{
			layoutA = in_laA;
		}
		void setLayoutB (Layout * in_laB)
		{
			layoutB = in_laB;
		}
		Layout *getLayoutA ()
		{
			return layoutA;
		}
		Layout *getLayoutB ()
		{
			return layoutB;
		}
		virtual void resize (int x, int y, int w, int h);
		int getRatio ()
		{
			return ratio;
		}
		bool isVertical ()
		{
			return vertical;
		}
};

class LayoutBlockFixedA:public LayoutBlock
{
	public:
		LayoutBlockFixedA (Layout * in_laA, Layout * in_laB,
			bool in_vertical,
			int sizeA):LayoutBlock (in_laA, in_laB,
			in_vertical, sizeA)
		{
		}
		virtual ~ LayoutBlockFixedA (void)
		{
		}
		virtual void resize (int x, int y, int w, int h);
};

class LayoutBlockFixedB:public LayoutBlock
{
	public:
		LayoutBlockFixedB (Layout * in_laA, Layout * in_laB,
			bool in_vertical,
			int sizeB):LayoutBlock (in_laA, in_laB,
			in_vertical, sizeB)
		{
		}
		virtual ~ LayoutBlockFixedB (void)
		{
		}
		virtual void resize (int x, int y, int w, int h);
};

}
#endif							 /*! __RTS2_NLAYOUT__ */
