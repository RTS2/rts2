/* 
 * Driver for Ondrejov, Astrolab D50 scope.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
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

//#define DEBUG_EXTRA

#include "connection/tgdrive.h"
#include "connection/remotes.h"
#include "fork.h"

#include "configuration.h"
#include "libnova_cpp.h"


#define OPT_RA                OPT_LOCAL + 2201
#define OPT_DEC               OPT_LOCAL + 2202

#define RTS2_D50_TIMERRG    RTS2_LOCAL_EVENT + 1210
#define RTS2_D50_TIMERDG    RTS2_LOCAL_EVENT + 1211
#define RTS2_D50_TSTOPRG    RTS2_LOCAL_EVENT + 1212
#define RTS2_D50_TSTOPDG    RTS2_LOCAL_EVENT + 1213

#define RTS2_D50_AUTOSAVE   RTS2_LOCAL_EVENT + 1214
#define RTS2_D50_BOOSTSPEED   RTS2_LOCAL_EVENT + 1215

// steps per full RA and DEC revolutions (360 degrees)
//#define RA_TICKS                 (-14350 * 65535)
//#define DEC_TICKS                (-10400 * 65535)
// config pro D50:
#define RA_TRANSMISION		2304
#define DEC_TRANSMISION		2000
#define RA_TICKS		(-2304 * 65536)
#define DEC_TICKS		(2000 * 65536)

#define RAGSTEP			1000
#define DEGSTEP			1000

// track is 15 arcsec/second
//#define TRACK_SPEED              (-TGA_SPEEDFACTOR / 6.0)
// 1/360/60/4*2304 = 0.026666666666666657
#define TRACK_SPEED              (-TGA_SPEEDFACTOR * 0.026666666666666657)
// track one arcdeg in second
#define SPEED_ARCDEGSEC          (TRACK_SPEED * (4.0 * 60.0))

#define REMOTES_REG_MC_POWERSUPPLYENABLED	0x80
#define REMOTES_REG_MC_DRIVEOUTPUTENABLED	0x40
#define REMOTES_REG_MC_PRESSURELIMITENABLED	0x20
#define REMOTES_REG_MC_STEPPERSIGNALENABLED	0x02
#define REMOTES_REG_MC_CLOCKWISEDIRECTIONENABLED	0x01


namespace rts2teld
{

class D50:public Fork
{
	public:
		D50 (int in_argc, char **in_argv);
		virtual ~ D50 (void);
		
		virtual int scriptEnds ();
		virtual void postEvent (rts2core::Event *event);
		virtual int commandAuthorized (rts2core::Connection * conn);
	protected:
		virtual void usage ();
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual int info ();
		virtual int idle ();

		virtual int resetMount ();

		virtual int startResync ();
		virtual int isMoving ();
		virtual int endMove ();
		virtual int stopMove ();
		virtual int setTo (double set_ra, double set_dec);
		virtual int setToPark ();
		virtual int startPark ();
		virtual int isParking () { return isMoving (); };
		virtual int endPark ();

		virtual void setDiffTrack (double dra, double ddec);

		virtual int updateLimits ();
		virtual int getHomeOffset (int32_t & off);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
	private:
		int runD50 ();

		TGDrive *raDrive;
		TGDrive *decDrive;

		rts2core::ConnREMOTES *remotesRA;
		rts2core::ConnREMOTES *remotesDec;
		unsigned char remotesMCRegisterRAState;
		unsigned char remotesMCRegisterDecState;

		int32_t tAc;

		const char *devRA;
		const char *devDEC;

		rts2core::ValueDouble *moveTolerance;

		rts2core::ValueDouble *moveSpeedBacklash;
		rts2core::ValueDouble *moveSpeedBacklashInterval;
		rts2core::ValueDouble *moveSpeedFull;
		rts2core::ValueDouble *moveSpeedTurbo;
		rts2core::ValueBool *moveTurboSwitch;

		//void matchGuideRa (int rag);
		//void matchGuideDec (int decg);

		void callAutosave ();
		bool parking;

		// low-level functions for communication with REMOTES
		int remotesGetMCRegister (rts2core::ConnREMOTES * connRem, unsigned char * mcReg);
		int remotesSetMCRegister (rts2core::ConnREMOTES * connRem, unsigned char mcReg);
		int remotesGetStepperPulseLength (rts2core::ConnREMOTES * connRem, unsigned short * pulseLength);
		int remotesSetStepperPulseLength (rts2core::ConnREMOTES * connRem, unsigned short pulseLength);
		int remotesGetAbsolutePosition (rts2core::ConnREMOTES * connRem, unsigned short * absPos);

		// rts2 variables mirroring actual state
		rts2core::ValueBool *remotesMotorsPower;
		rts2core::ValueBool *remotesMotorsExternalEnable;
		rts2core::ValueBool *remotesWormPressureLimiter;
		rts2core::ValueBool *remotesWormStepsGenerator;
		rts2core::ValueDouble *remotesWormStepsFreqTarget;
		rts2core::ValueDouble *remotesWormStepsFreqReal;
		rts2core::ValueInteger *remotesWormStepsPulseLength;
		rts2core::ValueInteger *remotesAbsEncRA;
		rts2core::ValueInteger *remotesAbsEncDec;
		//rts2core::ValueInteger *remotesIncEncRA;
		//rts2core::ValueInteger *remotesIncEncDec;

		// high-level functions for managing the REMOTES things
		int setMotorsPower (bool power);
		int setMotorsExternalEnable (bool extEnable);
		int setWormPressureLimiter (bool pressureLimiter);
		int setWormStepsGenerator (bool stepsGenerator);
		int setWormStepsFreq (double freq);
		int updateWormStepsFreq (bool updateAlsoTargetFreq = false);

};

};

using namespace rts2teld;



D50::D50 (int in_argc, char **in_argv):Fork (in_argc, in_argv, true, true)
{
	raDrive = NULL;
	decDrive = NULL;
	
	devRA = NULL;
	devDEC = NULL;
	
	ra_ticks = labs ((int32_t) (RA_TICKS));
	dec_ticks = labs ((int32_t) (DEC_TICKS));
	
	haCpd = (int32_t) (RA_TICKS) / 360.0;
	decCpd = (int32_t) (DEC_TICKS) / 360.0;
	
	acMargin = (int32_t) (haCpd * 5);
	
	haZero = decZero = 0.0;
	
	parking = false;
	
	createRaPAN ();
	createDecPAN ();
	
	createValue (moveSpeedBacklash, "speed_backlash", "[deg/s] initial speed to cross backlash", false, RTS2_VALUE_WRITABLE);
	moveSpeedBacklash->setValueDouble (0.3);
	
	createValue (moveSpeedBacklashInterval, "speed_backlash_interval", "[s] interval of keeping initial speed to cross backlash", false, RTS2_VALUE_WRITABLE);
	moveSpeedBacklashInterval->setValueDouble (1.5);

	createValue (moveSpeedFull, "speed_full", "[deg/s] standard maximal slew speed", false, RTS2_VALUE_WRITABLE);
	moveSpeedFull->setValueDouble (1.5);

	createValue (moveSpeedTurbo, "speed_turbo", "[deg/s] turbo maximal slew speed", false, RTS2_VALUE_WRITABLE);
	moveSpeedTurbo->setValueDouble (5.0);

	createValue (moveTurboSwitch, "speed_turbo_switch", "turbo slew speed switch", false, RTS2_VALUE_WRITABLE);
	moveTurboSwitch->setValueBool (false);

	createValue (moveTolerance, "move_tolerance", "[deg] minimal movement distance", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	moveTolerance->setValueDouble (4.0 / 60.0);
	
	addOption (OPT_RA, "ra", 1, "RA drive serial device");
	addOption (OPT_DEC, "dec", 1, "DEC drive serial device");

	addParkPosOption ();

	createValue (remotesMotorsPower, "remotes_motors_power", "el. power to both motors", false);
	remotesMotorsPower->setValueBool (true);  // tady pozor, vypnuti serv zpusobi ztratu polohy, radeji prozatim nastavuji jako RO, nez se to nejak dale osetri...
	createValue (remotesMotorsExternalEnable, "remotes_motors_external_enable", "external enable switch to both motors", false, RTS2_VALUE_WRITABLE);
	remotesMotorsExternalEnable->setValueBool (true);
	createValue (remotesWormPressureLimiter, "remotes_worm_pressure_limiter", "decrease pressure on worm (for slew)", false, RTS2_VALUE_WRITABLE);
	remotesWormPressureLimiter->setValueBool (false);
	createValue (remotesWormStepsGenerator, "remotes_worm_steps_generator", "generator of steps for HA tracking", false, RTS2_VALUE_WRITABLE);
	remotesWormStepsGenerator->setValueBool (false);
	createValue (remotesWormStepsFreqTarget, "remotes_worm_steps_freq_tar", "target frequency of HA tracking steps generator [Hz]", false, RTS2_VALUE_WRITABLE);
	remotesWormStepsFreqTarget->setValueDouble (1752.41140771);	// computed default value
	createValue (remotesWormStepsFreqReal, "remotes_worm_steps_freq", "actual frequency of HA tracking steps generator [Hz]", false);
	createValue (remotesWormStepsPulseLength, "remotes_worm_steps_pulse_length", "pulse length of HA tracking steps generator", false);
	createValue (remotesAbsEncRA, "remotes_abs_encoder_ra", "raw abs. encoder position, RA axis", false);
	createValue (remotesAbsEncDec, "remotes_abs_encoder_dec", "raw abs. encoder position, Dec axis", false);

#ifndef RTS2_LIBERFA
	// apply all corrections by rts2 for mount
	setCorrections (true, true, true, true);
#endif
}

D50::~D50 (void)
{
	delete raDrive;
	delete decDrive;
}

void D50::postEvent (rts2core::Event *event)
{
	logStream (MESSAGE_DEBUG) << "****** postEvent(): " << event->getType () << "." << sendLog;
        switch (event->getType ())
        {
                case RTS2_D50_TIMERRG:
                        //matchGuideRa (raGuide->getValueInteger ());
                        break;
                /*case RTS2_D50_TIMERDG:
                        matchGuideDec (decGuide->getValueInteger ());
                        break;*/
                case RTS2_D50_TSTOPRG:
                        if (raDrive->checkStop ())
                        {
                                //addTimer (0.1, event);
                                return;
                        }
                        break;
                case RTS2_D50_TSTOPDG:
                        if (decDrive->checkStop ())
                        {
                                //addTimer (0.1, event);
                                return;
                        }
                        break;
                case RTS2_D50_AUTOSAVE:
                        callAutosave ();
                        if ((getState () & TEL_MASK_MOVING) == TEL_OBSERVING)
                        {
				addTimer (5, event);
				return;
                        }
                        break;
                case RTS2_D50_BOOSTSPEED:
                        if (getState () & TEL_MOVING || getState () & TEL_PARKING)
                        {
                        	// increase speed after backlash overcome, depends on actual state
				if (moveTurboSwitch->getValueBool ())
				{
					raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedTurbo->getValueDouble ());
					decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedTurbo->getValueDouble ());
				}
				else
				{
					raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedFull->getValueDouble ());
					decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedFull->getValueDouble ());
				}
                        }
                        break;
        }
        Fork::postEvent (event);
}

