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

		virtual int startPark ()
		{
			//tel.comMOVSTOW();
			return 0;
		}

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
	reqcount->setValueInteger (0);

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
	double tra = ngconn->getSexadecimal ("RA", reqcount->getValueInteger ()) * 15.0;
	reqcount->inc ();

	double tdec = ngconn->getSexadecimal ("DEC", reqcount->getValueInteger ());
	reqcount->inc ();

	setTelRaDec (tra, tdec);

	return Telescope::info ();
}

int TCSNG::startResync ()
{
	struct ln_equ_posn tar;
	struct ln_hrz_posn hrz;

	double JD = ln_get_julian_from_sys ();
	
	getTarget (&tar);
	
  	return 0;
}

int TCSNG::isMoving ()
{	
	return -2;
	
}

int main (int argc, char **argv)
{
	TCSNG device (argc, argv);
	return device.run ();
}
