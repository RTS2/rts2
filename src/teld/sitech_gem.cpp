/* 
 * Sidereal Technology Controller driver
 * Copyright (C) 2012-2013 Shashikiran Ganesh <shashikiran.ganesh@gmail.com>
 * Copyright (C) 2014 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2014 ISDEFE/ESA
 * Based on John Kielkopf's xmtel linux software, Dan's SiTech driver and documentation, and dummy telescope driver.
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

#include "gem.h"
#include "configuration.h"

#include "sitech.h"
#include "connection/sitech.h"

namespace rts2teld
{

/**
 * Sidereal Technology GEM telescope driver.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Sitech:public GEM
{
	public:
		Sitech (int argc, char **argv);
		~Sitech (void);

	protected:
		virtual int processOption (int in_opt);

		virtual int commandAuthorized (rts2core::Connection *conn);

		virtual int initValues ();
		virtual int initHardware ();

		virtual int info ();

		virtual int startResync ();

		virtual int stopMove ()
		{
			fullStop();
			return 0;
		}

		virtual int startPark ();

		virtual int endPark ()
		{
			return 0;
		}


		virtual int isMoving ();

		virtual int isMovingFixed ()
		{
			return isMoving ();
		}

		virtual int isParking ()
		{
			return isMoving ();
		}

		virtual double estimateTargetTime ()
		{
			return getTargetDistance () * 2.0;
		}

		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int updateLimits ();

		/**
		 * Gets home offset.
		 */
		virtual int getHomeOffset (int32_t & off)
		{
			off = 0;
			return 0;
		}

	private:
		/**
		 * Sends SiTech XYS command with requested coordinates.
		 */
		void sitechMove (int32_t ac, int32_t dc);

		ConnSitech *serConn;

		SitechAxisStatus radec_status;
		SitechAxisRequest radec_request;

		rts2core::ValueLong *ra_pos;
		rts2core::ValueLong *dec_pos;

		rts2core::ValueLong *ra_enc;
		rts2core::ValueLong *dec_enc;

		rts2core::ValueLong *mclock;
		rts2core::ValueInteger *temperature;

		rts2core::ValueInteger *ra_worm_phase;

		rts2core::ValueLong *ra_last;
		rts2core::ValueLong *dec_last;

		// request values - speed,..
		rts2core::ValueLong *ra_speed;
		rts2core::ValueLong *dec_speed;

		rts2core::ValueLong *ra_rate_adder;
		rts2core::ValueLong *dec_rate_adder;

		rts2core::ValueLong *ra_rate_adder_t;
		rts2core::ValueLong *dec_rate_adder_t;

		rts2core::ValueLong *ra_scope_ticks;
		rts2core::ValueLong *dec_scope_ticks;

		rts2core::ValueLong *ra_mot_ticks;
		rts2core::ValueLong *dec_mot_ticks;

		double offsetha;
		double offsetdec;
                 
		int pmodel;                    /* pointing model default to raw data */
	
		/* Communications variables and routines for internal use */
		const char *device_file;

		void fullStop ();
		
		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec);

		void GetGuideTargets (int *ntarget, int *starget, int *etarget, int *wtarget);
};

}

using namespace rts2teld;

