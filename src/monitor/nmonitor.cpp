/* 
 * NCurses based monitoring
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

#include <libnova/libnova.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <list>

#include <iostream>
#include <fstream>

#include "nmonitor.h"

#ifdef HAVE_XCURSES
char *XCursesProgramName = "rts2-mon";
#endif

void
Rts2NMonitor::sendCommand ()
{
	int curX = comWindow->getCurX ();
	char command[curX + 1];
	char *cmd_top = command;
	Rts2Conn *conn = NULL;
	comWindow->getWinString (command, curX);
	command[curX] = '\0';
	// try to find ., which show DEVICE.command notation..
	while (*cmd_top && !isspace (*cmd_top))
	{
		if (*cmd_top == '.')
		{
			*cmd_top = '\0';
			conn = getConnection (command);
			*cmd_top = '.';
			cmd_top++;
			break;
		}
		cmd_top++;
	}
	if (conn == NULL)
	{
		conn = connectionAt (deviceList->getSelRow ());
		cmd_top = command;
	}
	if (*cmd_top)
	{
		oldCommand = new Rts2Command (this, cmd_top);
		conn->queCommand (oldCommand);
		comWindow->wclear ();
		comWindow->printCommand (command);
		wmove (comWindow->getWriteWindow (), 0, 0);
	}
}


int
Rts2NMonitor::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'c':
			colorsOff = true;
			break;
		default:
			return Rts2Client::processOption (in_opt);
	}
	return 0;
}


#ifdef HAVE_PGSQL_SOAP
int
Rts2NMonitor::processArgs (const char *arg)
{
	tarArg = new Rts2SimbadTarget (arg);
	int ret = tarArg->load ();
	if (ret)
	{
		std::cerr << "Cannot resolve target " << arg << std::endl;
		return -1;
	}

	std::cout << "Type Y and press enter.." << std::endl;

	char c;
	std::cin >> c;

	return 0;
}
#endif							 /* HAVE_PGSQL_SOAP */

void
Rts2NMonitor::addSelectSocks ()
{
	// add stdin for ncurses input
	FD_SET (1, &read_set);
	Rts2Client::addSelectSocks ();
}


Rts2ConnCentraldClient *
Rts2NMonitor::createCentralConn ()
{
	return new Rts2NMonCentralConn (this, getCentralLogin (),
		getCentralPassword (), getCentralHost (),
		getCentralPort ());
}


void
Rts2NMonitor::selectSuccess ()
{
	Rts2Client::selectSuccess ();
	while (1)
	{
		int input = getch ();
		if (input == ERR)
			break;
		processKey (input);
	}
}


void
Rts2NMonitor::messageBox (const char *query, messageAction action)
{
	const static char *buts[] = { "Yes", "No" };
	if (msgBox)
		return;
	msgAction = action;
	msgBox = new Rts2NMsgBox (query, buts, 2);
	msgBox->draw ();
	windowStack.push_back (msgBox);
}


void
Rts2NMonitor::messageBoxEnd ()
{
	if (msgBox->exitState == 0)
	{
		const char *cmd = "off";
		switch (msgAction)
		{
			case SWITCH_OFF:
				cmd = "off";
				break;
			case SWITCH_STANDBY:
				cmd = "standby";
				break;
			case SWITCH_ON:
				cmd = "on";
				break;
		}
		connections_t::iterator iter;
		for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		{
			(*iter)->queCommand (new Rts2Command (this, cmd));
		}
	}
	delete msgBox;
	msgBox = NULL;
	windowStack.pop_back ();
}


