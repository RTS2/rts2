/* 
 * NCurses based monitoring
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

#ifndef __RTS2_NMONITOR__
#define __RTS2_NMONITOR__

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#include "../utils/block.h"
#include "../utils/rts2client.h"
#include "../utils/rts2command.h"

#include "../writers/rts2image.h"
#include "../writers/rts2devcliimg.h"

#ifdef HAVE_PGSQL
#include "../db/simbad/simbadtarget.h"
#endif

#include "nlayout.h"
#include "daemonwindow.h"
#include "ndevicewindow.h"
#include "nmenu.h"
#include "nmsgbox.h"
#include "nmsgwindow.h"
#include "nstatuswindow.h"
#include "ncomwin.h"

// colors used in monitor
#define CLR_DEFAULT          1
#define CLR_OK               2
#define CLR_TEXT             3
#define CLR_PRIORITY         4
#define CLR_WARNING          5
#define CLR_FAILURE          6
#define CLR_STATUS           7
#define CLR_FITS             8
#define CLR_MENU             9
#define CLR_SCRIPT_CURRENT  10

#define K_ENTER             13
#define K_ESC               27

#define CMD_BUF_LEN        100

#define MENU_OFF             1
#define MENU_STANDBY         2
#define MENU_ON              3
#define MENU_EXIT            4
#define MENU_ABOUT           5
#define MENU_MANUAL          6

#define MENU_DEBUG_BASIC    11
#define MENU_DEBUG_LIMITED  12
#define MENU_DEBUG_FULL     13

#define MENU_SORT_ALPHA     21
#define MENU_SORT_RTS2      22

enum messageAction { SWITCH_OFF, SWITCH_STANDBY, SWITCH_ON, NONE };

using namespace rts2ncurses;

/**
 * This class hold "root" window of display,
 * takes care about displaying it's connection etc..
 *
 * @author Petr Kubánek <petr@kubanek.net>
 */
class NMonitor:public Rts2Client
{
	public:
		NMonitor (int argc, char **argv);
		virtual ~ NMonitor (void);

		virtual int init ();
		virtual int idle ();

		virtual Rts2ConnClient *createClientConnection (int _centrald_num, char *_deviceName);
		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);

		virtual int deleteConnection (Rts2Conn * conn);

		virtual void message (Rts2Message & msg);

		void resize ();

		void processKey (int key);

		void commandReturn (rts2core::Rts2Command * cmd, int cmd_status);

	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);

		virtual void addSelectSocks ();
		virtual void selectSuccess ();

		virtual Rts2ConnCentraldClient *createCentralConn ();

	private:
		WINDOW * cursesWin;
		Layout *masterLayout;
		LayoutBlock *daemonLayout;
		NDevListWindow *deviceList;
		NWindow *daemonWindow;
		NMsgWindow *msgwindow;
		NComWin *comWindow;
		NMenu *menu;

		NMsgBox *msgBox;

		NStatusWindow *statusWindow;

		rts2core::Rts2Command *oldCommand;

		std::list < NWindow * >windowStack;

		int cmd_col;
		char cmd_buf[CMD_BUF_LEN];

		int repaint ();

		messageAction msgAction;

		bool colorsOff;

		void showHelp ();

		/**
		 * Display message box with OK button.
		 */
		void messageBox (const char *text);
		void messageBox (const char *query, messageAction action);
		void messageBoxEnd ();
		void menuPerform (int code);
		void leaveMenu ();

		void changeActive (NWindow * new_active);
		void changeListConnection ();

		int old_lines;
		int old_cols;

		NWindow *getActiveWindow () { return *(--windowStack.end ()); }
		void setXtermTitle (const std::string &title) { std::cout << "\033]2;" << title << "\007"; }

		void sendCommand ();

#ifdef HAVE_PGSQL
		rts2db::SimbadTarget *tarArg;
#endif

		enum { ORDER_RTS2, ORDER_ALPHA } connOrder;

		connections_t orderedConn;

		void refreshConnections ();
		/**
		 * Return connection at given number.
		 *
		 * @param i Number of connection which will be returned.
		 *
		 * @return NULL if connection with given number does not exists, or @see Rts2Conn reference if it does.
		 */
		Rts2Conn *connectionAt (unsigned int i);
};

/**
 * Make sure that update of connection state is notified in monitor.
 */
class NMonConn:public Rts2ConnClient
{
	public:
		NMonConn (NMonitor * _master, int _centrald_num, char *_name):Rts2ConnClient (_master, _centrald_num, _name)
		{
			master = _master;
		}

		virtual void commandReturn (rts2core::Rts2Command * cmd, int in_status)
		{
			master->commandReturn (cmd, in_status);
			return Rts2ConnClient::commandReturn (cmd, in_status);
		}
	private:
		NMonitor * master;
};

/**
 * Make sure that command end is properly reflected
 */
class NMonCentralConn:public Rts2ConnCentraldClient
{
	public:
		NMonCentralConn (NMonitor * in_master,
			const char *in_login,
			const char *in_password,
			const char *in_master_host,
			const char
			*in_master_port):Rts2ConnCentraldClient (in_master,
			in_login,
			in_password,
			in_master_host,
			in_master_port)
		{
			master = in_master;
		}

		virtual void commandReturn (rts2core::Rts2Command * cmd, int in_status)
		{
			master->commandReturn (cmd, in_status);
			Rts2ConnCentraldClient::commandReturn (cmd, in_status);
		}
	private:
		NMonitor * master;
};
#endif							 /* !__RTS2_NMONITOR__ */