int D50::commandAuthorized (rts2core::Connection * conn)
{
	logStream (MESSAGE_DEBUG) << "****** commandAuthorized (): " << conn->getCommand () << sendLog;
        if (conn->isCommand ("writeeeprom"))
        {
                raDrive->write2b (TGA_MASTER_CMD, 3);
                decDrive->write2b (TGA_MASTER_CMD, 3);
                return 0;
        }
        return Fork::commandAuthorized (conn);
}

void D50::usage ()
{
        std::cout << "\t" << getAppName () << " --ra /dev/ttyS0 --dec /dev/ttyS1" << std::endl;
}

int D50::processOption (int opt)
{
	logStream (MESSAGE_DEBUG) << "****** processOption ()" << sendLog;
        switch (opt)
        {
                case OPT_RA:
                        devRA = optarg;
                        break;
                case OPT_DEC:
                        devDEC = optarg;
                        break;
                default:
                        return Fork::processOption (opt);
        }
        return 0;
}

int D50::init ()
{
	int ret;
	unsigned char remotesMacRA[6] = {0x00, 0x04, 0xa3, 0x12, 0x34, 0x56};
	unsigned char remotesMacDec[6] = {0x00, 0x04, 0xa3, 0x12, 0x34, 0x55};
	char remotesEthRA [] = "eth1";
	char remotesEthDec [] = "eth1";
	unsigned short us;

	ret = Fork::init ();
	if (ret)
		return ret;

	// initialize REMOTES units first...
	remotesRA = new rts2core::ConnREMOTES (this, remotesEthRA, remotesMacRA);
	//remotesRA->setDebug (getDebug ());
	remotesRA->setDebug (true);
	remotesRA->init ();
	remotesMCRegisterRAState = REMOTES_REG_MC_POWERSUPPLYENABLED | REMOTES_REG_MC_DRIVEOUTPUTENABLED | REMOTES_REG_MC_CLOCKWISEDIRECTIONENABLED;
	ret = remotesSetMCRegister (remotesRA, remotesMCRegisterRAState);
        if (ret)
                return ret;

	remotesDec = new rts2core::ConnREMOTES (this, remotesEthDec, remotesMacDec);
	//remotesDec->setDebug (getDebug ());
	remotesDec->setDebug (true);
	remotesDec->init ();
	remotesMCRegisterDecState = REMOTES_REG_MC_POWERSUPPLYENABLED | REMOTES_REG_MC_DRIVEOUTPUTENABLED;
	ret = remotesSetMCRegister (remotesDec, remotesMCRegisterDecState);
        if (ret)
                return ret;


	// then servo controllers...
        if (devRA == NULL)
        {
                logStream (MESSAGE_ERROR) << "RA device file was not specified." << sendLog;
                return -1;
        }

        if (devDEC == NULL)
        {
                logStream (MESSAGE_ERROR) << "DEC device file was not specified." << sendLog;
                return -1;
        }

        raDrive = new TGDrive (devRA, "RA_", this);
        raDrive->setDebug (getDebug ());
        raDrive->setLogAsHex ();
        ret = raDrive->init ();
        if (ret)
                return ret;

        decDrive = new TGDrive (devDEC, "DEC_", this);
        decDrive->setDebug (getDebug ());
        decDrive->setLogAsHex ();
        ret = decDrive->init ();
        if (ret)
                return ret;

	// for special cases of persisting movement, try to gently stop motors first...
	raDrive->stop ();
	decDrive->stop ();
	while (raDrive->checkStopPlain () || decDrive->checkStopPlain ())
	{
		usleep (USEC_SEC / 5);
	}

	setWormStepsFreq (remotesWormStepsFreqTarget->getValueDouble ());
	remotesGetAbsolutePosition (remotesRA, &us);
	remotesAbsEncRA->setValueInteger (us);
	remotesGetAbsolutePosition (remotesDec, &us);
	remotesAbsEncDec->setValueInteger (us);


	// and finally, the remaining usual teld setup...
	rts2core::Configuration *config = rts2core::Configuration::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	setTelLongLat (config->getObserver ()->lng, config->getObserver ()->lat);
	setTelAltitude (config->getObservatoryAltitude ());

	// zero dec is on local meridian, 90 - telLatitude bellow (to nadir)
	decZero = - (90.0 - fabs (telLatitude->getValueDouble ()));

	if (telLatitude->getValueDouble () < 0)
	{
		decZero *= -1.0;
		// swap values which are opposite for south hemispehere
	}

	// TODO: mozna jeste poresit i uplne vypnuti serv? A take pridat check, jestli aktualni poloha ze serv odpovida te z cidel...

	//addTimer (1, new rts2core::Event (RTS2_D50_AUTOSAVE));

	return 0;
}

