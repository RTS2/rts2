/* 
 * Connection for OpenTPL.
 * Copyright (C) 2007-2008 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_IR_CONN__
#define __RTS2_IR_CONN__

#include <opentpl/client.h>
#include "../utils/rts2app.h"	 // logStream for logging
#include "../utils/rts2value.h"

using namespace OpenTPL;

/**
 * Connection for access to OpenTPL.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class IrConn
{
	private:
		Client *tplc;
	public:
		IrConn (std::string ir_host, int ir_port)
		{
			tplc = new Client (ir_host, ir_port);
			logStream (MESSAGE_DEBUG)
				<< "Status: ConnId = " << tplc->ConnId ()
				<< " connected: " << (tplc->IsConnected () ? "yes" : "no")
				<< " authenticated " << (tplc->IsAuth () ? "yes" : "no")
				<< " Welcome Message " << tplc->WelcomeMessage ().c_str ()
				<< sendLog;
		}

		~IrConn (void)
		{
			delete tplc;
		}

		/**
		 * Check if connection to TPL is running.
		 *
		 * @return True if connetion is running.
		 */
		bool isOK ()
		{
			return (tplc->IsAuth () && tplc->IsConnected ());
		}

		template < typename T > int tpl_get (const char *name, T & val, int *status);

		/**
		 * Check if given module exists.
		 *
		 * @param name  Module name.
		 * @return True if module exists.
		 */
		bool haveModule (const char *name)
		{
			int ver;
			int status = TPL_OK;

			char *vName = new char[strlen (name) + 9];
			strcpy (vName, name);
			strcat (vName, ".VERSION");

			status = tpl_get (vName, ver, &status);

			delete []vName;

			return status == TPL_OK && ver > 0;
		}

		/**
		 * Set Rts2ValueDouble from OpenTPL.
		 *
		 * @param name   Value name.
		 * @param value  Rts2ValueDouble which will be set.
		 * @param status OpenTPL status.
		 *
		 * @return OpenTPL status of the get operation.
		 */
		int getValueDouble (const char *name, Rts2ValueDouble *value, int *status)
		{
			double val;
			int ret = tpl_get (name, val, status);
			if (ret == TPL_OK)
				value->setValueDouble (val);

			return ret;
		}

		/**
		 * Set value. Do not wait for result.
		 *
		 * @param name   Value name.
		 * @param val    Value.
		 * @param status OpenTPL status.
		 *
		 * @return OpenTPL status of the operation.
		 */
		template < typename T > int tpl_set (const char *name, T val, int *status);
		
		/**
		 * Set value and wait for result.
		 *
		 * @param name   Value name.
		 * @param val    Value.
		 * @param status OpenTPL status.
		 *
		 * @return OpenTPL status of the operation.
		 */
		template < typename T > int tpl_setw (const char *name, T val, int *status);

		int getError (int in_error, std::string & desc)
		{
			char *txt;
			std::string err_desc;
			std::ostringstream os;
			int status = TPL_OK;
			int errNum = in_error & 0x00ffffff;
			asprintf (&txt, "CABINET.STATUS.TEXT[%i]", errNum);
			status = tpl_get (txt, err_desc, &status);
			if (status)
				os << "Telescope getting error: " << status
					<<  " sev:" <<  std::hex << (in_error & 0xff000000)
					<< " err:" << std::hex << errNum;
			else
				os << "Telescope sev: " << std::hex << (in_error & 0xff000000)
					<< " err:" << std::hex << errNum << " desc: " << err_desc;
			free (txt);
			desc = os.str ();
			return status;
		}
};

template < typename T > int
IrConn::tpl_get (const char *name, T & val, int *status)
{
	OpenTPL::ClientStatus cstatus = TPLC_OK;

	if (*status == TPL_OK)
	{
		#ifdef DEBUG_ALL
		std::cout << "tpl_get name " << name << std::endl;
		#endif
		Request *r = tplc->Get (name, false, cstatus);
		if (r == NULL)
		{
			logStream (MESSAGE_ERROR) << "While getting " << name << ", request object is NULL, status: " 
				<< cstatus << sendLog;
			return -1;
		}
		cstatus = r->Wait (5000);

		if (cstatus != TPLC_OK)
		{
			logStream (MESSAGE_ERROR) << "IR tpl_get error " << name
				<< " status " << cstatus << sendLog;
			r->Abort ();
			*status = 1;
		}

		if (!*status)
		{
			RequestAnswer & answr = r->GetAnswer ();

			if (answr.begin ()->result == TPL_OK)
				val = (T) * (answr.begin ()->values.begin ());
			else
				*status = 2;
		}

		#ifdef DEBUG_ALL
		if (!*status)
			std::cout << "tpl_get name " << name << " val " << val << std::endl;
		#endif

		delete r;
	}
	return *status;
}


template < typename T > int
IrConn::tpl_set (const char *name, T val, int *status)
{
	// int cstatus;

	if (*status == TPL_OK)
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "tpl_set name " << name << " val " << val
			<< sendLog;
		#endif
		
		// autodispose work only correctly only with new TPL library
		tplc->Set (name, Value (val), true);

		#ifdef DEBUG_ALL
		if (!*status)
			std::cout << "tpl_set name " << name << " val " << val << std::endl;
		#endif

	}
	return *status;
}


template < typename T > int
IrConn::tpl_setw (const char *name, T val, int *status)
{
	int cstatus;

	if (*status == TPL_OK)
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "tpl_setw name " << name << " val " << val
			<< sendLog;
		#endif

		Request *r = tplc->Set (name, Value (val), false);
		cstatus = r->Wait (5000);

		if (cstatus != TPLC_OK)
		{
			logStream (MESSAGE_ERROR) << "IR tpl_setw error " << name << " val "
				<< val << " status " << cstatus << sendLog;
			r->Abort ();
			*status = 1;
		}

		#ifdef DEBUG_ALL
		if (!*status)
			std::cout << "tpl_setw name " << name << " val " << val << std::endl;
		#endif

		delete r;
	}
	return *status;
}
#endif							 /* ! __RTS2_IR_CONN__ */
