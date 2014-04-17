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
 * Changes to the original driver (simplifications):
 * <ul>
 *  <li>removed temperature and barrometric pressure from refraction calculation, as those should be minor</li>
 *  <li>removed horizon/other checks, as those are handled by RTS2 core</li>
 *  <li>removed modelling (handled in RTS2 core)</li>
 * </ul>
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

		virtual int updateLimits ()
		{
			return 0;
		}

		/**
		 * Gets home offset.
		 */
		virtual int getHomeOffset (int32_t & off)
		{
			off = 0;
			return 0;
		}

	private:
		ConnSitech *serConn;

		SitechAxisStatus radec_status;

		rts2core::ValueLong *ra_pos;
		rts2core::ValueLong *dec_pos;

		rts2core::ValueLong *ra_enc;
		rts2core::ValueLong *dec_enc;

		rts2core::ValueLong *mclock;
		rts2core::ValueInteger *temperature;

		rts2core::ValueInteger *ra_worm_phase;

		rts2core::ValueLong *ra_last;
		rts2core::ValueLong *dec_last;

		double homera;                    /* Startup RA reset based on HA       */
		double homedec;                   /* Startup Dec                        */
		double offsetha;
		double offsetdec;
                 
		int pmodel;                    /* pointing model default to raw data */
	
		/* Contants */
		double mtrazmcal; 
		double mtraltcal;
		double mntazmcal;
		double mntaltcal;
	
		/* Global encoder counts */
		int mtrazm ;
		int mtralt ;
		int mntazm ;
		int mntalt ;

		/* Global pointing angles derived from the encoder counts */
		double mtrazmdeg;
		double mtraltdeg;
		double mntazmdeg;
		double mntaltdeg;
 
		/* Global tracking parameters */
		int azmtrackrate0;
		int alttrackrate0;
		int azmtracktarget;
		int azmtrackrate;
		int alttracktarget;
		int alttrackrate;
		
		/* Communications variables and routines for internal use */
		const char *device_file;

		void fullStop ();
		
		int SyncTelEncoders (void);

		/**
		 * Retrieve telescope counts, convert them to RA and Declination.
		 */
		void getTel (double &telra, double &teldec);

		int GoToCoords (double newra, double newdec);
		void GetGuideTargets (int *ntarget, int *starget, int *etarget, int *wtarget);
		int CheckGoTo (double newra, double newdec);
};

}

using namespace rts2teld;

Sitech::Sitech (int argc, char **argv):GEM (argc,argv), radec_status ()
{
	homera = 0.;                    /* Startup RA reset based on HA       */
	homedec = 0.;                   /* Startup Dec                        */
	offsetha=0.;
	offsetdec=0.;

	pmodel=RAW;
	mtrazmcal = MOTORAZMCOUNTPERDEG; 
	mtraltcal = MOTORALTCOUNTPERDEG;
	mntazmcal = MOUNTAZMCOUNTPERDEG;
	mntaltcal = MOUNTALTCOUNTPERDEG;


	/* Global encoder counts */
	mtrazm = 0;
	mtralt = 0;
	mntazm = 0;
	mntalt = 0;

	/* Global pointing angles derived from the encoder counts */

	mtrazmdeg = 0.;
	mtraltdeg = 0.;
	mntazmdeg = 0.;
	mntaltdeg = 0.;
 
	/* Global tracking parameters */
	azmtrackrate0   = 0;
	alttrackrate0   = 0;
	azmtracktarget  = 0;
	azmtrackrate    = 0;
	alttracktarget  = 0;
	alttrackrate    = 0;
	
	device_file = "/dev/ttyUSB0";

	createValue (ra_pos, "AXRA", "RA motor axis count", true);
	createValue (dec_pos, "AXDEC", "DEC motor axis count", true);

	createValue (ra_enc, "ENCRA", "RA encoder readout", true);
	createValue (dec_enc, "ENCDEC", "DEC encoder readout", true);

	createValue (mclock, "mclock", "millisecond board clocks", false);
	createValue (temperature, "temperature", "[C] board temperature (CPU)", false);
	createValue (ra_worm_phase, "y_worm_phase", "RA worm phase", false);
	createValue (ra_last, "ra_last", "RA motor location at last RA scope encoder location change", false);
	createValue (dec_last, "dec_last", "DEC motor location at last DEC scope encoder location change", false);

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

void Sitech::getTel (double &telra, double &teldec)
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

	double t_dec;

	int ret = counts2sky (radec_status.y_enc, radec_status.x_enc, telra, t_dec, teldec);
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
	char returnstr[256] = "";
	int numread;

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();

	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
	
	strcpy (telType, "Sitech");

	/* Software allows for different mount types but the A200HR requires "GEM" */
	/* GEM:    over the pier pointing at the pole                              */
	/* EQFORK: pointing at the equator on the meridian                         */
	/* ALTAZ:  level and pointing north                                        */

	fprintf (stderr, "On a cold start A200HR must be over the pier\n");
	fprintf (stderr, "  pointing at the pole.\n\n");
	fprintf (stderr, "We always read the precision encoder and \n"); 
	fprintf (stderr, "  set the motor encoders to match.\n");
	/* Make the connection */
	
	/* On the Sidereal Technologies controller                                */
	/*   there is an FTDI USB to serial converter that appears as             */
	/*   /dev/ttyUSB0 on Linux systems without other USB serial converters.   */
	/*   The serial device is known to the program that calls this procedure. */
	
	serConn = new ConnSitech (device_file, this);
	ret = serConn->init ();

	if (ret)
		return -1;
	serConn->flushPortIO ();
	
	
	/* Test connection by asking for firmware version    */
  	if (serConn->writePort ("XV\r", 3) == -1)
	    return -1;

	serConn->getSiTechValue ('X', "V");
     
	numread=serConn->readPort (returnstr,4);
	if (numread !=0 ) 
	{
		returnstr[numread] = '\0';
		fprintf (stderr, "Planewave A200HR\n");
		fprintf (stderr, "Sidereal Technology %s\n", returnstr);
		fprintf (stderr, "Telescope connected \n");  
	}
	else
	{
		logStream (MESSAGE_ERROR) << "A200HR drive control did not respond." << sendLog;
		return -1;
	}  

	/* Flush the input buffer */
	serConn->flushPortIO ();
  
	/* Set global tracking parameters */
          
	/* Set rates for defaults */
    
	azmtrackrate0 = AZMSIDEREALRATE;
	alttrackrate0 = ALTSIDEREALRATE;
	azmtrackrate = 0;
	alttrackrate = 0;
       
	/* Perform startup tests for slew limits if any */

	/*flag = SyncTelEncoders();
	if (flag != true)
	{
		logStream (MESSAGE_ERROR) << "Initial telescope pointing request was out of range ..." << sendLog;
		logStream (MESSAGE_ERROR) << "Cycle power, set telescope to home position, restart." << sendLog;
		return -1;
	} */
  
	/* Pause for telescope controller to initialize */
   
	usleep (500000);
  
	/* Read encoders and confirm pointing */
  
	getTel (homera, homedec);
  
	fprintf (stderr, "Mount motor encoder RA: %lf\n", homera);
	fprintf (stderr, "Mount motor encoder Dec: %lf\n", homedec);

	logStream (MESSAGE_INFO) << "The telescope is on line .." << sendLog;
    
	/* Flush the input buffer in case there is something left from startup */

	serConn->flushPortIO();
	return 0;

}

