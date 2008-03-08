#ifndef __RTS2_IR_CONN__
#define __RTS2_IR_CONN__

#include <opentpl/client.h>
#include "../utils/rts2app.h"	 // logStream for logging

using namespace OpenTPL;

/**
 * Class which enables access to OpenTPL.
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
		template < typename T > int tpl_set (const char *name, T val, int *status);
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
	int cstatus = TPL_OK;

	if (!*status)
	{
		#ifdef DEBUG_ALL
		std::cout << "tpl_get name " << name << std::endl;
		#endif
		Request *r = tplc->Get (name, false);
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
	//  int cstatus;

	if (!*status)
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "tpl_set name " << name << " val " << val
			<< sendLog;
		#endif
								 // change to set...?
		tplc->Set (name, Value (val), false);
		//      cstatus = r->Wait (5000);

		#ifdef DEBUG_ALL
		if (!*status)
			std::cout << "tpl_set name " << name << " val " << val << std::endl;
		#endif

		//      delete r;
	}
	return *status;
}


template < typename T > int
IrConn::tpl_setw (const char *name, T val, int *status)
{
	int cstatus;

	if (!*status)
	{
		#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "tpl_setw name " << name << " val " << val
			<< sendLog;
		#endif

								 // change to set...?
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
