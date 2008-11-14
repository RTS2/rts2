#include "telescope.h"
#include "rts2toindi.h"

//static char lx200basic[]= "LX200 Basic" ;
static char lx200generic[]= "LX200 Generic" ;
static char *INDIdevice= lx200generic ;

class Rts2DevMyTelescope: public Rts2DevTelescope
{

	private:
		int tel_read_ra_dec ();
		int tel_read_geo() ;
		void set_move_timeout (time_t plus_time);
		int tel_check_coords (double ra, double dec);
		const char *indiserver;
		const char *indiport;
		int iport ;

		FILE *wfp ;
		FILE *rfp ;
		enum
		{ NOTMOVE, MOVE_REAL }
		move_state;
		time_t move_timeout;
		double lastMoveRa, lastMoveDec;

	protected:
		virtual int startMove ();
		virtual int isMoving ();
		virtual int stopMove ();
		virtual int startPark ();
		virtual int isParking ();
		virtual int endPark ();

	public:
		Rts2DevMyTelescope (int argc, char ** argv);
		virtual ~ Rts2DevMyTelescope (void);

		int tel_slew_to (double ra, double dec);
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int initValues ();
		virtual int info ();
};

int
Rts2DevMyTelescope::tel_read_geo ()
{
	float flng ;
	double lng;
	float flat ;
	double lat;
	float fhgt ;
	double hgt ;

	SearchDef *getINDI=NULL;
	int getINDIn=0 ;
	openINDIServer( indiserver, iport, &wfp, &rfp);

	rts2getINDI(INDIdevice, "defNumberVector", "GEOGRAPHIC_COORD", "*", &getINDIn, &getINDI, wfp, rfp) ;
	closeINDIServer(wfp, rfp);

	fprintf( stderr, "%s.%s.%s=%s, status %s\n", getINDI[0].d, getINDI[0].p, getINDI[0].gev[0].ge, getINDI[0].\
		gev[0].gv, getINDI[0].pstate) ;
	fprintf( stderr, "%s.%s.%s=%s, status %s\n", getINDI[0].d, getINDI[0].p, getINDI[0].gev[1].ge, getINDI[0].\
		gev[1].gv, getINDI[0].pstate) ;
	fprintf( stderr, "%s.%s.%s=%s, status %s\n", getINDI[0].d, getINDI[0].p, getINDI[0].gev[2].ge, getINDI[0].\
		gev[2].gv, getINDI[0].pstate) ;

	sscanf( getINDI[0].gev[0].gv, "%f", &flng) ;
	lng= (double) flng ;
	telLongitude->setValueDouble (lng);

	sscanf( getINDI[0].gev[1].gv, "%f", &flat) ;
	lat= (double) flat ;
	telLatitude->setValueDouble ( lat) ;

	sscanf( getINDI[0].gev[2].gv, "%f", &fhgt) ;
	hgt= (double) fhgt ;
	telAltitude->setValueDouble ( hgt) ;

	free_getINDIproperty(getINDI, getINDIn) ;

	return 0;
}


int
Rts2DevMyTelescope::tel_read_ra_dec ()
{
	float fra ;
	double ra;
	float fdec ;
	double dec;

	SearchDef *getINDI=NULL;
	int getINDIn=0 ;
	openINDIServer( indiserver, iport, &wfp, &rfp);

	rts2getINDI(INDIdevice, "defNumberVector", "EQUATORIAL_EOD_COORD", "*", &getINDIn, &getINDI, wfp, rfp) ;
	closeINDIServer(wfp, rfp);

	fprintf( stderr, "%s.%s.%s=%s, status %s\n", getINDI[0].d, getINDI[0].p, getINDI[0].gev[0].ge, getINDI[0].\
		gev[0].gv, getINDI[0].pstate) ;
	fprintf( stderr, "%s.%s.%s=%s, status %s\n", getINDI[0].d, getINDI[0].p, getINDI[0].gev[1].ge, getINDI[0].\
		gev[1].gv, getINDI[0].pstate) ;

	sscanf( getINDI[0].gev[0].gv, "%f", &fra) ;
	ra= (double) fra ;
	setTelRa (ra * 15.);

	sscanf( getINDI[0].gev[1].gv, "%f", &fdec) ;
	dec= (double) fdec ;
	setTelDec (dec);

	free_getINDIproperty(getINDI, getINDIn) ;

	return 0;
}


