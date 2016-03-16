#include "teld.h"
#include "configuration.h"
#include "conntcsng.h"

using namespace rts2teld;

/*moveState: 0=>Not moving no move called, 
                1=>move called but not moving, 2=>moving*/
#define TCSNG_NO_MOVE_CALLED   0
#define TCSNG_MOVE_CALLED      1
#define TCSNG_MOVING           2

/**
 * TCS telescope class to interact with TCSng control systems.
 *
 * author Scott Swindell
 */
class TCSNG:public Telescope
{
	public:
		TCSNG (int argc, char **argv);
		virtual ~TCSNG ();

	protected:
		virtual int processOption (int in_opt);

		virtual int initHardware ();

		virtual int info ();

		virtual int startResync ();

		virtual int startMoveFixed (double tar_az, double tar_alt)
		{
			return 0;
		}

		virtual int stopMove ()
		{
			
			//tel.comCANCEL();
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

	private:
		ConnTCSNG *ngconn;

		HostString *host;

		bool nillMove;
		rts2core::ValueSelection *tcsngmoveState;

		rts2core::ValueInteger *reqcount;
};

using namespace rts2teld;

const char *deg2hours (double h)
{
	static char rbuf[200];
	struct ln_hms hms;
	ln_deg_to_hms (h, &hms);

	snprintf (rbuf, 200, "%02d:%02d:%02.2f", hms.hours, hms.minutes, hms.seconds);
	return rbuf;
}

const char *deg2dec (double d)
{
	static char rbuf[200];
	struct ln_dms dms;
	ln_deg_to_dms (d, &dms);

	snprintf (rbuf, 200, "%c%02d:%02d:%02.2f", dms.neg ? '-':'+', dms.degrees, dms.minutes, dms.seconds);
	return rbuf;
}

TCSNG::TCSNG (int argc, char **argv):Telescope (argc,argv)
{
	
	/*RTS2 moveState is an rts2 data type that is alwasy 
	set equal to the tcsng.moveState member 
	this way moveState variable can be seen in rts2-mon*/
	createValue (tcsngmoveState, "tcsng_move_state", "TCSNG move state", false);
	tcsngmoveState->addSelVal ("NO MOVE");
	tcsngmoveState->addSelVal ("MOVE CALLED");
	tcsngmoveState->addSelVal ("MOVING");
	tcsngmoveState->setValueInteger (0);

	createValue (reqcount, "tcsng_req_count", "TCSNG request counter", false);
	reqcount->setValueInteger (TCSNG_NO_MOVE_CALLED);

	ngconn = NULL;
	host = NULL;

	addOption ('t', NULL, 1, "TCS NG hostname[:port]");
}

TCSNG::~TCSNG ()
{
	delete ngconn;
}

int TCSNG::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 't':
			host = new HostString (optarg, "5750");
			break;
		default:
			return Telescope::processOption (in_opt);
	}
	return 0;
}

int TCSNG::initHardware ()
{
	if (host == NULL)
	{
		logStream (MESSAGE_ERROR) << "You must specify TCS NG server hostname (with -t option)." << sendLog;
		return -1;
	}

	ngconn = new ConnTCSNG (this, host->getHostname (), host->getPort (), "TCSNG", "TCS");
	ngconn->setDebug (getDebug ());

	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telLongitude->setValueDouble (config->getObserver ()->lng);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());
			
	return Telescope::initHardware ();
}

int TCSNG::info ()
{
	setTelRaDec (ngconn->getSexadecimalHours ("RA"), ngconn->getSexadecimalAngle ("DEC"));
	double nglst = ngconn->getSexadecimalTime ("ST");

	reqcount->setValueInteger (ngconn->getReqCount ());

	return Telescope::infoLST (nglst);
}

int TCSNG::startResync ()
{
	struct ln_equ_posn tar;
	getTarget (&tar);

	char cmd[200];
	snprintf (cmd, 200, "NEXTPOS %s %s 2000 0 0", deg2hours (tar.ra), deg2dec (tar.dec));
	ngconn->command (cmd);
	ngconn->command ("MOVNEXT");

	tcsngmoveState->setValueInteger (TCSNG_MOVE_CALLED);
  	return 0;
}

int TCSNG::startPark ()
{
	ngconn->command ("MOVSTOW");
	return 0;
}

int TCSNG::isMoving ()
{
	int mot = atoi (ngconn->request ("MOTION"));
	switch (tcsngmoveState->getValueInteger ())
	{
		case TCSNG_MOVE_CALLED:
			if ((mot & TCSNG_RA_AZ) || (mot & TCSNG_DEC_AL) || (mot & TCSNG_DOME))
				tcsngmoveState->setValueInteger (TCSNG_MOVING);
			return USEC_SEC / 100;
			break;
		case TCSNG_MOVING:
			if ((mot & TCSNG_RA_AZ) || (mot & TCSNG_DEC_AL) || (mot & TCSNG_DOME))
				return USEC_SEC / 100;
			tcsngmoveState->setValueInteger (TCSNG_NO_MOVE_CALLED);
			return -2;
			break;
	}
	return -1;
	
}

int main (int argc, char **argv)
{
	TCSNG device (argc, argv);
	return device.run ();
}