int D50::info ()
{
	unsigned short us;

	logStream (MESSAGE_DEBUG) << "****** info ()" << sendLog;
        raDrive->info ();
        decDrive->info ();

	remotesGetMCRegister (remotesRA, &remotesMCRegisterRAState);
	remotesGetMCRegister (remotesDec, &remotesMCRegisterDecState);
	remotesWormPressureLimiter->setValueBool ((remotesMCRegisterRAState & REMOTES_REG_MC_PRESSURELIMITENABLED) == 0x00 ? false : true);
	remotesWormStepsGenerator->setValueBool ((remotesMCRegisterRAState & REMOTES_REG_MC_STEPPERSIGNALENABLED) == 0x00 ? false : true);
	updateWormStepsFreq ();

	remotesGetAbsolutePosition (remotesRA, &us);
	remotesAbsEncRA->setValueInteger (us);
	//logStream (MESSAGE_DEBUG) << "RA position: " << us << " " << std::hex << us << sendLog;
	remotesGetAbsolutePosition (remotesDec, &us);
	remotesAbsEncDec->setValueInteger (us);

        double t_telRa;
        double t_telDec;
	int t_telFlip;
	double ut_telRa;
	double ut_telDec;
        int32_t raPos = raDrive->getPosition ();
        int32_t decPos = decDrive->getPosition ();
	counts2sky (raPos, decPos, t_telRa, t_telDec, t_telFlip, ut_telRa, ut_telDec);
	setTelRaDec (t_telRa, t_telDec);
	telFlip->setValueInteger (t_telFlip);
	setTelUnRaDec (ut_telRa, ut_telDec);

	return Fork::info ();
}