int
Rts2DevMyTelescope::tel_slew_to (double ra, double dec)
{
	fprintf( stderr, "Rts2DevMyTelescope::tel_slew_toIN\n") ;
	char cvalues[1024] ;

	openINDIServer( indiserver, iport, &wfp, &rfp);
	//lx 200 basic    rts2setINDI( INDIdevice, "defSwitchVector", "ON_COORD_SET", "SLEW;TRACK;SYNC", "On;Off;Off", wfp, rfp) ;
	// lx200generic
	rts2setINDI( INDIdevice, "defSwitchVector", "ON_COORD_SET", "TRACK;SYNC", "On;Off", wfp, rfp) ;
	//
	// Convert 0...360. degree to 0...24 decimal hours
	//
	sprintf( cvalues, "%10.7f;%10.7f", ra/15., dec) ;
	fprintf( stderr, "Rts2DevMyTelescope::tel_slew_to SEND  RA;DEC  %s\n", cvalues) ;

	rts2setINDI( INDIdevice, "defNumberVector", "EQUATORIAL_EOD_COORD_REQUEST", "RA;DEC",  cvalues, wfp, rfp) ;
	closeINDIServer(wfp, rfp) ;

	return 0;
}


int
Rts2DevMyTelescope::tel_check_coords (double ra, double dec)
{
	fprintf( stderr, "Rts2DevMyTelescope::tel_check_coords IN\n") ;
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
		fprintf( stderr, "Rts2DevTelescopeINDIap::tel_check_coords OUT timed out\n") ;
		return 2;
	}
	if (tel_read_ra_dec ())
	{
		fprintf( stderr, "Rts2DevTelescopeINDIap::tel_check_coords OUT bad status RA/DEC\n") ;
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
	fprintf( stderr, "Rts2DevTelescopeINDIap::tel_check_coords OUT bad status SEP target: %8.5f %8.5f, object: %8.5f %8.5f\n", target.ra, target.dec, object.ra, object.dec) ;
	return 1;
}


void
Rts2DevMyTelescope::set_move_timeout (time_t plus_time)
{
	fprintf( stderr, "Rts2DevTelescopeINDIap::set_move_timeout  IN\n") ;
	time_t now;
	time (&now);

	move_timeout = now + plus_time;
}


int
Rts2DevMyTelescope::startMove ()
{
	int ret ;

	ret = tel_slew_to (getTelRa (), getTelDec ());
	move_state = MOVE_REAL;
	set_move_timeout (100);
	lastMoveRa = getTelRa ();
	lastMoveDec = getTelDec ();

	return 0;
}


int
Rts2DevMyTelescope::isMoving ()
{
	int ret;
	fprintf( stderr, "Rts2DevMyTelescope::isMoving  IN\n") ;
	switch (move_state)
	{
		case MOVE_REAL:
			ret = tel_check_coords (lastMoveRa, lastMoveDec);
			switch (ret)
			{
				case -1:
					fprintf( stderr, "Rts2DevMyTelescope::isMoving  OUT MOVE_REAL CASE bad RA/DEC\n") ;
					return -1;
				case 0:
					fprintf( stderr, "Rts2DevMyTelescope::isMoving  OUT MOVE_REAL CASE target not yet reached\n") ;
					return USEC_SEC /10;
				case 1:
				case 2:
					fprintf( stderr, "Rts2DevMyTelescope::isMoving  OUT MOVE_REAL CASE target reached\n") ;
					move_state = NOTMOVE;
					return -2;
			}
			break;
		default:
			break;
	}
	fprintf( stderr, "Rts2DevMyTelescope::isMoving  OUT move_state\n") ;
	return -1;
}


