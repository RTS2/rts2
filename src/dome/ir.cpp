/* 
 * Driver for IR (OpenTPL) dome.
 * Copyright (C) 2008 Petr Kubanek <petr@kubanek.net>
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

#include "dome.h"
#include "irconn.h"
#include "../utils/rts2config.h"

#include <libnova/libnova.h>

namespace rts2dome
{

/**
 * Driver for Bootes IR dome.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Ir:public Dome
{
	private:
		std::string ir_ip;
		int ir_port;

		IrConn *irConn;

		Rts2ValueFloat *domeUp;
		Rts2ValueFloat *domeDown;

		Rts2ValueDouble *domeCurrAz;
		Rts2ValueDouble *domeTargetAz;
		Rts2ValueBool *domePower;
		Rts2ValueDouble *domeTarDist;

		Rts2ValueBool *domeAutotrack;

		/**
		 * Set dome autotrack.
		 *
		 * @param new_auto New auto track value. True or false.
		 *
		 * @return -1 on error, 0 on success.
		 */
		int setDomeTrack (bool new_auto);
		int initIrDevice ();

	protected:
		virtual int processOption (int in_opt);

		virtual int setValue (Rts2Value *old_value, Rts2Value *new_value);

	public:
		Ir (int argc, char **argv);
		virtual ~ Ir (void);
		virtual int init ();

		virtual int info ();

		virtual int startOpen ();
		virtual long isOpened ();
		virtual int endOpen ();

		virtual int startClose ();
		virtual long isClosed ();
		virtual int endClose ();
};

}

using namespace rts2dome;