Sitech::Sitech (int argc, char **argv):GEM (argc,argv), radec_status (), radec_request ()
{
	unlockPointing ();

	setCorrections (true, true, true);

	offsetha = 0.;
	offsetdec = 0.;

	pmodel = RAW;

	device_file = "/dev/ttyUSB0";

	createValue (ra_pos, "AXRA", "RA motor axis count", true, RTS2_VALUE_WRITABLE);
	createValue (dec_pos, "AXDEC", "DEC motor axis count", true, RTS2_VALUE_WRITABLE);

	createValue (ra_enc, "ENCRA", "RA encoder readout", true);
	createValue (dec_enc, "ENCDEC", "DEC encoder readout", true);

	createValue (mclock, "mclock", "millisecond board clocks", false);
	createValue (temperature, "temperature", "[C] board temperature (CPU)", false);
	createValue (ra_worm_phase, "y_worm_phase", "RA worm phase", false);
	createValue (ra_last, "ra_last", "RA motor location at last RA scope encoder location change", false);
	createValue (dec_last, "dec_last", "DEC motor location at last DEC scope encoder location change", false);

	createValue (ra_speed, "ra_speed", "RA speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE);
	createValue (dec_speed, "dec_speed", "DEC speed (base rate), in counts per servo loop", false, RTS2_VALUE_WRITABLE);

	createValue (ra_rate_adder, "ra_rate_adder", "RA rate adder", false, RTS2_VALUE_WRITABLE);
	createValue (dec_rate_adder, "dec_rate_adder", "DEC rate adder", false, RTS2_VALUE_WRITABLE);

	createValue (ra_rate_adder_t, "ra_rate_adder_t", "RA rate adder time (in servo loops; 1953 would be 1 second)", false, RTS2_VALUE_WRITABLE);
	createValue (dec_rate_adder_t, "dec_rate_adder_t", "DEC rate adder time (in servo loops; 1953 would be 1 second)", false, RTS2_VALUE_WRITABLE);

	createValue (ra_scope_ticks, "ra_scope_ticks", "RA scope encoder ticks per revolution");
	createValue (dec_scope_ticks, "dec_scope_ticks", "DEC scope encoder ticks per revolution");

	createValue (ra_mot_ticks, "ra_mot_ticks", "RA motor encoder ticks per revolution");
	createValue (dec_mot_ticks, "dec_mot_ticks", "DEC motor encoder ticks per revolution");

	ra_speed->setValueLong (1000000);
	dec_speed->setValueLong (1000000);

	ra_rate_adder->setValueLong (0);
	dec_rate_adder->setValueLong (0);

	ra_rate_adder_t->setValueLong (0);
	dec_rate_adder_t->setValueLong (0);

	createParkPos (0, 89.999);

	addOption ('f', "device_file", 1, "device file (ussualy /dev/ttySx");

	addParkPosOption ();
}

Sitech::~Sitech(void)
{
	delete serConn;
	serConn = NULL;
}

/* Full stop */
void Sitech::fullStop (void)
{ 
	try
	{
		serConn->siTechCommand ('X', "N");
		usleep (100000);
		serConn->siTechCommand ('Y', "N");
	}
	catch (rts2core::Error er)
	{
		logStream (MESSAGE_ERROR) << "cannot send full stop " << er << sendLog;
	}
}

void Sitech::getTel (double &telra, double &teldec, int &telflip, double &un_telra, double &un_teldec)
{
	serConn->getAxisStatus ('X', radec_status);

	ra_pos->setValueLong (radec_status.y_pos);
	dec_pos->setValueLong (radec_status.x_pos);

	ra_enc->setValueLong (radec_status.y_enc);
	dec_enc->setValueLong (radec_status.x_enc);

	mclock->setValueLong (radec_status.mclock);
	temperature->setValueInteger (radec_status.temperature);

	ra_worm_phase->setValueInteger (radec_status.y_worm_phase);

	ra_last->setValueLong (radec_status.y_last);
	dec_last->setValueLong (radec_status.x_last);

	int ret = counts2sky (radec_status.y_pos, radec_status.x_pos, telra, teldec, telflip, un_telra, un_teldec);
	if (ret)
		logStream  (MESSAGE_ERROR) << "error transforming counts" << sendLog;
}

/*!
 * Init telescope, connect on given tel_desc.
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int Sitech::initHardware ()
{
	int ret;
	int numread;

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();

	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	strcpy (telType, "Sitech");

	/* Make the connection */
	
	/* On the Sidereal Technologies controller                                */
	/*   there is an FTDI USB to serial converter that appears as             */
	/*   /dev/ttyUSB0 on Linux systems without other USB serial converters.   */
	/*   The serial device is known to the program that calls this procedure. */
	
	serConn = new ConnSitech (device_file, this);
	serConn->setDebug (getDebug ());

	ret = serConn->init ();

	if (ret)
		return -1;
	serConn->flushPortIO ();

	numread = serConn->getSiTechValue ('X', "V");
     
	if (numread != 0) 
	{
		logStream (MESSAGE_DEBUG) << "Sidereal Technology Controller version " << numread / 10.0 << sendLog;
	}
	else
	{
		logStream (MESSAGE_ERROR) << "A200HR drive control did not respond." << sendLog;
		//return -1;
	}  

	SitechControllerConfiguration sconfig;
	//serConn->getConfiguration (sconfig);

	//serConn->resetController ();
	
	/* Flush the input buffer in case there is something left from startup */

	serConn->flushPortIO();
	return 0;

}

