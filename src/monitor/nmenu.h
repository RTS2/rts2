/*
 * Ncurses menus.
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

#ifndef __RTS2_NMENU__
#define __RTS2_NMENU__

#include "daemonwindow.h"

#include <vector>

namespace rts2ncurses
{

/**
 * One action item for NSubmenu.
 */
class NAction
{
	public:
		NAction (const char *in_text, int in_code, Utf8Chars& chars) : utf8Chars (chars)
		{
			text = in_text;
			code = in_code;
		}
		virtual ~ NAction (void)
		{
		}
		virtual void draw (WINDOW * window, int y);

		int getCode ()
		{
			return code;
		}

		/**
		 * Set menu text.
		 */
		void setText (const char *_text)
		{
			text = _text;
		}

	protected:
		const char *text;
		Utf8Chars& utf8Chars;

	private:
		int code;
};

/**
 * Boolean menu - can be checked or unchecked.
 */
class NActionBool:public NAction
{
	public:
		NActionBool (const char *_text_active, const char *_text_unactive, int _code, Utf8Chars chars):NAction (_text_active, _code, chars) { isActive = true; text_unactive = _text_unactive; }

		virtual void draw (WINDOW *window, int y);
		void setActive (bool _active) { isActive = _active; }

	private:
		bool isActive;
		const char *text_unactive;
};

/**
 * Submenu class. Holds list of actions which will be displayed in
 * submenu.
 */
class NSubmenu:public NSelWindow
{
	public:
		NSubmenu (const char *in_text);
		virtual ~ NSubmenu (void);
		void addAction (NAction * in_action)
		{
			actions.push_back (in_action);
		}

		/**
		 * Create new menu action.
		 *
		 * @return created NAction object.
		 */
		NAction * createAction (const char *in_text, int in_code)
		{
			NAction *ret = new NAction (in_text, in_code, utf8Chars);
			addAction (ret);
			grow (strlen (in_text) + 4, 1);
			return ret;
		}

		NActionBool * createActionBool (const char *in_text, const char *_unactive, int in_code)
		{
			NActionBool *ret = new NActionBool (in_text, _unactive, in_code, utf8Chars);
			addAction (ret);
			grow (strlen (in_text) + 5, 1);
			return ret;
		}

		void draw (WINDOW * master_window);
		void drawSubmenu ();
		NAction *getSelAction ()
		{
			return actions[getSelRow ()];
		}

	private:
		std::vector < NAction * >actions;
		const char *text;
};

/**
 * Application menu. It holds submenus, which organize different
 * actions.
 */
class NMenu:public NWindow
{
	public:
		NMenu ();
		virtual ~ NMenu (void);
		virtual keyRet injectKey (int key);
		virtual void draw ();
		virtual void enter ();
		virtual void leave ();

		void addSubmenu (NSubmenu * in_submenu);
		NAction *getSelAction ()
		{
			if (selSubmenu)
				return selSubmenu->getSelAction ();
			return NULL;
		}
	private:
		std::vector < NSubmenu * >submenus;
		std::vector < NSubmenu * >::iterator selSubmenuIter;
		NSubmenu *selSubmenu;
		int top_x;
};

}
#endif							 /* !__RTS2_NMENU__ */