int Sitech::info ()
{
	double t_telRa, t_telDec;

	getTel (t_telRa, t_telDec);
	
	setTelRa (t_telRa);
	setTelDec (t_telDec);
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

int Sitech::initValues ()
{
	return Telescope::initValues ();
}

int Sitech::startResync ()
{
	CheckGoTo (getTelTargetRa (), getTelTargetDec ()); 
	return 0;
}

/* Test whether the destination was reached                  */
/* Return 0 if underway                                      */
/* Return 1 if done                                          */
/* Return 2 if outside tolerance                             */

int Sitech::CheckGoTo(double newra, double newdec)
{
	int status;
	double telra, teldec;
	double tolra, toldec;
	double raerr, decerr;

	status = GoToCoords(newra, newdec);

	if (status == 1)
	{
		/* Slew is in progress */
		return(0);
	}
	
	/* Get the telescope coordinates now */
	
	getTel(telra, teldec);
	
	/* Find pointing errors in seconds of arc for both axes	 */

	/* Allow for 24 hour ra wrap and trap dec at the celestial poles */
	
/*	raerr = 3600. * 15. * ln_range (telra - newra);
	if (fabs(teldec) < 89. )
	{
		decerr = 3600. * (teldec - newdec);
	}
	else
	{
		decerr = 0.;
	}
	
	tolra = SLEWTOLRA;
	toldec = SLEWTOLDEC; */

	if ((fabs(raerr) < tolra) && (fabs(decerr) < toldec))
	{
		logStream (MESSAGE_INFO) << "Goto completed within tolerance" << sendLog;
		return(1);
	}
	else
	{		
		logStream (MESSAGE_INFO) << "Goto completed outside tolerance" << sendLog;
		return(2);
	}	
}  

/* Go to new celestial coordinates                                            */
/* Based on CenterGuide algorithm rather than controller goto function        */
/* Evaluate if target coordinates are valid                                   */
/* Test slew limits in altitude, polar, and hour angles                       */
/* Query if target is above the horizon                                       */
/* Return without action for invalid requests                                 */
/* Interrupt any slew sequence in progress                                    */
/* Check current pointing                                                     */
/* Find encoder readings required to point at target                          */
/* Repeated calls are required when more than one segment is needed           */
int Sitech::GoToCoords (double newra, double newdec)
{
	return -1;
}
 
int Sitech::isMoving ()
{
	return -1;
}

int Sitech::startPark ()
{
	GoToCoords (parkPos->getAlt (), parkPos->getAz ());
	return 0;
}

int main (int argc, char **argv)
{	
	Sitech device = Sitech (argc, argv);
	return device.run ();
}