int D50::idle ()
{
	logStream (MESSAGE_DEBUG) << "****** D50:idle ()" << sendLog;
	runD50 ();

	return Fork::idle ();
}

int D50::runD50 ()
{
	logStream (MESSAGE_DEBUG) << "****** D50:runD50 ()" << sendLog;
	if (!(getState () & TEL_MOVING))
	{
		if ((trackingRequested () == 0) && raDrive->isInStepperMode ())
		{
			logStream (MESSAGE_DEBUG) << "****** runD50 () - setting tracking to off - DS mode" << sendLog;
                        raDrive->setTargetSpeed ( 0, true );
                        raDrive->setMode (TGA_MODE_DS);
			setWormStepsGenerator (false);
                        //raDrive->info ();
		} else if (isTracking () == true && !raDrive->isInStepperMode ()) 
		{
			logStream (MESSAGE_DEBUG) << "****** runD50 () - setting tracking to on - SM mode" << sendLog;
			if (!raDrive->isMoving ())
			{
				setWormStepsGenerator (true);
				raDrive->setMode (TGA_MODE_SM);
                        	//raDrive->info ();
			}
			else
			{
				logStream (MESSAGE_ERROR) << "not switching to stepper mode, raDrive seems to be moving?!?" << sendLog;
                        	raDrive->info ();
			}
		}
	}

	return 0;
}

