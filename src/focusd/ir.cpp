/*!
 * $Id$
 *
 * @author standa
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <opentpl/client.h>

#include "focuser.h"

using namespace OpenTPL;

class Rts2DevFocuserIr:public Rts2DevFocuser
{
	private:
		char *ir_ip;
		int ir_port;
		Client *tplc;

		Rts2ValueInteger *realPos;
		// low-level tpl functions
		template < typename T > int tpl_get (const char *name, T & val, int *status);
		template < typename T > int tpl_set (const char *name, T val, int *status);
		template < typename T > int tpl_setw (const char *name, T val, int *status);
		// low-level image fuctions
		// int data_handler (int sock, size_t size, struct image_info *image);
		// int readout ();
		// high-level I/O functions
		// int foc_pos_get (int * position);
		// int foc_pos_set (int pos);
	protected:
		virtual int endFocusing ();
		virtual bool isAtStartPosition ();
	public:
		Rts2DevFocuserIr (int argc, char **argv);
		virtual ~ Rts2DevFocuserIr (void);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int ready ();
		virtual int initValues ();
		virtual int info ();
		virtual int stepOut (int num);
		virtual int setTo (int num);
		virtual int isFocusing ();
};

template < typename T > int
Rts2DevFocuserIr::tpl_get (const char *name, T & val, int *status)
{
	int cstatus;

	if (!*status)
	{
		Request *r = tplc->Get (name, false);
		cstatus = r->Wait (USEC_SEC);

		if (cstatus != TPLC_OK)
		{
			logStream (MESSAGE_ERROR) << "focuser IR tpl_get error " << cstatus
				<< sendLog;
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

		delete r;
	}
	return *status;
}


template < typename T > int
Rts2DevFocuserIr::tpl_set (const char *name, T val, int *status)
{
	int cstatus;

	if (!*status)
	{
								 // change to set...?
		Request *r = tplc->Set (name, Value (val), true);
		r->Wait (2000);

		delete r;
	}
	return *status;
}


template < typename T > int
Rts2DevFocuserIr::tpl_setw (const char *name, T val, int *status)
{
	int cstatus;

	if (!*status)
	{
								 // change to set...?
		Request *r = tplc->Set (name, Value (val), false);
		cstatus = r->Wait ();

		if (cstatus != TPLC_OK)
		{
			logStream (MESSAGE_ERROR) << "focuser IR tpl_setw error " << cstatus
				<< sendLog;
			*status = 1;
		}

		delete r;
	}
	return *status;
}


Rts2DevFocuserIr::Rts2DevFocuserIr (int in_argc, char **in_argv):Rts2DevFocuser (in_argc,
in_argv)
{
	createValue (realPos, "FOC_REAL", "real position of the focuser", true, 0, 0, false);

	ir_ip = NULL;
	ir_port = 0;
	tplc = NULL;

	addOption ('I', "ir_ip", 1, "IR TCP/IP adress");
	addOption ('P', "ir_port", 1, "IR TCP/IP port number");

	strcpy (focType, "BOOTES_IR");
}


Rts2DevFocuserIr::~Rts2DevFocuserIr ()
{
	delete tplc;
}


int
Rts2DevFocuserIr::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'I':
			ir_ip = optarg;
			break;
		case 'P':
			ir_port = atoi (optarg);
			break;
		default:
			return Rts2DevFocuser::processOption (in_opt);
	}
	return 0;
}


/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int
Rts2DevFocuserIr::init ()
{
	int ret;
	ret = Rts2DevFocuser::init ();
	if (ret)
		return ret;

	if (!ir_ip || !ir_port)
	{
		fprintf (stderr, "Invalid port or IP address of mount controller PC\n");
		return -1;
	}

	tplc = new Client (ir_ip, ir_port, 2000);

	logStream (MESSAGE_DEBUG) << "focuser IR status: ConnId = " << tplc->
		ConnId () << " , connected: " << (tplc->
		IsConnected ()? "yes" : "no") <<
		" , authenticated " << (tplc->
		IsAuth ()? "yes" : "no") << ", Welcome Message = "
		<< tplc->WelcomeMessage ().c_str () << sendLog;

	if (!tplc->IsAuth () || !tplc->IsConnected ())
	{
		logStream (MESSAGE_ERROR) << "focuser IR - connection to server failed"
			<< sendLog;
		return -1;
	}

	ret = checkStartPosition ();

	return ret;
}


int
Rts2DevFocuserIr::ready ()
{
	return !tplc->IsConnected ();
}


int
Rts2DevFocuserIr::initValues ()
{
	strcpy (focType, "FIR");
	return Rts2DevFocuser::initValues ();
}


int
Rts2DevFocuserIr::info ()
{
	int status = 0;

	if (!(tplc->IsAuth () && tplc->IsConnected ()))
		return -1;

	double f_realPos;
	status = tpl_get ("FOCUS.REALPOS", f_realPos, &status);
	if (status)
		return -1;

	realPos->setValueInteger ((int) (f_realPos * 1000.0));

	return Rts2DevFocuser::info ();
}


int
Rts2DevFocuserIr::stepOut (int num)
{
	int status = 0;

	int power = 1;
	int referenced = 0;
	double offset;

	status = tpl_get ("FOCUS.REFERENCED", referenced, &status);
	if (referenced != 1)
	{
		logStream (MESSAGE_ERROR) << "focuser IR stepOut referenced is: " <<
			referenced << sendLog;
		return -1;
	}
	status = tpl_setw ("FOCUS.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "focuser IR stepOut cannot set POWER to 1"
			<< sendLog;
	}
	status = tpl_get ("FOCUS.OFFSET", offset, &status);
	offset += (double) num / 1000.0;
	status = tpl_setw ("FOCUS.OFFSET", offset, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "focuser IR stepOut cannot set offset!" <<
			sendLog;
		return -1;
	}
	return 0;
}


int
Rts2DevFocuserIr::setTo (int num)
{
	int status = 0;

	int power = 1;
	int referenced = 0;
	double offset;

	status = tpl_get ("FOCUS.REFERENCED", referenced, &status);
	if (referenced != 1)
	{
		logStream (MESSAGE_ERROR) << "focuser IR stepOut referenced is: " <<
			referenced << sendLog;
		return -1;
	}
	status = tpl_setw ("FOCUS.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "focuser IR stepOut cannot set POWER to 1"
			<< sendLog;
	}
	offset = (double) num / 1000.0;
	status = tpl_setw ("FOCUS.OFFSET", offset, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "focuser IR stepOut cannot set offset!" <<
			sendLog;
		return -1;
	}
	setFocusTimeout (100);
	return 0;
}


int
Rts2DevFocuserIr::isFocusing ()
{
	double targetdistance;
	int status = 0;
	status = tpl_get ("FOCUS.TARGETDISTANCE", targetdistance, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "focuser IR isFocusing status: " << status
			<< sendLog;
		return -1;
	}
	return (fabs (targetdistance) < 0.001) ? -2 : USEC_SEC / 50;
}


int
Rts2DevFocuserIr::endFocusing ()
{
	int status = 0;
	int power = 0;
	status = tpl_setw ("FOCUS.POWER", power, &status);
	if (status)
	{
		logStream (MESSAGE_ERROR) <<
			"focuser IR endFocusing cannot set POWER to 0" << sendLog;
		return -1;
	}
	return 0;
}


bool
Rts2DevFocuserIr::isAtStartPosition ()
{
	int ret;
	ret = info ();
	if (ret)
		return false;
	return (fabs ((float) getFocPos ()) < 50);
}


int
main (int argc, char **argv)
{
	Rts2DevFocuserIr device = Rts2DevFocuserIr (argc, argv);
	return device.run ();
}
