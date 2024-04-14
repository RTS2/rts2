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

#include "nlayout.h"

using namespace rts2ncurses;

Utf8Chars Layout::utf8Chars;

void LayoutBlock::resize (int x, int y, int w, int h)
{
	int tmp_i;
	if (vertical)
	{
		tmp_i = (int) ((double) w * ratio / 100.0);
		layoutA->resize (x, y, tmp_i, h);
		layoutB->resize (x + tmp_i, y, w - tmp_i, h);
	}
	else
	{
		tmp_i = (int) ((double) h * ratio / 100.0);
		layoutA->resize (x, y, w, tmp_i);
		layoutB->resize (x, y + tmp_i, w, h - tmp_i);
	}
}

void LayoutBlockFixedA::resize (int x, int y, int w, int h)
{
	int tmp_i = getRatio ();
	if (isVertical ())
	{
		if (w < tmp_i)
			tmp_i = w;
		getLayoutA ()->resize (x, y, tmp_i, h);
		getLayoutB ()->resize (x + tmp_i, y, w - tmp_i, h);
	}
	else
	{
		if (h < tmp_i)
			tmp_i = h;
		getLayoutA ()->resize (x, y, w, tmp_i);
		getLayoutB ()->resize (x, y + tmp_i, w, h - tmp_i);
	}
}

void LayoutBlockFixedB::resize (int x, int y, int w, int h)
{
	int tmp_i = getRatio ();
	if (isVertical ())
	{
		if (w < tmp_i)
			tmp_i = w;
		getLayoutA ()->resize (x, y, w - tmp_i, h);
		getLayoutB ()->resize (x + w - tmp_i, y, tmp_i, h);
	}
	else
	{
		if (h < tmp_i)
			tmp_i = h;
		getLayoutA ()->resize (x, y, w, h - tmp_i);
		getLayoutB ()->resize (x, y + h - tmp_i, w, tmp_i);
	}
}