int D50::resetMount ()
{
	logStream (MESSAGE_DEBUG) << "****** resetMount ()" << sendLog;
        try
        {
                raDrive->reset ();
                decDrive->reset ();
        }
        catch (TGDriveError e)
        {
                logStream (MESSAGE_ERROR) << "error reseting mount" << sendLog;
                return -1;
        }

        return Fork::resetMount ();
}

int D50::startResync ()
{
	logStream (MESSAGE_DEBUG) << "****** startResync ()" << sendLog;
        //deleteTimers (RTS2_D50_AUTOSAVE);
        int32_t dc;
        int ret = sky2counts (tAc, dc);
        if (ret)
                return -1;
	// this maight be redundant local check, we do it to be sure not to make flip and harm ourselfs...
	if (dc < dcMin || dc > dcMax)
	{
		logStream (MESSAGE_ERROR) << "dc value out of limits!" << sendLog;
		return -1;
	}

	setWormPressureLimiter (true);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	addTimer (moveSpeedBacklashInterval->getValueDouble (), new rts2core::Event (RTS2_D50_BOOSTSPEED));
        raDrive->setTargetPos (tAc);
        decDrive->setTargetPos (dc);
        return 0;
}

int D50::isMoving ()
{
	logStream (MESSAGE_DEBUG) << "****** isMoving ()" << sendLog;
        //callAutosave ();
	logStream (MESSAGE_DEBUG) << "------ isMoving: getState=" << getState () << ", tracking=" << trackingRequested () << ", parking=" << parking << ", raDrive->isMoving=" << raDrive->isMoving () << ", raDrive->isInPositionMode=" << raDrive->isInPositionMode () << ", decDrive->isMoving=" << decDrive->isMoving () << sendLog;
        if ((trackingRequested () != 0) && raDrive->isInPositionMode ())
        {
                if (raDrive->isMovingPosition ())
                {
			logStream (MESSAGE_DEBUG) << "------ updating ac position!" << sendLog;
                        int32_t diffAc;
                        int32_t ac;
                        int32_t dc;
                        int ret = sky2counts (ac, dc);
                        if (ret)
                                return -1;
                        diffAc = ac - tAc;
                        // if difference in HA is greater than tolerance...
                        if (fabs (diffAc) > haCpd * moveTolerance->getValueDouble ())
                        {
                                raDrive->setTargetPos (ac);
                                tAc = ac;
				return USEC_SEC / 10;
                        }
                }
                else
                {
                        //raDrive->setTargetSpeed (TRACK_SPEED);
			setWormStepsGenerator (true);
                        raDrive->setMode (TGA_MODE_SM);
                        //raDrive->info ();
                }
        }
        if (((trackingRequested () != 0) && raDrive->isInPositionMode ()) || ((trackingRequested () == 0) && raDrive->isMoving ()) || decDrive->isMoving ())
		return USEC_SEC / 10;
        return -2;
}

int D50::endMove ()
{
	logStream (MESSAGE_DEBUG) << "****** endMove ()" << sendLog;
	//addTimer (5, new rts2core::Event (RTS2_D50_AUTOSAVE));
	setWormPressureLimiter (false);
	// snizeni rychlosti presunu obou motoru
	deleteTimers (RTS2_D50_BOOSTSPEED);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	setTimeout (USEC_SEC);
	//TODO: pridat check, jestli informace z ABS (a inc?) cidel odpovida poloze!
	return Fork::endMove ();
}

int D50::stopMove ()
{
	logStream (MESSAGE_DEBUG) << "****** stopMove ()" << sendLog;
        //addTimer (5, new rts2core::Event (RTS2_D50_AUTOSAVE));
	if (raDrive->isMoving () || decDrive->isMoving ())	// otherwise it's only about cleaning the TEL_MOVING state, we don't want to clean parking flag and other things then
	{
        	parking = false;
        	raDrive->stop ();
        	decDrive->stop ();
		while (raDrive->checkStopPlain () || decDrive->checkStopPlain ())
		{
			usleep (USEC_SEC / 5);
		}
	}
	setWormPressureLimiter (false);
	deleteTimers (RTS2_D50_BOOSTSPEED);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
        return 0;
}

