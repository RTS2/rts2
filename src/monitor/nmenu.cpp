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

#include "nmonitor.h"
#include "nmenu.h"

using namespace rts2ncurses;

void NAction::draw (WINDOW * window, int y)
{
	mvwprintw (window, y, 0, "%s", text);
}

void NActionBool::draw (WINDOW *window, int y)
{
	if (isActive)
	{
		mvwaddch (window, y, 0, ACS_DIAMOND);
		wprintw (window, " %s", text);
	}
	else
	{
		mvwaddch (window, y, 0, ACS_BULLET);
		wprintw (window, " %s", text_unactive);
	}
}

NSubmenu::NSubmenu (const char *in_text): NSelWindow (0, 0, 2, 2)
{
	text = in_text;
	grow (strlen (text) + 2, 0);
	setLineOffset (0);
}

NSubmenu::~NSubmenu (void)
{
}

void NSubmenu::draw (WINDOW * master_window)
{
	mvwprintw (master_window, 0, getX () + 1, "%s", text);
}

void NSubmenu::drawSubmenu ()
{
	NSelWindow::draw ();
	werase (scrolpad);
	maxrow = 0;
	for (std::vector < NAction * >::iterator iter = actions.begin (); iter != actions.end (); iter++, maxrow++)
	{
		NAction *action = *iter;
		action->draw (scrolpad, maxrow);
	}
}

NMenu::NMenu ():NWindow (0, 0, COLS, 1, 0)
{
	selSubmenuIter = submenus.begin ();
	selSubmenu = NULL;
	top_x = 0;
}

NMenu::~NMenu (void)
{
	for (selSubmenuIter = submenus.begin (); selSubmenuIter != submenus.end (); selSubmenuIter++)
	{
		selSubmenu = *selSubmenuIter;
		delete selSubmenu;
	}
	submenus.clear ();
}

keyRet NMenu::injectKey (int key)
{

	switch (key)
	{
		case KEY_UP:
		case KEY_DOWN:
		case KEY_HOME:
		case KEY_END:
			if (selSubmenu)
			{
				selSubmenu->injectKey (key);
			}
			break;
		case KEY_RIGHT:
			selSubmenuIter++;
			if (selSubmenuIter == submenus.end ())
				selSubmenuIter = submenus.begin ();
			selSubmenu = *selSubmenuIter;
			break;
		case KEY_LEFT:
			if (selSubmenuIter == submenus.begin ())
				selSubmenuIter = submenus.end ();
			selSubmenuIter--;
			selSubmenu = *selSubmenuIter;
			break;
		case '\n':
		case KEY_ENTER:
		case K_ENTER:
			return RKEY_ENTER;
		case KEY_EXIT:
		case K_ESC:
			selSubmenu = NULL;
			return RKEY_ESC;
		default:
			return NWindow::injectKey (key);
	}
	return RKEY_HANDLED;
}

void NMenu::draw ()
{
	NWindow::draw ();
	werase (window);
	wcolor_set (window, CLR_MENU, NULL);
	mvwhline (window, 0, 0, ' ', getWidth ());
	
	for (std::vector < NSubmenu * >::iterator iter = submenus.begin (); iter != submenus.end (); iter++)
	{
		NSubmenu *submenu = *iter;
		if (submenu == selSubmenu)
		{
			submenu->drawSubmenu ();
			submenu->winrefresh ();
		}
		submenu->draw (window);
	}

	// end with hostname..
	char buf[HOST_NAME_MAX];
	gethostname (buf, HOST_NAME_MAX);
	wcolor_set (window, CLR_TEXT, NULL);
	mvwprintw (window, 0, getWidth () - strlen (buf) - 1, "%s", buf);

	winrefresh ();
}

void NMenu::enter ()
{
	selSubmenu = *selSubmenuIter;
}

void NMenu::leave ()
{
	selSubmenu = NULL;
}

void NMenu::addSubmenu (NSubmenu * in_submenu)
{
	submenus.push_back (in_submenu);
	selSubmenuIter = submenus.begin ();
	in_submenu->winmove (top_x, 0);
	top_x += in_submenu->getWidth () + 2;
}