void
Rts2NMonitor::menuPerform (int code)
{
	switch (code)
	{
		case MENU_OFF:
			messageBox ("Are you sure to switch off?", SWITCH_OFF);
			break;
		case MENU_STANDBY:
			messageBox ("Are you sure to switch to standby?", SWITCH_STANDBY);
			break;
		case MENU_ON:
			messageBox ("Are you sure to switch to on?", SWITCH_ON);
			break;
		case MENU_ABOUT:

			break;

		case MENU_DEBUG_BASIC:
			msgwindow->setMsgMask (0x03);
			break;
		case MENU_DEBUG_LIMITED:
			msgwindow->setMsgMask (0x07);
			break;
		case MENU_DEBUG_FULL:
			msgwindow->setMsgMask (MESSAGE_MASK_ALL);
			break;
		case MENU_EXIT:
			endRunLoop ();
			break;
	}
}


void
Rts2NMonitor::leaveMenu ()
{
	menu->leave ();
	windowStack.pop_back ();
}


void
Rts2NMonitor::changeActive (Rts2NWindow * new_active)
{
	Rts2NWindow *activeWindow = *(--windowStack.end ());
	windowStack.pop_back ();
	activeWindow->leave ();
	windowStack.push_back (new_active);
	new_active->enter ();
}


void
Rts2NMonitor::changeListConnection ()
{
	Rts2Conn *conn = connectionAt (deviceList->getSelRow ());
	if (conn)
	{
		delete daemonWindow;
		daemonWindow = NULL;
		// if connection is among centrald connections..
		connections_t::iterator iter;
		for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
			if (conn == (*iter))
				daemonWindow = new Rts2NDeviceCentralWindow (conn);

		if (daemonWindow == NULL)
			daemonWindow = new Rts2NDeviceWindow (conn);
	}
	else
	{
		delete daemonWindow;
		daemonWindow = new Rts2NCentraldWindow (this);
	}
	daemonLayout->setLayoutA (daemonWindow);
	resize ();
}


Rts2NMonitor::Rts2NMonitor (int in_argc, char **in_argv):
Rts2Client (in_argc, in_argv)
{
	masterLayout = NULL;
	daemonLayout = NULL;
	statusWindow = NULL;
	deviceList = NULL;
	daemonWindow = NULL;
	comWindow = NULL;
	menu = NULL;
	msgwindow = NULL;
	msgBox = NULL;
	cmd_col = 0;

	oldCommand = NULL;

	colorsOff = false;

	old_lines = 0;
	old_cols = 0;

	#ifdef HAVE_PGSQL_SOAP
	tarArg = NULL;
	#endif						 /* HAVE_PGSQL_SOAP */

	addOption ('c', NULL, 0, "don't use colors");
}


Rts2NMonitor::~Rts2NMonitor (void)
{
	erase ();
	refresh ();

	nocbreak ();
	echo ();
	endwin ();

	delete msgBox;
	delete statusWindow;

	delete masterLayout;

	#ifdef HAVE_PGSQL_SOAP
	delete tarArg;
	#endif						 /* HAVE_PGSQL_SOAP */
}


int
Rts2NMonitor::repaint ()
{
	curs_set (0);
	if (LINES != old_lines || COLS != old_cols)
		resize ();
	deviceList->draw ();
	if (daemonWindow == NULL)
	{
		changeListConnection ();
	}
	daemonWindow->draw ();
	msgwindow->draw ();
	statusWindow->draw ();
	comWindow->draw ();
	menu->draw ();
	if (msgBox)
		msgBox->draw ();

	Rts2NWindow *activeWindow = getActiveWindow ();
	if (!activeWindow->setCursor ())
		comWindow->setCursor ();
	curs_set (1);
	doupdate ();
	return 0;
}