int
Ir::setValue (Rts2Value *old_value, Rts2Value *new_value)
{	
	int status = TPL_OK;
	if (old_value == domeAutotrack)
	{
		status = setDomeTrack (((Rts2ValueBool *) new_value)->getValueBool ());
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == domeUp)
	{
		status = irConn->tpl_set ("DOME[1].TARGETPOS", new_value->getValueFloat (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == domeDown)
	{
		status = irConn->tpl_set ("DOME[2].TARGETPOS", new_value->getValueFloat (), &status);
		if (status != TPL_OK)
			return -2;
		return 0;
	}
	if (old_value == domeTargetAz)
	{
		status = irConn->tpl_set ("DOME[0].TARGETPOS",
			ln_range_degrees (new_value->getValueDouble () - 180.0),
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;

	}
	if (old_value == domePower)
	{
		status =  irConn->tpl_set ("DOME[0].POWER",
			((Rts2ValueBool *) new_value)->getValueBool ()? 1 : 0,
			&status);
		if (status != TPL_OK)
			return -2;
		return 0;

	}
	return Dome::setValue (old_value, new_value);
}


int
Ir::startOpen ()
{
	int status = TPL_OK;
	status = irConn->tpl_set ("DOME[1].TARGETPOS", 1, &status);
	status = irConn->tpl_set ("DOME[2].TARGETPOS", 1, &status);
	logStream (MESSAGE_INFO) << "opening dome, status " << status << sendLog;
	if (status != TPL_OK)
		return -1;
	return 0;
}


long
Ir::isOpened ()
{
	int status = TPL_OK;
	double pos1, pos2;
	status = irConn->tpl_get ("DOME[1].CURRPOS", pos1, &status);
	status = irConn->tpl_get ("DOME[2].CURRPOS", pos2, &status);

	if (status != TPL_OK)
		return -1;

	if (pos1 == 1 && pos2 == 1)
		return -2;

	return USEC_SEC;
}


int
Ir::endOpen ()
{
	return 0;
}


int
Ir::startClose ()
{
	int status = TPL_OK;
	status = irConn->tpl_set ("DOME[1].TARGETPOS", 0, &status);
	status = irConn->tpl_set ("DOME[2].TARGETPOS", 0, &status);
	logStream (MESSAGE_INFO) << "closing dome, status " << status << sendLog;
	if (status != TPL_OK)
		return -1;
	return 0;
}


long
Ir::isClosed ()
{
	int status = TPL_OK;
	double pos1, pos2;
	status = irConn->tpl_get ("DOME[1].CURRPOS", pos1, &status);
	status = irConn->tpl_get ("DOME[2].CURRPOS", pos2, &status);

	if (status != TPL_OK)
		return -1;

	if (pos1 == 0 && pos2 == 0)
		return -2;

	return USEC_SEC;
}


int
Ir::endClose ()
{
	return 0;
}


int
Ir::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'I':
			ir_ip = std::string (optarg);
			break;
		case 'N':
			ir_port = atoi (optarg);
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}


Ir::Ir (int argc, char **argv)
:Dome (argc, argv)
{
	ir_ip = std::string ("");
	ir_port = 0;

	createValue (domeAutotrack, "dome_auto_track", "dome auto tracking", false);
	domeAutotrack->setValueBool (true);

	createValue (domeUp, "dome_up", "upper dome cover", false);
	createValue (domeDown, "dome_down", "dome down cover", false);

	createValue (domeCurrAz, "dome_curr_az", "dome current azimunt", false, RTS2_DT_DEGREES);
	createValue (domeTargetAz, "dome_target_az", "dome targer azimut", false, RTS2_DT_DEGREES);
	createValue (domePower, "dome_power", "if dome have power", false);
	createValue (domeTarDist, "dome_tar_dist", "dome target distance", false, RTS2_DT_DEG_DIST);

	addOption ('I', "ir_ip", 1, "IR TCP/IP address");
	addOption ('N', "ir_port", 1, "IR TCP/IP port number");
}


Ir::~Ir (void)
{

}


int
Ir::setDomeTrack (bool new_auto)
{
	int status = TPL_OK;
	int old_track;
	status = irConn->tpl_get ("POINTING.TRACK", old_track, &status);
	if (status != TPL_OK)
		return -1;
	old_track &= ~128;
	status = irConn->tpl_set ("POINTING.TRACK", old_track | (new_auto ? 128 : 0), &status);
	if (status != TPL_OK)
	{
		logStream (MESSAGE_ERROR) << "Cannot setDomeTrack" << sendLog;
	}
	return status;
}


int
Ir::initIrDevice ()
{
	Rts2Config *config = Rts2Config::instance ();
	config->loadFile (NULL);
	// try to get default from config file
	if (ir_ip.length () == 0)
	{
		config->getString ("ir", "ip", ir_ip);
	}
	if (!ir_port)
	{
		config->getInteger ("ir", "port", ir_port);
	}
	if (ir_ip.length () == 0 || !ir_port)
	{
		std::cerr << "Invalid port or IP address of mount controller PC"
			<< std::endl;
		return -1;
	}

	irConn = new IrConn (ir_ip, ir_port);

	// are we connected ?
	if (!irConn->isOK ())
	{
		std::cerr << "Connection to server failed" << std::endl;
		return -1;
	}

	return 0;
}


int
Ir::info ()
{
	double dome_curr_az, dome_target_az, dome_tar_dist, dome_power, dome_up, dome_down;
	int status = TPL_OK;
	status = irConn->tpl_get ("DOME[0].CURRPOS", dome_curr_az, &status);
	status = irConn->tpl_get ("DOME[0].TARGETPOS", dome_target_az, &status);
	status = irConn->tpl_get ("DOME[0].TARGETDISTANCE", dome_tar_dist, &status);
	status = irConn->tpl_get ("DOME[0].POWER", dome_power, &status);
	status = irConn->tpl_get ("DOME[1].CURRPOS", dome_up, &status);
	status = irConn->tpl_get ("DOME[2].CURRPOS", dome_down, &status);
	if (status == TPL_OK)
	{
		domeCurrAz->setValueDouble (ln_range_degrees (dome_curr_az + 180));
		domeTargetAz->setValueDouble (ln_range_degrees (dome_target_az + 180));
		domeTarDist->setValueDouble (dome_tar_dist);
		domePower->setValueBool (dome_power == 1);
		domeUp->setValueFloat (dome_up);
		domeDown->setValueFloat (dome_down);
	}

	return Dome::info ();
}


int
Ir::init ()
{
	int ret = Dome::init ();
	if (ret)
		return ret;
	return initIrDevice ();
}


int
main (int argc, char **argv)
{
	Ir device = Ir (argc, argv);
	return device.run ();
}