int D50::setTo (double set_ra, double set_dec)
{
	logStream (MESSAGE_DEBUG) << "****** setTo ()" << sendLog;
        struct ln_equ_posn eq;
        eq.ra = set_ra;
        eq.dec = set_dec;
        int32_t ac;
        int32_t dc;
        int32_t off;
	logStream (MESSAGE_DEBUG) << "------ setting coordinates to RA: " << set_ra << ", DEC: " << set_dec << sendLog;
        getHomeOffset (off);
	zeroCorrRaDec ();
        int ret = sky2counts (&eq, ac, dc, ln_get_julian_from_sys ());
        if (ret)
                return -1;

	logStream (MESSAGE_DEBUG) << "------ setting coordinates to ac: " << ac << ", dc: " << dc << sendLog;
        raDrive->setCurrentPos (ac);
        decDrive->setCurrentPos (dc);
        //callAutosave ();
        if (isTracking ())
	{
		setWormStepsGenerator (true);
                raDrive->setMode (TGA_MODE_SM);
                //raDrive->info ();
	}
                //raDrive->setTargetSpeed (TRACK_SPEED);
        return 0;
}

int D50::setToPark ()
{
	logStream (MESSAGE_DEBUG) << "****** setToPark ()" << sendLog;
        if (parkPos == NULL)
                return -1;

        struct ln_equ_posn epark;
        struct ln_hrz_posn park;
        struct ln_lnlat_posn observer;

        parkPos->getAltAz (&park);

        observer.lng = telLongitude->getValueDouble ();
        observer.lat = telLatitude->getValueDouble ();

        ln_get_equ_from_hrz (&park, &observer, ln_get_julian_from_sys (), &epark);

        return setTo (epark.ra, epark.dec);
}

int D50::startPark ()
{
	logStream (MESSAGE_DEBUG) << "****** startPark ()" << sendLog;
        if (parkPos)
        {
                parking = true;
		// to park to real "zero" position, switch off corrections&modelling, and put it back again after move command
#ifndef RTS2_LIBERFA
		bool calAber = calculateAberation ();
		bool calNut = calculateNutation ();
		bool calPrec = calculatePrecession ();
		bool calRef = calculateRefraction ();
		setCorrections (false, false, false, false);
#endif
		bool calMod = isModelOn ();
		modelOff ();
                setTargetAltAz (parkPos->getAlt (), parkPos->getAz ());
                int ret = moveAltAz ();
#ifndef RTS2_LIBERFA
		setCorrections (calAber, calNut, calPrec, calRef);
#endif
		if (calMod)
			 modelOn ();
                return ret ? -1 : 1;
        }
        else
                return -1;
}

int D50::endPark ()
{
	logStream (MESSAGE_DEBUG) << "****** endPark ()" << sendLog;
        //callAutosave ();
        parking = false;
	setWormPressureLimiter (false);
	setWormStepsGenerator (false);
	deleteTimers (RTS2_D50_BOOSTSPEED);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	//setTimeout (USEC_SEC * 10);
	setTimeout (USEC_SEC);
	//TODO: pridat check, jestli informace z ABS (a inc?) cidel odpovida poloze!
        return 0;
}

void D50::setDiffTrack (double dra, double ddec)
{
	logStream (MESSAGE_DEBUG) << "****** setDiffTrack ()" << sendLog;
        if (parking)
                return;
        if (info ())
                throw rts2core::Error ("cannot call info in setDiffTrack");
        if (!raDrive->isInPositionMode () || !raDrive->isMovingPosition ())
        {
                if (isTracking ())
		{
                        //raDrive->setTargetSpeed (TRACK_SPEED + dra * SPEED_ARCDEGSEC);
			//TODO: tady se musi dodelat trackovani pres REMOTES
		}
                else
		{
                        //raDrive->setTargetSpeed (dra * SPEED_ARCDEGSEC);
			//TODO: tady se musi dodelat trackovani pres REMOTES
		}
        }
        /*if (!decDrive->isInPositionMode () || !decDrive->isMovingPosition ())
        {
                //decDrive->setTargetSpeed (ddec * SPEED_ARCDEGSEC);
		//TODO: tady se musi dodelat trackovani pres REMOTES
        }*/
}

int D50::updateLimits ()
{
	logStream (MESSAGE_DEBUG) << "****** updateLimits ()" << sendLog;
	if ( haCpd > 0 )
	{
		acMin = (int32_t) (haCpd * (-180.0 - haZero));
		acMax = (int32_t) (haCpd * (180.0 - haZero));
	}
	else
	{
		acMax = (int32_t) (haCpd * (-180.0 - haZero));
		acMin = (int32_t) (haCpd * (180.0 - haZero));
	}

	if ( decCpd > 0 )
	{
		dcMin = (int32_t) 0;	// in our case, dc=0 in parking (and lowest possible dec) position
		dcMax = (int32_t) (decCpd * (90.0 - decZero));
	}
	else
	{
		dcMax = (int32_t) 0;	// in our case, dc=0 in parking (and lowest possible dec) position
		dcMin = (int32_t) (decCpd * (90.0 - decZero));
	}
	return 0;
}

