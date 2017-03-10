/* 
 * List of connected daemons.
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

#ifndef __RTS2_DAEMONWINDOW__
#define __RTS2_DAEMONWINDOW__

#include "nwindow.h"

#include "block.h"
#include "connection.h"
#include "client.h"
#include "devclient.h"

namespace rts2ncurses
{

/**
 * Selection window. Provides list of items, and enables user
 * select one with arrow keys.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NSelWindow:public NWindow
{
	public:
		NSelWindow (int x, int y, int w, int h, int border = 1, int sw = 300, int sh = 1000);
		virtual ~ NSelWindow (void);
		virtual keyRet injectKey (int key);
		virtual void winrefresh ();
		int getSelRow ()
		{
			if (selrow == -1)
				return (maxrow - 1);
			return selrow;
		}
		void setSelRow (int new_sel) { selrow = new_sel; }

		/**
		 * Decrease/increase selection row by a given change value.
		 */
		void changeSelRow (int change);

		virtual void winerase () { werase (scrolpad); }

		virtual WINDOW *getWriteWindow () { return scrolpad; }

		int getScrollWidth ()
		{
			return getmaxx (scrolpad);
		}
		int getScrollHeight ()
		{
			return getmaxy (scrolpad);
		}
		void setLineOffset (int new_lineOffset) { lineOffset = new_lineOffset; }
		int getPadoffY () { return padoff_y; }

	protected:
		int selrow;
		int maxrow;
		int padoff_x;
		int padoff_y;
		int lineOffset;
		WINDOW *scrolpad;
};

class NDevListWindow:public NSelWindow
{
	public:
		NDevListWindow (rts2core::Block * in_block, rts2core::connections_t *in_conns);
		virtual ~ NDevListWindow (void);
		virtual void draw ();

	private:
		rts2core::Block * block;
		rts2core::connections_t *conns;
};

class NCentraldWindow:public NSelWindow
{
	public:
		NCentraldWindow (rts2core::Client * in_client);
		virtual ~ NCentraldWindow (void);
		virtual void draw ();
	private:
		rts2core::Client * client;
		void printState (rts2core::Rts2Connection * conn);
		void drawDevice (rts2core::Rts2Connection * conn);
};

}
#endif							 /*! __RTS2_DAEMONWINDOW__ */