int
Rts2NMonitor::init ()
{
	int ret;
	ret = Rts2Client::init ();
	if (ret)
		return ret;

	// init ncurses
	cursesWin = initscr ();
	if (!cursesWin)
	{
		std::cerr << "ncurses not supported (initscr call failed)!" << std::
			endl;
		return -1;
	}

	cbreak ();
	noecho ();
	nonl ();
	intrflush (stdscr, FALSE);
	keypad (stdscr, TRUE);
	timeout (0);

	ESCDELAY = 0;

	// create & init menu
	menu = new Rts2NMenu ();
	Rts2NSubmenu *sub = new Rts2NSubmenu ("System");
	sub->createAction ("Off", MENU_OFF);
	sub->createAction ("Standby", MENU_STANDBY);
	sub->createAction ("On", MENU_ON);
	sub->createAction ("Exit", MENU_EXIT);
	menu->addSubmenu (sub);

	sub = new Rts2NSubmenu ("Debug");
	sub->createAction ("Basic", MENU_DEBUG_BASIC);
	sub->createAction ("Limited", MENU_DEBUG_LIMITED);
	sub->createAction ("Full", MENU_DEBUG_FULL);
	menu->addSubmenu (sub);

	sub = new Rts2NSubmenu ("Help");
	sub->createAction ("About", MENU_ABOUT);
	menu->addSubmenu (sub);

	// start color mode
	start_color ();
	use_default_colors ();

	if (has_colors () && !colorsOff)
	{
		init_pair (CLR_DEFAULT, -1, -1);
		init_pair (CLR_OK, COLOR_GREEN, -1);
		init_pair (CLR_TEXT, COLOR_BLUE, -1);
		init_pair (CLR_PRIORITY, COLOR_CYAN, -1);
		init_pair (CLR_WARNING, COLOR_YELLOW, -1);
		init_pair (CLR_FAILURE, COLOR_RED, -1);
		init_pair (CLR_STATUS, COLOR_RED, COLOR_CYAN);
		init_pair (CLR_FITS, COLOR_BLUE, -1);
		init_pair (CLR_MENU, COLOR_RED, COLOR_CYAN);
		init_pair (CLR_SCRIPT_CURRENT, COLOR_RED, COLOR_CYAN);
	}

	// init windows
	deviceList = new Rts2NDevListWindow (this);
	comWindow = new Rts2NComWin ();
	msgwindow = new Rts2NMsgWindow ();
	windowStack.push_back (deviceList);
	deviceList->enter ();
	statusWindow = new Rts2NStatusWindow (comWindow, this);
	daemonWindow = new Rts2NDeviceCentralWindow (*(getCentraldConns ()->begin ()));

	// init layout
	daemonLayout =
		new Rts2NLayoutBlockFixedB (daemonWindow, comWindow, false, 5);
	masterLayout = new Rts2NLayoutBlock (deviceList, daemonLayout, true, 10);
	masterLayout = new Rts2NLayoutBlock (masterLayout, msgwindow, false, 75);

	connections_t::iterator iter;
	for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
		(*iter)->queCommand (new Rts2Command (this, "info"));

	setMessageMask (MESSAGE_MASK_ALL);

	resize ();

	return repaint ();
}


int
Rts2NMonitor::idle ()
{
	int ret = Rts2Client::idle ();
	repaint ();
	setTimeout (USEC_SEC);
	return ret;
}


Rts2ConnClient *
Rts2NMonitor::createClientConnection (int _centrald_num, char *_deviceName)
{
	return new Rts2NMonConn (this, _centrald_num, _deviceName);
}


Rts2DevClient *
Rts2NMonitor::createOtherType (Rts2Conn * conn, int other_device_type)
{
	Rts2DevClient *retC = Rts2Client::createOtherType (conn, other_device_type);
	#ifdef HAVE_PGSQL_SOAP
	if (other_device_type == DEVICE_TYPE_MOUNT && tarArg)
	{
		struct ln_equ_posn tarPos;
		tarArg->getPosition (&tarPos);
		conn->
			queCommand (new
			Rts2CommandMove (this, (Rts2DevClientTelescope *) retC,
			tarPos.ra, tarPos.dec));
	}
	#endif						 /* HAVE_PGSQL_SOAP */
	return retC;
}


int
Rts2NMonitor::deleteConnection (Rts2Conn * conn)
{
	if (conn == connectionAt (deviceList->getSelRow ()))
	{
		// that will trigger daemonWindow reregistration before repaint
		daemonWindow = NULL;
	}
	return Rts2Client::deleteConnection (conn);
}


