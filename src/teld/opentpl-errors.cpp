/* 
 * Driver for OpemTPL mounts.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "../utils/connopentpl.h"
#include "../utils/rts2cliapp.h"
#include "../utils/rts2config.h"

#include <sstream>
#include <iomanip>

#define OPT_SAMPLE          OPT_LOCAL+5
#define OPT_RESET_MODEL     OPT_LOCAL+6

using namespace rts2core;

class OpenTPLAxis
{
	private:
		double referenced;
		double currpos;
		double targetpos;
		double offset;
		double realpos;
		double power;
		double power_state;
		int version;
		const char *name;
	public:
		OpenTPLAxis (const char *_name, double _referenced, double _currpos,
			double _targetpos, double _offset, double _realpos,
			double _power, double _power_state, int _version);
		friend std::ostream & operator << (std::ostream & _os, OpenTPLAxis irax);
};

OpenTPLAxis::OpenTPLAxis (const char *_name, double _referenced, double _currpos,
double _targetpos, double _offset, double _realpos,
double _power, double _power_state, int _version)
{
	name = _name;
	referenced = _referenced;
	currpos = _currpos;
	targetpos = _targetpos;
	offset = _offset;
	realpos = _realpos;
	power = _power;
	power_state = _power_state;
	version = _version;
}


std::ostream & operator << (std::ostream & _os, OpenTPLAxis irax)
{
	std::ios_base::fmtflags old_settings = _os.flags ();
	_os.setf (std::ios_base::fixed, std::ios_base::floatfield);
	_os
		<< irax.name << ".REFERENCED " << irax.referenced << std::endl
		<< irax.name << ".CURRPOS " << irax.currpos << std::endl
		<< irax.name << ".TARGETPOS " << irax.targetpos << std::endl
		<< irax.name << ".OFFSET " << irax.offset << std::endl
		<< irax.name << ".REALPOS " << irax.realpos << std::endl
		<< irax.name << ".POWER " << irax.power << std::endl
		<< irax.name << ".POWER_STATE " << irax.power_state << std::endl
		<< irax.name << ".VERSION " << irax.version << std::endl;
	_os.setf (old_settings);
	return _os;
}


class AppOpenTPLError:public Rts2CliApp
{
	private:
		std::string ir_ip;
		int ir_port;

		std::list < const char *> errList;
		enum { NO_OP, CAL, RESET, REFERENCED, SAMPLE }
		op;

		OpenTPLAxis getAxisStatus (const char *ax_name);
		rts2core::OpenTpl *opentplConn;

		int doReferenced ();
	protected:
		virtual int processOption (int in_opt);
		virtual int processArgs (const char *arg);
		virtual int init ();
	public:
		AppOpenTPLError (int in_argc, char **in_argv);
		virtual ~ AppOpenTPLError (void)
		{
			delete opentplConn;
		}
		virtual int doProcessing ();
};

OpenTPLAxis
AppOpenTPLError::getAxisStatus (const char *ax_name)
{
	double referenced = rts2_nan ("f");
	double currpos = rts2_nan ("f");
	double targetpos = rts2_nan ("f");
	double offset = rts2_nan ("f");
	double realpos = rts2_nan ("f");
	double power = rts2_nan ("f");
	double power_state = rts2_nan ("f");
	int version = 0;
	std::ostringstream * os;
	int status = 0;

	os = new std::ostringstream ();
	(*os) << ax_name << ".REFERENCED";
	status = opentplConn->get (os->str ().c_str (), referenced, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".CURRPOS";
	status = opentplConn->get (os->str ().c_str (), currpos, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".TARGETPOS";
	status = opentplConn->get (os->str ().c_str (), targetpos, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".OFFSET";
	status = opentplConn->get (os->str ().c_str (), offset, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".REALPOS";
	status = opentplConn->get (os->str ().c_str (), realpos, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".POWER";
	status = opentplConn->get (os->str ().c_str (), power, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".POWER";
	status = opentplConn->get (os->str ().c_str (), power_state, &status);
	delete os;
	os = new std::ostringstream ();
	(*os) << ax_name << ".VERSION";
	status = opentplConn->get (os->str ().c_str (), version, &status);
	delete os;
	return OpenTPLAxis (ax_name, referenced, currpos, targetpos, offset, realpos,
		power, power_state, version);
}


int
AppOpenTPLError::doReferenced ()
{
	int status = 0;
	int track;
	double fpar;
	std::string slist;
	status = opentplConn->get ("CABINET.REFERENCED", fpar, &status);
	std::cout << "CABINET.REFERENCED " << fpar << std::endl;
	status = opentplConn->get ("CABINET.POWER", fpar, &status);
	std::cout << "CABINET.POWER " << fpar << std::endl;
	status = opentplConn->get ("CABINET.POWER_STATE", fpar, &status);
	std::cout << "CABINET.POWER_STATE " << fpar << std::endl;

	status = opentplConn->get ("CABINET.STATUS.LIST", slist, &status);
	std::cout << "CABINET.STATUS.LIST " << slist << std::endl;

	status = opentplConn->get ("POINTING.TRACK", track, &status);
	std::cout << "POINTING.TRACK " << track << std::endl;
	status = opentplConn->get ("POINTING.CURRENT.RA", fpar, &status);
	std::cout << "POINTING.CURRENT.RA " << fpar << std::endl;
	status = opentplConn->get ("POINTING.CURRENT.DEC", fpar, &status);
	std::cout << "POINTING.CURRENT.DEC " << fpar << std::endl;

	status = opentplConn->get ("POINTING.TARGET.RA", fpar, &status);
	std::cout << "POINTING.TARGET.RA " << fpar << std::endl;
	status = opentplConn->get ("POINTING.TARGET.DEC", fpar, &status);
	std::cout << "POINTING.TARGET.DEC " << fpar << std::endl;

	std::cout << getAxisStatus ("ZD");
	std::cout << getAxisStatus ("AZ");
	std::cout << getAxisStatus ("FOCUS");
	std::cout << getAxisStatus ("MIRROR");
	std::cout << getAxisStatus ("DEROTATOR[3]");
	if (opentplConn->haveModule ("COVER"))
		std::cout << getAxisStatus ("COVER");
	if (opentplConn->haveModule ("DOME[0]"))
		std::cout << getAxisStatus ("DOME[0]");
	if (opentplConn->haveModule ("DOME[1]"))
		std::cout << getAxisStatus ("DOME[1]");
	if (opentplConn->haveModule ("DOME[2]"))
		std::cout << getAxisStatus ("DOME[2]");
	return status;
}


AppOpenTPLError::AppOpenTPLError (int in_argc, char **in_argv):
Rts2CliApp (in_argc, in_argv)
{
	ir_port = 0;
	opentplConn = NULL;

	op = NO_OP;

	addOption ('I', "ir_ip", 1, "IR TCP/IP address");
	addOption ('N', "ir_port", 1, "IR TCP/IP port number");

	addOption ('c', NULL, 0, "Calculate model");
	addOption (OPT_RESET_MODEL, "reset-model", 0, "Reset model counts");
	addOption ('f', NULL, 0, "Referencing status");
	addOption (OPT_SAMPLE, "sample", 0, "Sample measurements");
}


int
AppOpenTPLError::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'I':
			ir_ip = std::string (optarg);
			break;
		case 'N':
			ir_port = atoi (optarg);
			break;
		case 'c':
			op = CAL;
			break;
		case OPT_RESET_MODEL:
			op = RESET;
			break;
		case 'f':
			op = REFERENCED;
			break;
		case OPT_SAMPLE:
			op = SAMPLE;
			break;
		default:
			return Rts2App::processOption (in_opt);
	}
	return 0;
}


int
AppOpenTPLError::processArgs (const char *arg)
{
	errList.push_back (arg);
	return 0;
}


int
AppOpenTPLError::init ()
{
	int ret;
	ret = Rts2App::init ();
	if (ret)
		return ret;

	Rts2Config *config = Rts2Config::instance ();
	config->loadFile (NULL);
	// try to get default from config file
	if (ir_ip.length () == 0)
	{
		config->getString ("opentpl", "ip", ir_ip);
	}
	if (!ir_port)
	{
		config->getInteger ("opentpl", "port", ir_port);
	}
	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	opentplConn = new rts2core::OpenTpl (NULL, ir_ip, ir_port);
	try
	{
		opentplConn->init();
	}
	catch (ConnError er)
	{
		std::cerr << "Cannot init connection to mount server, error: " << er << std::endl;
	}

	return 0;
}


int
AppOpenTPLError::doProcessing ()
{
	std::string descri;
	for (std::list < const char *>::iterator iter = errList.begin ();
		iter != errList.end (); iter++)
	{
		opentplConn->getError (atoi (*iter), descri);
		std::cout << descri << std::endl;
	}
	int status = 0;
	double fparam;

	switch (op)
	{
		case SAMPLE:
			status = opentplConn->set ("POINTING.POINTINGPARAMS.SAMPLE", 1, &status);
		case NO_OP:
			break;
		case CAL:
			fparam = 2;
			status = opentplConn->set ("POINTING.POINTINGPARAMS.SAVE", 1, &status);
			status = opentplConn->set ("POINTING.POINTINGPARAMS.CALCULATE", fparam, &status);
			break;
		case RESET:
			fparam = 0;
			status =
				opentplConn->set ("POINTING.POINTINGPARAMS.RECORDCOUNT", fparam, &status);
			break;
		case REFERENCED:
			return doReferenced ();
	}

	// dump model
	double aoff, zoff, ae, an, npae, ca, flex;
	int recordcount;
	int track;
	std::string dumpfile;

	status = opentplConn->get ("POINTING.POINTINGPARAMS.DUMPFILE", dumpfile, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.AOFF", aoff, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.ZOFF", zoff, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.AE", ae, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.AN", an, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.NPAE", npae, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.CA", ca, &status);
	status = opentplConn->get ("POINTING.POINTINGPARAMS.FLEX", flex, &status);

	std::cout << "POINTING.POINTINGPARAMS.DUMPFILE " << dumpfile << std::endl;
	std::cout.precision (20);
	std::cout << "AOFF = " << aoff << std::endl;
	std::cout << "ZOFF = " << zoff << std::endl;
	std::cout << "AE = " << ae << std::endl;
	std::cout << "AN = " << an << std::endl;
	std::cout << "NPAE = " << npae << std::endl;
	std::cout << "CA = " << ca << std::endl;
	std::cout << "FLEX = " << flex << std::endl;
	// dump offsets
	status = opentplConn->get ("AZ.OFFSET", aoff, &status);
	status = opentplConn->get ("ZD.OFFSET", zoff, &status);

	std::cout << "AZ.OFFSET " << aoff << std::endl;
	std::cout << "ZD.OFFSET " << zoff << std::endl;

	status = opentplConn->get ("POINTING.TRACK ", track, &status);
	std::cout << "POINTING.TRACK " << track << std::endl;

	status =
		opentplConn->get ("POINTING.POINTINGPARAMS.RECORDCOUNT ", recordcount, &status);

	std::cout << "POINTING.POINTINGPARAMS.RECORDCOUNT " << recordcount << std::
		endl;

	status = opentplConn->get ("POINTING.POINTINGPARAMS.CALCULATE", fparam, &status);

	std::cout << "POINTING.POINTINGPARAMS.CALCULATE " << fparam << std::endl;

	return 0;
}


int
main (int argc, char **argv)
{
	AppOpenTPLError device = AppOpenTPLError (argc, argv);
	return device.run ();
}