int D50::getHomeOffset (int32_t & off)
{
	logStream (MESSAGE_DEBUG) << "****** getHomeOffset ()" << sendLog;
	off = 0;
	return 0;
}

/*void D50::matchGuideRa (int rag)
{
        raDrive->info ();
        if (rag == 0)
        {
                raDrive->stop ();
                addTimer (0.1, new rts2core::Event (RTS2_D50_TSTOPRG));
        }
        else
        {
                raDrive->setTargetPos (raDrive->getPosition () + ((rag == 1 ? -1 : 1) * RAGSTEP));
                addTimer (1, new rts2core::Event (RTS2_D50_TIMERRG));
        }
}*/

/*void D50::matchGuideDec (int deg)
{
        decDrive->info ();
        if (deg == 0)
        {
                decDrive->stop ();
                addTimer (0.1, new rts2core::Event (RTS2_D50_TSTOPDG));
        }
        else
        {
                decDrive->setTargetPos (decDrive->getPosition () + ((deg == 1 ? -1 : 1) * DEGSTEP));
                addTimer (1, new rts2core::Event (RTS2_D50_TIMERDG));
        }
}*/

void D50::callAutosave ()
{
	logStream (MESSAGE_DEBUG) << "****** callAutosave ()" << sendLog;
        if (info ())
                return;
        autosaveValues ();
}

int D50::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	logStream (MESSAGE_DEBUG) << "****** setValue ()" << sendLog;
	/*if (old_value == raPAN)
	{
		matchGuideRa (new_value->getValueInteger ());
		return 0;
	}
	else if (old_value == decPAN)
	{
		matchGuideDec (new_value->getValueInteger ());
		return 0;
	}*/
	if (old_value == remotesMotorsPower)
	{
		return setMotorsPower (new_value->getValueInteger ());
	}
	else if (old_value == remotesMotorsExternalEnable)
	{
		return setMotorsExternalEnable (new_value->getValueInteger ());
	}
	else if (old_value == remotesWormPressureLimiter)
	{
		return setWormPressureLimiter (new_value->getValueInteger ());
	}
	else if (old_value == remotesWormStepsGenerator)
	{
		return setWormStepsGenerator (new_value->getValueInteger ());
	}
	else if (old_value == remotesWormStepsFreqTarget)
	{
		return setWormStepsFreq (new_value->getValueDouble ());
	}

        int ret = raDrive->setValue (old_value, new_value);
        if (ret != 1)
                return ret;
        ret = decDrive->setValue (old_value, new_value);
        if (ret != 1)
                return ret;

	return Fork::setValue (old_value, new_value);
}

int D50::scriptEnds ()
{
	logStream (MESSAGE_DEBUG) << "****** scriptEnds ()" << sendLog;
	if (moveTurboSwitch->getValueBool ())
	{
		moveTurboSwitch->setValueBool (false);
	}
	return Telescope::scriptEnds ();
}

int D50::remotesGetMCRegister (rts2core::ConnREMOTES * connRem, unsigned char * mcReg)
{
	int ret;
	ret = connRem->read1b (0, mcReg);
	if (ret == 1)
		return 0;
	else
		return -1;
}

int D50::remotesSetMCRegister (rts2core::ConnREMOTES * connRem, unsigned char mcReg)
{
	int ret;
	unsigned char uch;
	ret = connRem->write1b (0, mcReg, &uch);
	// we could check the previous value in uch, but we don't bother now...
	if (ret == 1)
		return 0;
	else
		return -1;
}

int D50::remotesGetStepperPulseLength (rts2core::ConnREMOTES * connRem, unsigned short * pulseLength)
{
	int ret;
	ret = connRem->read2b (1, pulseLength);
	if (ret == 2)
		return 0;
	else
		return -1;
}

int D50::remotesSetStepperPulseLength (rts2core::ConnREMOTES * connRem, unsigned short pulseLength)
{
	int ret;
	unsigned short us;
	ret = connRem->write2b (1, pulseLength, &us);
	if (ret == 2)
		return 0;
	else
		return -1;
}

int D50::remotesGetAbsolutePosition (rts2core::ConnREMOTES * connRem, unsigned short * absPos)
{
	int ret;
	ret = connRem->read2b (2, absPos);
	if (ret == 2)
		return 0;
	else
		return -1;
}