int
Rts2DevMyTelescope::stopMove ()
{
	move_state = NOTMOVE ;
	return 0;					 // success
}


int
Rts2DevMyTelescope::startPark ()
{
	fprintf( stderr, "Rts2DevMyTelescope::Park    IN\n") ;

	return 0 ;
	// homeing in on RA=0 might be bad      return tel_slew_to (0, 0);
	// The AP 1200 GTO parks everywhere, do not forget to power cycle the mount
	openINDIServer( indiserver, iport, &wfp, &rfp);
	rts2setINDI( INDIdevice, "defSwitchVector", "CONNECTION", "CONNECT;DISCONNECT", "Off;On", wfp, rfp) ;
	closeINDIServer(wfp, rfp) ;

	return 0;
}


int
Rts2DevMyTelescope::isParking ()
{
	fprintf( stderr, "Rts2DevMyTelescope::isParking    IN\n") ;
	return -2;					 //destination was reached
}


int
Rts2DevMyTelescope::endPark ()
{
	return 0;
}


int
Rts2DevMyTelescope::info ()
{

	fprintf( stderr, "Rts2DevMyTelescope::info IN\n") ;
	if (tel_read_ra_dec ())
	{
		fprintf( stderr, "Rts2DevMyTelescope::info OUT    -1===================\n") ;
		return -1;
	}
	else
	{

		fprintf( stderr, "Rts2DevMyTelescope::info OUT\n") ;
	}

	return Rts2DevTelescope::info ();
}


Rts2DevMyTelescope::Rts2DevMyTelescope (int argc, char ** argv): Rts2DevTelescope (argc, argv)
{

	indiserver = "192.168.0.12";
	indiport= "7624" ;

	addOption ('u', "indiserver", 1, "host name or IP address of the INDI server, default: localhost");
	addOption ('v', "indiport"  , 1, "port at which the INDI server listens, default here 9624");

	move_state = NOTMOVE;
}


int
Rts2DevMyTelescope::init ()
{
	int status;
	fprintf( stderr, "Rts2DevMyTelescope::init IN\n") ;
	status = Rts2DevTelescope::init ();
	if (status)
	{
		fprintf( stderr, "Rts2DevMyTelescope::init OUT bad status %d===================\n", status) ;
		return status;
	}
	sscanf( indiport, "%d", &iport) ;
	openINDIServer( indiserver, iport, &wfp, &rfp);

	logStream (MESSAGE_DEBUG) << "Connection to AP  established to " << indiserver <<" on port "
		<< iport << sendLog;
	closeINDIServer(wfp, rfp) ;
	return 0 ;
}


int
Rts2DevMyTelescope::initValues ()
{

	set_move_timeout (100);

	fprintf( stderr, "Rts2DevMyTelescope::initValues IN\n") ;
	if (tel_read_geo ())
	{
		fprintf( stderr, "Rts2DevMyTelescope::initValues geo FAILED\n") ;
		return -1;
	}
	strcpy (telType, "LX200 Astro-Physics INDI");

	telFlip->setValueInteger (0);

	fprintf( stderr, "Rts2DevMyTelescope::initValues SUCCESS\n") ;

	return Rts2DevTelescope::initValues ();
}


Rts2DevMyTelescope::~Rts2DevMyTelescope (void)
{
	fclose (wfp);
}


int
Rts2DevMyTelescope::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'u':
			indiserver = optarg;
			break;
		case 'v':
			indiport = optarg;
			break;
		default:
			return Rts2DevTelescope::processOption (in_opt);
	}
	return 0;
}


int
main (int argc, char **argv)
{
	Rts2DevMyTelescope device = Rts2DevMyTelescope (argc, argv);
	return device.run ();
}