int Sitech::info ()
{
	double t_telRa, t_telDec, ut_telRa, ut_telDec;
	int t_telFlip;

	getTel (t_telRa, t_telDec, t_telFlip, ut_telRa, ut_telDec);
	
	setTelRaDec (t_telRa, t_telDec);
	telFlip->setValueInteger (t_telFlip);
	setTelUnRaDec (ut_telRa, ut_telDec);

	ra_scope_ticks->setValueLong (serConn->getSiTechValue ('X', "XZ"));
	dec_scope_ticks->setValueLong (serConn->getSiTechValue ('X', "XT"));

	ra_mot_ticks->setValueLong (serConn->getSiTechValue ('X', "XV"));
	dec_mot_ticks->setValueLong (serConn->getSiTechValue ('X', "XU"));

	serConn->getSiTechValue ('X', "Z");
	serConn->getSiTechValue ('Y', "Z");

	serConn->getSiTechValue ('X', "");
	serConn->getSiTechValue ('Y', "");

	return rts2teld::GEM::info ();
}

int Sitech::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'f':
			device_file = optarg;
			break;

		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int Sitech::commandAuthorized (rts2core::Connection *conn)
{
	if (conn->isCommand ("zero_motor"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->setSiTechValue ('X', "F", 0);
		serConn->setSiTechValue ('Y', "F", 0);
		return 0;
	}
	else if (conn->isCommand ("reset_controller"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->resetController ();
		return 0;
	}
	else if (conn->isCommand ("go_auto"))
	{
		if (!conn->paramEnd ())
			return -2;
		serConn->siTechCommand ('X', "A");
		serConn->siTechCommand ('Y', "A");
		return 0;
	}
	return GEM::commandAuthorized (conn);
}

int Sitech::initValues ()
{
	return Telescope::initValues ();
}

int Sitech::startResync ()
{
	int32_t ac, dc;
	int ret = sky2counts (ac, dc);
	if (ret)
		return -1;

	sitechMove (ac, dc);
	
	return 0;
}

int Sitech::isMoving ()
{
	return -1;
}

int Sitech::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == ra_pos)
	{
		sitechMove (newValue->getValueLong (), dec_pos->getValueLong ());
		return 0;
	}
	if (oldValue == dec_pos)
	{
		sitechMove (ra_pos->getValueLong (), newValue->getValueLong ());
		return 0;
	}

	return GEM::setValue (oldValue, newValue);
}

int Sitech::updateLimits ()
{
	acMin->setValueLong (-10000000);
	acMax->setValueLong (10000000);
	dcMin->setValueLong (-10000000);
	dcMax->setValueLong (10000000);
	return 0;
}

int Sitech::startPark ()
{
	return 0;
}

void Sitech::sitechMove (int32_t ac, int32_t dc)
{
	radec_request.y_dest = ac;
	radec_request.x_dest = dc;

	radec_request.y_speed = ra_speed->getValueLong ();
	radec_request.x_speed = dec_speed->getValueLong ();

	radec_request.y_rate_adder = ra_rate_adder->getValueLong ();
	radec_request.x_rate_adder = dec_rate_adder->getValueLong ();

	radec_request.y_rate_adder_t = ra_rate_adder_t->getValueLong ();
	radec_request.x_rate_adder_t = dec_rate_adder_t->getValueLong ();

	serConn->sendAxisRequest ('Y', radec_request);
}

int main (int argc, char **argv)
{	
	Sitech device = Sitech (argc, argv);
	return device.run ();
}