void
Rts2NMonitor::message (Rts2Message & msg)
{
	*msgwindow << msg;
}


void
Rts2NMonitor::resize ()
{
	menu->resize (0, 0, COLS, 1);
	statusWindow->resize (0, LINES - 1, COLS, 1);
	masterLayout->resize (0, 1, COLS, LINES - 2);

	old_lines = LINES;
	old_cols = COLS;
}


void
Rts2NMonitor::processKey (int key)
{
	Rts2NWindow *activeWindow = getActiveWindow ();
	keyRet ret = RKEY_HANDLED;
	switch (key)
	{
		case '\t':
		case KEY_STAB:
			if (activeWindow == deviceList)
			{
				changeActive (daemonWindow);
				activeWindow = NULL;
			}
			else if (activeWindow == daemonWindow)
			{
				if (daemonWindow->hasEditBox ())
				{
					ret = daemonWindow->injectKey (key);
				}
				else
				{
					changeActive (msgwindow);
					activeWindow = NULL;
				}
			}
			else if (activeWindow == msgwindow)
			{
				changeActive (deviceList);
				activeWindow = NULL;
			}
			break;
		case KEY_BTAB:
			if (activeWindow == deviceList)
			{
				changeActive (msgwindow);
				activeWindow = NULL;
			}
			else if (activeWindow == daemonWindow)
			{
				if (daemonWindow->hasEditBox ())
				{
					ret = daemonWindow->injectKey (key);
				}
				else
				{
					changeActive (deviceList);
					activeWindow = NULL;
				}
			}
			else if (activeWindow == msgwindow)
			{
				changeActive (daemonWindow);
				activeWindow = NULL;
			}
			break;
		case KEY_F (2):
			messageBox ("Are you sure to switch off?", SWITCH_OFF);
			break;
		case KEY_F (3):
			messageBox ("Are you sure to switch to standby?", SWITCH_STANDBY);
			break;
		case KEY_F (4):
			messageBox ("Are you sure to switch to on?", SWITCH_ON);
			break;
		case KEY_F (5):
			queAll ("info");
			break;
		case KEY_F (8):
			doupdate ();
			break;
		case KEY_F (9):
			windowStack.push_back (menu);
			menu->enter ();
			break;
		case KEY_F (10):
			endRunLoop ();
			break;
		case KEY_RESIZE:
			resize ();
			break;
			// default for active window
		case KEY_ENTER:
		case K_ENTER:
			// preproccesed enter in case device window is selected..
			if (activeWindow == daemonWindow && comWindow->getCurX () != 0
				&& !daemonWindow->hasEditBox ())
			{
				ret = comWindow->injectKey (key);
				sendCommand ();
				break;
			}
		default:
			ret = activeWindow->injectKey (key);
			if (ret == RKEY_NOT_HANDLED)
			{
				ret = comWindow->injectKey (key);
				if (key == KEY_ENTER || key == K_ENTER)
					sendCommand ();
			}
	}
	// draw device values
	if (activeWindow == deviceList)
	{
		changeListConnection ();
	}
	// handle msg box
	if (activeWindow == msgBox && ret != RKEY_HANDLED)
	{
		messageBoxEnd ();
	}
	else if (activeWindow == menu && ret != RKEY_HANDLED)
	{
		Rts2NAction *action;
		action = menu->getSelAction ();
		if (action)
		{
			leaveMenu ();
			menuPerform (action->getCode ());
		}
		else
		{
			leaveMenu ();
		}
	}
}


void
Rts2NMonitor::commandReturn (Rts2Command * cmd, int cmd_status)
{
	if (oldCommand == cmd)
		comWindow->commandReturn (cmd, cmd_status);
}


int
main (int argc, char **argv)
{
	Rts2NMonitor monitor = Rts2NMonitor (argc, argv);
	return monitor.run ();
}
