/* 
 * Windowing class build on ncurses
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


#ifndef __RTS2_NWINDOW__
#define __RTS2_NWINDOW__

#include <rts2-config.h>

#ifdef RTS2_HAVE_CURSES_H
#include <curses.h>
#elif defined(RTS2_HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#endif

#include "nlayout.h"

typedef enum { RKEY_ENTER, RKEY_ESC, RKEY_HANDLED, RKEY_NOT_HANDLED } keyRet;

namespace rts2ncurses
{

/**
 * Basic class for NCurser windows.
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NWindow:public Layout
{
	public:
		NWindow (int x, int y, int w, int h, bool border = true);
		virtual ~ NWindow (void);
		/**
		 * Handles key pressed event. Return values has following meaning:
		 * RKEY_ENTER - enter key was pressed, we should do some action and possibly exit the window
		 * RKEY_ESC - EXIT or ESC key was pressed, we should exit the window
		 * RKEY_HANDLED - key was handled by the window, futher processing should not happen
		 * RKEY_NOT_HANDLED - key was not handled, futher processing/default fall-back call is necessary
		 */
		virtual keyRet injectKey (int key);
		virtual void draw ();

		int getX ();
		int getY ();
		int getCurX ();
		int getCurY ();
		int getWindowX ();
		int getWindowY ();
		int getWidth ();
		int getHeight ();
		int getWriteWidth ();
		int getWriteHeight ();

		void setX (int x) { winmove (x, getY ()); }

		void setY (int y) { winmove (getX (), y); }

		virtual void winclear (void) { werase (window);	}

		void winmove (int x, int y);
		virtual void resize (int x, int y, int w, int h);
		
		void setWidth (int w) { resize (getX (), getY(), w, getHeight ()); }
		void grow (int max_w, int h_dif);

		virtual void winrefresh ();

		/**
		 * Gets called when window get focus.
		 */
		virtual void enter ();

		/**
		 * Gets called when window lost focus.
		 */
		virtual void leave ();

		/**
		 * Set screen cursor to current window.
		 * Return true if the window claims cursor, otherwise return false.
		 */
		virtual bool setCursor ();

		void setNormal () { wattrset (getWriteWindow (), A_NORMAL); }

		/**
		 * Set reverse attribute for full pad (e.g. invert colors in pad).
		 */
		void setReverse () { wattrset (getWriteWindow (), A_REVERSE); }

		void setUnderline () { wattrset (getWriteWindow (), A_UNDERLINE); }

		/**
		 * Returns window which is used to write text
		 */
		virtual WINDOW *getWriteWindow () { return window; }

		bool haveBox () { return _haveBox; }

		/**
		 * Indicate this window needs enter, so enter and tab keys will not be stolen for wait command.
		 */
		virtual bool hasEditBox () { return false; }

		/**
		 * Return window active state. That is true if window is active and receives keyboard inputs.
		 *
		 * @return window active state.
		 */
		bool isActive () { return active; }
	protected:
		WINDOW * window;
		void errorMove (const char *op, int y, int x, int h, int w);
	private:
		bool _haveBox;
		bool active;
};

}
#endif							 /* !__RTS2_NWINDOW__ */
