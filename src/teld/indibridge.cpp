/*
 * Bridge driver for INDI standard compliant driven telescope.
 *
 * Copyright (C) 2008 Markus Wildi, Observatory Vermes
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

#include "teld.h"
#include "rts2toindi.h"

#define OPT_INDI_SERVER		OPT_LOCAL + 53
#define OPT_INDI_PORT		OPT_LOCAL + 54
#define OPT_INDI_DEVICE		OPT_LOCAL + 55

namespace rts2teld
{

class INDIBridge: public Telescope
{

	private:
		int tel_read_ra_dec ();
		int tel_read_geo() ;
		void set_move_timeout (time_t plus_time);
		int tel_check_coords (double ra, double dec);
		char indiserver[256];
		int indiport ;
                char indidevice[256];

		enum
		{ NOTMOVE, MOVE_REAL }
		move_state;
		time_t move_timeout;
		double lastMoveRa, lastMoveDec;

	protected:
		virtual int startResync ();
		virtual int isMoving ();
		virtual int stopMove ();
		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();

	public:
		INDIBridge (int argc, char ** argv);
		virtual ~ INDIBridge (void);

		int tel_slew_to (double ra, double dec);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();
};

};

using namespace rts2teld;

int
INDIBridge::tel_read_geo ()
{
	float flng ;
	double lng;
	float flat ;
	double lat;
	float fhgt ;
	double hgt ;

	SearchDef *getINDI=NULL;
	int getINDIn=0 ;
// This first call is necessary to receive further updates from the INDI server
	rts2getINDI(indidevice, "defNumberVector", "GEOGRAPHIC_COORD", "*", &getINDIn, &getINDI) ;

 	sscanf( getINDI[0].gev[0].gv, "%f", &flng) ;
 	lng= (double) flng ;
 	telLongitude->setValueDouble (lng);

 	sscanf( getINDI[0].gev[1].gv, "%f", &flat) ;
 	lat= (double) flat ;
 	telLatitude->setValueDouble ( lat) ;

 	sscanf( getINDI[0].gev[2].gv, "%f", &fhgt) ;
 	hgt= (double) fhgt ;
 	telAltitude->setValueDouble ( hgt) ;

	rts2free_getINDIproperty(getINDI, getINDIn) ;

	return 0;
}

int
INDIBridge::tel_read_ra_dec ()
{
	setTelRa (indi_ra * 15.);
	setTelDec( indi_dec) ;

	return 0;
}

int
INDIBridge::tel_slew_to (double ra, double dec)
{
    //fprintf( stderr, "INDIBridge::tel_slew_toIN\n") ;
	char cvalues[1024] ;

	rts2setINDI( indidevice, "defSwitchVector", "ON_COORD_SET", "SLEW;SYNC", "On;Off") ;
	//
	// Convert 0...360. degree to 0...24 decimal hours
	//
	sprintf( cvalues, "%10.7f;%10.7f", ra/15., dec) ;
	//fprintf( stderr, "INDIBridge::tel_slew_to SEND  RA;DEC  %s\n", cvalues) ;

	rts2setINDI( indidevice, "defNumberVector", "EQUATORIAL_EOD_COORD_REQUEST", "RA;DEC",  cvalues) ;

	return 0;
}

int
INDIBridge::tel_check_coords (double ra, double dec)
{
	// ADDED BY JF
	double JD;

	double sep;
	time_t now;

	struct ln_equ_posn object, target;
	struct ln_lnlat_posn observer;
	struct ln_hrz_posn hrz;

	time (&now);
	if (now > move_timeout)
	{
	    //fprintf( stderr, "TelescopeINDIap::tel_check_coords OUT timed out\n") ;
		return 2;
	}
	if (tel_read_ra_dec ())
	{
	    //fprintf( stderr, "TelescopeINDIap::tel_check_coords OUT bad status RA/DEC\n") ;
		return -1;
	}
	// ADDED BY JF
	// CALCULATE & PRINT ALT/AZ & HOUR ANGLE TO LOG
	object.ra = getTelRa ();
	object.dec = getTelDec ();

	observer.lng = telLongitude->getValueDouble ();
	observer.lat = telLatitude->getValueDouble ();

	JD = ln_get_julian_from_sys ();

	ln_get_hrz_from_equ (&object, &observer, JD, &hrz);

	//wildilogStream (MESSAGE_DEBUG) << "LX200 tel_check_coords TELESCOPE ALT " << hrz.
	//	alt << " AZ " << hrz.az << sendLog;

	target.ra = ra;
	target.dec = dec;

	sep = ln_get_angular_separation (&object, &target);

	if (sep > 0.1)
		return 0;
	//fprintf( stderr, "TelescopeINDIap::tel_check_coords OUT status SEP target: %8.5f %8.5f, object: %8.5f %8.5f\n", target.ra, target.dec, object.ra, object.dec) ;
	return 1;
}


void
INDIBridge::set_move_timeout (time_t plus_time)
{
	time_t now;
	time (&now);

	move_timeout = now + plus_time;
}


int
INDIBridge::startResync ()
{
	tel_slew_to (getTelTargetRa (), getTelTargetDec ());
	move_state = MOVE_REAL;
	set_move_timeout (100);
	lastMoveRa = getTelTargetRa ();
	lastMoveDec = getTelTargetDec ();

	return 0;
}

int
INDIBridge::isMoving ()
{
	int ret;
	switch (move_state)
	{
		case MOVE_REAL:
			ret = tel_check_coords (lastMoveRa, lastMoveDec);
			switch (ret)
			{
				case -1:
					return -1;
				case 0:
					return USEC_SEC /10;
				case 1:
			        case 2:
				    //fprintf( stderr, "INDIBridge::isMoving arget reached\n") ;
					move_state = NOTMOVE;
					return -2;
			}
			break;
		default:
			break;
	}
	return -1;
}


int
INDIBridge::stopMove ()
{
	move_state = NOTMOVE ;
	return 0; // success
}


int
INDIBridge::startPark ()
{
	return 0;
}


int
INDIBridge::isParking ()
{
	return -2; //destination was reached
}

int
INDIBridge::endPark ()
{
	return 0;
}

int
INDIBridge::info ()
{
	if (tel_read_ra_dec ())
	{
		return -1;
	}
	return Telescope::info ();
}


INDIBridge::INDIBridge (int argc, char ** argv): Telescope (argc, argv)
{
        strcpy( indiserver, "127.0.0.1");
        indiport= 7624 ;
	strcpy( indidevice, "LX200 Astro-Physics");

	addOption (OPT_INDI_SERVER, "indiserver", 1, "host name or IP address of the INDI server, default: 127.0.0.1");
	addOption (OPT_INDI_PORT, "indiport"  , 1, "port at which the INDI server listens, default here 7624");
	addOption (OPT_INDI_DEVICE, "indidevice", 1, "name of the INDI device");

	move_state = NOTMOVE;
}


int
INDIBridge::init ()
{
	int status;

	//fprintf( stderr, "INDIBridge::init IN, %s, %d, %s\n", indiserver, indiport, indidevice) ;
	status = Telescope::init ();
	if (status)
	{
	    //fprintf( stderr, "INDIBridge::init OUT bad status %d===================\n", status) ;
		return status;
	}
        
	rts2openINDIServer( indiserver, indiport);

	logStream (MESSAGE_DEBUG) << "Connection to AP  established to " << indiserver <<" on port "
		<< indiport << sendLog;

	return 0 ;
}


int
INDIBridge::initValues ()
{
	pthread_t  thread_0;

	indi_ra =0. ;
        indi_dec= 0. ;
	set_move_timeout (100);

	if (tel_read_geo ())
	{
	    logStream (MESSAGE_DEBUG) << "Connection to INDI  server" << indiserver <<" on port "
				      << indiport << "failed" << sendLog;
	    return -1;
	}
	strcpy (telType, "LX200 INDI bridge");

	telFlip->setValueInteger (0);

	pthread_create( &thread_0, NULL, rts2listenINDIthread, indidevice) ;
	return Telescope::initValues ();
}


INDIBridge::~INDIBridge (void)
{
    rts2closeINDIServer() ;
}


int
INDIBridge::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_INDI_SERVER:
		        strcpy( indiserver, optarg) ;
			break;
		case OPT_INDI_PORT:
		        indiport = atoi(optarg);
			break;
		case OPT_INDI_DEVICE:
			strcpy( indidevice, optarg) ;
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}


int
main (int argc, char **argv)
{
	INDIBridge device = INDIBridge (argc, argv);
	return device.run ();
}