int D50::setMotorsPower (bool power)
{
	int ret1, ret2;
	if (power == true)
	{
		remotesMCRegisterRAState |= REMOTES_REG_MC_POWERSUPPLYENABLED;
		remotesMCRegisterDecState |= REMOTES_REG_MC_POWERSUPPLYENABLED;
	}
	else
	{
		remotesMCRegisterRAState &= ~REMOTES_REG_MC_POWERSUPPLYENABLED;
		remotesMCRegisterDecState &= ~REMOTES_REG_MC_POWERSUPPLYENABLED;
	}
	ret1 = remotesSetMCRegister (remotesRA, remotesMCRegisterRAState);
	ret2 = remotesSetMCRegister (remotesDec, remotesMCRegisterDecState);
	if (ret1 == 0 && ret2 == 0)
	{
		remotesMotorsPower->setValueBool (power);
		return 0;
	}
	remotesGetMCRegister (remotesRA, &remotesMCRegisterRAState);
	remotesGetMCRegister (remotesDec, &remotesMCRegisterDecState);
	return -1;
}

int D50::setMotorsExternalEnable (bool extEnable)
{
	int ret1, ret2;
	if (extEnable == true)
	{
		remotesMCRegisterRAState |= REMOTES_REG_MC_DRIVEOUTPUTENABLED;
		remotesMCRegisterDecState |= REMOTES_REG_MC_DRIVEOUTPUTENABLED;
	}
	else
	{
		remotesMCRegisterRAState &= ~REMOTES_REG_MC_DRIVEOUTPUTENABLED;
		remotesMCRegisterDecState &= ~REMOTES_REG_MC_DRIVEOUTPUTENABLED;
	}
	ret1 = remotesSetMCRegister (remotesRA, remotesMCRegisterRAState);
	ret2 = remotesSetMCRegister (remotesDec, remotesMCRegisterDecState);
	if (ret1 == 0 && ret2 == 0)
	{
		remotesMotorsExternalEnable->setValueBool (extEnable);
		return 0;
	}
	remotesGetMCRegister (remotesRA, &remotesMCRegisterRAState);
	remotesGetMCRegister (remotesDec, &remotesMCRegisterDecState);
	return -1;
}

int D50::setWormPressureLimiter (bool pressureLimiter)
{
	int ret;
	if (pressureLimiter == true)
	{
		remotesMCRegisterRAState |= REMOTES_REG_MC_PRESSURELIMITENABLED;
	}
	else
	{
		remotesMCRegisterRAState &= ~REMOTES_REG_MC_PRESSURELIMITENABLED;
	}
	ret = remotesSetMCRegister (remotesRA, remotesMCRegisterRAState);
	if (ret == 0)
	{
		remotesWormPressureLimiter->setValueBool (pressureLimiter);
		return 0;
	}
	remotesGetMCRegister (remotesRA, &remotesMCRegisterRAState);
	return -1;
}

int D50::setWormStepsGenerator (bool stepsGenerator)
{
	int ret;
	if (stepsGenerator == true)
	{
		remotesMCRegisterRAState |= REMOTES_REG_MC_STEPPERSIGNALENABLED;
	}
	else
	{
		remotesMCRegisterRAState &= ~REMOTES_REG_MC_STEPPERSIGNALENABLED;
	}
	ret = remotesSetMCRegister (remotesRA, remotesMCRegisterRAState);
	if (ret == 0)
	{
		remotesWormStepsGenerator->setValueBool (stepsGenerator);
		return 0;
	}
	remotesGetMCRegister (remotesRA, &remotesMCRegisterRAState);
	return -1;
}

int D50::setWormStepsFreq (double freq)
{
	int ret;
	unsigned short pulseLength;
	double freqReal;
	pulseLength = (unsigned short) (96000000.0/freq - 1.0 + 0.5);
	freqReal = 96000000/(1.0 + pulseLength);
	ret = remotesSetStepperPulseLength (remotesRA, pulseLength);
	if (ret == 0)
	{
		remotesWormStepsPulseLength->setValueInteger (pulseLength);
		remotesWormStepsFreqReal->setValueDouble (freqReal);
		remotesWormStepsFreqTarget->setValueDouble (freq);
		return 0;
	}
	return -1;
}

int D50::updateWormStepsFreq (bool updateAlsoTargetFreq)
{
	int ret;
	unsigned short pulseLength;
	double freqReal;
	ret = remotesGetStepperPulseLength (remotesRA, &pulseLength);
	if (ret == 0)
	{
		freqReal = 96000000/(1.0 + pulseLength);
		remotesWormStepsPulseLength->setValueInteger (pulseLength);
		remotesWormStepsFreqReal->setValueDouble (freqReal);
		if (updateAlsoTargetFreq == true)
			remotesWormStepsFreqTarget->setValueDouble (freqReal);
		return 0;
	}
	return ret;
}


int main (int argc, char **argv)
{
	D50 device = D50 (argc, argv);
	return device.run ();
}
