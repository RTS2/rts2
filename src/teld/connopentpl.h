/* 
 * OpenTPL interface.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CONNOPENTPL__
#define __RTS2_CONNOPENTPL__

#include "../utils/rts2connnosend.h"

#include <string>
#include <ostream>

#define TPL_OK              0
#define TPL_ERR             1

namespace rts2core
{


/**
 * Error thrown on any OpenTPL errors.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OpenTplError
{
	private:
		const char *desc;
	public:
		OpenTplError (const char *_desc)
		{
			desc = _desc;
		}
		
		const char *getDesc ()
		{
			return desc;
		}

		friend std::ostream & operator << (std::ostream & os , OpenTplError & err);

};

/**
 * Class for communicating with OpenTPL devices.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class OpenTpl: public Rts2ConnNoSend
{
	private:
		std::string hostname;
		int port;
	
		char valReply[500];
		
		int tpl_command_no;

		// used commands IDs - ID which waits for command complete
		std::vector <int> used_command_ids;
		
		int sendCommand (const char *cmd, const char *p1, bool wait = true);

		// loop till there isn't reply on connection from the command
		int waitReply ();
		
	public:
		OpenTpl (Rts2Block *_master, std::string _host_name, int _port);
		virtual ~OpenTpl ();

		virtual int idle ();
		virtual int init ();
		
		virtual int receive (fd_set *set);

		int tpl_set (const char *_name, double value, int *tpl_status);
		int tpl_get (const char *_name, double &value, int *tpl_status);

		int tpl_set (const char *_name, int value, int *tpl_status, bool wait=true);
		int tpl_get (const char *_name, int &value, int *tpl_status);

		/**
		 * Sets telescope value, only waits for command OK - do not
		 * wait for completion.
		 */
		int tpl_setww (const char *_name, int value, int *tpl_status);

		int tpl_set (const char *_name, std::string value, int *tpl_status);
		int tpl_get (const char *_name, std::string &value, int *tpl_status);

		/**
		 * Handle event from connection.
		 *
		 */
		void handleEvent (const char *buffer);

		/**
		 * Handle command return from connection.
		 *
		 * @param buffer Buffer with command.
		 * @param isActual If true, command is the actual command  - result of its execution can be written to valReply, and its error are propagated by throwinf OpenTplError.
		 *
		 * @return 0 on know command, 1 on command complete
		 * @throws OpenTplError
		 */
		int handleCommand (char *buffer, bool isActual); 

		/**
		 * Check if connection to TPL is running.
		 *
		 * @return True if connetion is running.
		 */
		bool isOK ()
		{
			return true;
			// return (tplc->IsAuth () && tplc->IsConnected ());
		}

		/**
		 * Check if given module exists.
		 *
		 * @param _name  Module _name.
		 * @return True if module exists.
		 */
		bool haveModule (const char *_name)
		{
			int ver;
			int tpl_status = TPL_OK;

			char *vName = new char[strlen (_name) + 9];
			strcpy (vName, _name);
			strcat (vName, ".VERSION");

			tpl_status = tpl_get (vName, ver, &tpl_status);

			delete []vName;

			return tpl_status == TPL_OK && ver > 0;
		}

		/**
		 * Set Rts2ValueDouble from OpenTPL.
		 *
		 * @param _name   Value _name.
		 * @param value  Rts2ValueDouble which will be set.
		 * @param tpl_status OpenTPL status.
		 *
		 * @return OpenTPL status of the get operation.
		 */
		int getValueDouble (const char *_name, Rts2ValueDouble *value, int *tpl_status)
		{
			double val;
			int ret = tpl_get (_name, val, tpl_status);
			if (ret == TPL_OK)
				value->setValueDouble (val);

			return ret;
		}

		int getError (int in_error, std::string & desc)
		{
			char *txt;
			std::string err_desc;
			std::ostringstream os;
			int tpl_status = TPL_OK;
			int errNum = in_error & 0x00ffffff;
			asprintf (&txt, "CABINET.STATUS.TEXT[%i]", errNum);
			tpl_status = tpl_get (txt, err_desc, &tpl_status);
			if (tpl_status)
				os << "Telescope getting error: " << tpl_status
					<<  " sev:" <<  std::hex << (in_error & 0xff000000)
					<< " err:" << std::hex << errNum;
			else
				os << "Telescope sev: " << std::hex << (in_error & 0xff000000)
					<< " err:" << std::hex << errNum << " desc: " << err_desc;
			free (txt);
			desc = os.str ();
			return tpl_status;
		}
};

}

#endif  /* !__RTS2_CONNOPENTPL__ */
