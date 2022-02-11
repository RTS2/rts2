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
//#define DEBUG_BRUTAL

#include "connection/tgdrive.h"
#include "connection/remotes.h"
#include "fork.h"

#include "configuration.h"
#include "libnova_cpp.h"


#define OPT_RA                OPT_LOCAL + 2201
#define OPT_DEC               OPT_LOCAL + 2202

#define RTS2_D50_TIMER_GUIDE_RA    RTS2_LOCAL_EVENT + 1210
#define RTS2_D50_TIMER_GUIDE_DEC    RTS2_LOCAL_EVENT + 1211

#define RTS2_D50_AUTOSAVE   RTS2_LOCAL_EVENT + 1212
#define RTS2_D50_BOOSTSPEED   RTS2_LOCAL_EVENT + 1213

// steps per full RA and DEC revolutions (360 degrees)
//#define RA_TICKS                 (-14350 * 65535)
//#define DEC_TICKS                (-10400 * 65535)
// config pro D50:
//#define DEC_TRANSMISION              2000
#define RA_TRANSMISION                2304
// 24.11.2020 menime prevodovku 1:10 na 1:20, kolo ma 200 zubu
#define DEC_TRANSMISION                4000
#define RA_TICKS               (-2304 * 65536)
//#define DEC_TICKS            (2000 * 65536)
#define DEC_TICKS              (4000 * 65536)

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

// absolute encoders (connected via remotes, placed directly on axes)
#define ABS_ENC_REMOTES_STEPSPERREVOLUTION	8192


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
		virtual void telescopeAboveHorizon ();
		virtual int abortMoveTracking ();
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

		rts2core::ValueDouble *raPosKpSlew;
		rts2core::ValueDouble *raPosKpTrack;
		rts2core::ValueDouble *decPosKpSlew;
		rts2core::ValueDouble *decPosKpTrack;

		rts2core::ValueDouble *guideSpeedRA;
		rts2core::ValueDouble *guideSpeedDEC;
		rts2core::ValueInteger *guidePulseRA;
		rts2core::ValueInteger *guidePulseDEC;
		rts2core::ValueBool *guideActive;

		rts2core::ValueDouble *backlashRA;
		rts2core::ValueDouble *backlashDEC;

		void performGuideRA (int pulseLength = 0);
		void performGuideDEC (int pulseLength = 0);

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
		rts2core::ValueDouble *remotesWormStepsFreqDefault;
		rts2core::ValueDouble *remotesWormStepsFreqTarget;
		rts2core::ValueDouble *remotesWormStepsFreqReal;
		rts2core::ValueInteger *remotesWormStepsPulseLength;
		rts2core::ValueInteger *remotesAbsEncRA;
		rts2core::ValueInteger *remotesAbsEncDec;
		//rts2core::ValueInteger *remotesIncEncRA;
		//rts2core::ValueInteger *remotesIncEncDec;

		// the zero position is at ha/dec = 0/0
		rts2core::ValueInteger *remotesAbsEncRAOffset;
		rts2core::ValueInteger *remotesAbsEncDecOffset;
		double remotesAbsEncDegsPerStep;
		// hadec according to encoders, in degrees
		rts2core::ValueRaDec *remotesAbsEncDeg;
		rts2core::ValueRaDec *remotesAbsEncDegDifference;
		rts2core::ValueDouble *remotesAbsEncDegDifferenceLimit;
		rts2core::ValueBool *remotesAbsEncDegDifferenceIgnore;

		// high-level functions for managing the REMOTES things
		int setMotorsPower (bool power);
		int setMotorsExternalEnable (bool extEnable);
		int setWormPressureLimiter (bool pressureLimiter);
		int setWormStepsGenerator (bool stepsGenerator);
		int setWormStepsFreq (double freq);
		int updateWormStepsFreq (bool updateAlsoTargetFreq = false);

		int32_t lastSafeRaPos;
		int32_t lastSafeDecPos;
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
	
	createValue (backlashRA, "backlash_ra", "[deg] approx. value of backlash in RA", false, RTS2_VALUE_WRITABLE);
	backlashRA->setValueDouble (0.3);

	createValue (backlashDEC, "backlash_dec", "[deg] approx. value of backlash in DEC", false, RTS2_VALUE_WRITABLE);
	backlashDEC->setValueDouble (0.3);

	createValue (moveSpeedBacklash, "speed_backlash", "[deg/s] initial speed to cross backlash", false, RTS2_VALUE_WRITABLE);
	moveSpeedBacklash->setValueDouble (0.3);
	
	createValue (moveSpeedBacklashInterval, "speed_backlash_interval", "[s] interval of keeping initial speed to cross backlash", false, RTS2_VALUE_WRITABLE);
	moveSpeedBacklashInterval->setValueDouble (1.5);

	createValue (moveSpeedFull, "speed_full", "[deg/s] standard maximal slew speed", false, RTS2_VALUE_WRITABLE);
	moveSpeedFull->setValueDouble (1.5);

	createValue (moveSpeedTurbo, "speed_turbo", "[deg/s] turbo maximal slew speed", false, RTS2_VALUE_WRITABLE);
	moveSpeedTurbo->setValueDouble (5.0);

	createValue (decPosKpSlew, "dec_pos_kp_slew", "DEC servo feedback parameter positionKp, slew case", false, RTS2_VALUE_WRITABLE);
	decPosKpSlew->setValueDouble (15.0);

	createValue (raPosKpSlew, "ra_pos_kp_slew", "RA servo feedback parameter positionKp, slew case", false, RTS2_VALUE_WRITABLE);
	raPosKpSlew->setValueDouble (15.0);

	createValue (raPosKpTrack, "ra_pos_kp_track", "RA servo feedback parameter positionKp, tracking case", false, RTS2_VALUE_WRITABLE);
	raPosKpTrack->setValueDouble (15.0);

	createValue (decPosKpTrack, "dec_pos_kp_track", "DEC servo feedback parameter positionKp, tracking case", false, RTS2_VALUE_WRITABLE);
	decPosKpTrack->setValueDouble (15.0);

	// allowed values in RA are 100, (0,16.4>
	createValue (guideSpeedRA, "speed_guide_ra", "[%] RA guiding speed in perc. of sidereal rate", false, RTS2_VALUE_WRITABLE);
	guideSpeedRA->setValueDouble (10.0);
	createValue (guideSpeedDEC, "speed_guide_dec", "[%] DEC guiding speed in perc. of sidereal rate", false, RTS2_VALUE_WRITABLE);
	guideSpeedDEC->setValueDouble (10.0);

	// kladna hodnota - zvysit RA, zaporna hodnota snizit RA, nula = vypnout, pokud je nula od systemu, znamena to ready po popojeti
	// a je to integer, protoze jinak bychom mohli mit problem porovnavat s nulou
	createValue (guidePulseRA, "pulse_guide_ra", "[ms] time to guide in RA, negative changes direction", false, RTS2_VALUE_WRITABLE);
	guidePulseRA->setValueInteger (0);
	createValue (guidePulseDEC, "pulse_guide_dec", "[ms] time to guide in DEC, negative changes direction", false, RTS2_VALUE_WRITABLE);
	guidePulseDEC->setValueInteger (0);

	// na sync s druhou kamerou, zatim netreba
	//createValue (guideActive, "guide_active", "[true/false] guiding is active & set", false, RTS2_VALUE_WRITABLE);
	//guideActive->setValueBool (false);

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
	createValue (remotesWormStepsFreqDefault, "remotes_worm_steps_freq_def", "default frequency of HA tracking steps generator [Hz]", false, RTS2_VALUE_WRITABLE);
	remotesWormStepsFreqDefault->setValueDouble (1752.41140771);	// computed default value
	createValue (remotesWormStepsFreqTarget, "remotes_worm_steps_freq_tar", "target frequency, including guiding effects [Hz]", false);
	remotesWormStepsFreqTarget->setValueDouble (remotesWormStepsFreqDefault->getValueDouble ());
	createValue (remotesWormStepsFreqReal, "remotes_worm_steps_freq", "actual frequency of HA tracking steps generator [Hz]", false);
	createValue (remotesWormStepsPulseLength, "remotes_worm_steps_pulse_length", "pulse length of HA tracking steps generator", false);
	createValue (remotesAbsEncRA, "remotes_abs_encoder_ra", "raw abs. encoder position, RA axis", false);
	createValue (remotesAbsEncDec, "remotes_abs_encoder_dec", "raw abs. encoder position, Dec axis", false);
	createValue (remotesAbsEncRAOffset, "remotes_abs_encoder_ra_offset", "encoder's positional offset, RA axis", false, RTS2_VALUE_WRITABLE);
	createValue (remotesAbsEncDecOffset, "remotes_abs_encoder_dec_offset", "encoder's positional offset, Dec axis", false, RTS2_VALUE_WRITABLE);

	remotesAbsEncDegsPerStep = 360.0 / (double) ABS_ENC_REMOTES_STEPSPERREVOLUTION;

	createValue (remotesAbsEncDeg, "remotes_abs_encoder_deg", "abs. encoder HA/Dec position [deg]", false, RTS2_DT_DEGREES);
	createValue (remotesAbsEncDegDifference, "remotes_abs_encoder_difference", "difference of positions: motor - encoder, [deg]", false, RTS2_DT_DEGREES);
	createValue (remotesAbsEncDegDifferenceLimit, "limit_remotes_abs_encoder_difference", "limit of difference of positions, [deg]", false, RTS2_VALUE_WRITABLE);
	remotesAbsEncDegDifferenceLimit->setValueDouble (0.5);
	createValue (remotesAbsEncDegDifferenceIgnore, "ignore_remotes_abs_encoder_difference", "ignore limit of difference of positions", false, RTS2_VALUE_WRITABLE);
	remotesAbsEncDegDifferenceIgnore->setValueBool (false);

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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** postEvent(): " << event->getType () << "." << sendLog;
#endif
        switch (event->getType ())
        {
                case RTS2_D50_TIMER_GUIDE_RA:
                        performGuideRA ();
                        break;
                case RTS2_D50_TIMER_GUIDE_DEC:
                        performGuideDEC ();
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** commandAuthorized (): " << conn->getCommand () << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** processOption ()" << sendLog;
#endif
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
	remotesRA->setDebug (getDebug ());
	//remotesRA->setDebug (true);
	remotesRA->init ();
	remotesMCRegisterRAState = REMOTES_REG_MC_POWERSUPPLYENABLED | REMOTES_REG_MC_DRIVEOUTPUTENABLED | REMOTES_REG_MC_CLOCKWISEDIRECTIONENABLED;
	ret = remotesSetMCRegister (remotesRA, remotesMCRegisterRAState);
        if (ret)
                return ret;

	remotesDec = new rts2core::ConnREMOTES (this, remotesEthDec, remotesMacDec);
	remotesDec->setDebug (getDebug ());
	//remotesDec->setDebug (true);
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

	setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble ());
	remotesGetAbsolutePosition (remotesRA, &us);
	remotesAbsEncRA->setValueInteger (us);
	remotesAbsEncDeg->setRa ( ln_range_degrees ( (double) (us - remotesAbsEncRAOffset->getValueInteger ()) * remotesAbsEncDegsPerStep + 180.0) - 180.0);

	remotesGetAbsolutePosition (remotesDec, &us);
	remotesAbsEncDec->setValueInteger (us);
	remotesAbsEncDeg->setDec ( ln_range_degrees ( (double) (us - remotesAbsEncDecOffset->getValueInteger ()) * remotesAbsEncDegsPerStep + 180.0) - 180.0);

	// safe initial values for low-level "last safe" ac/dc - 0:0 is close to the "park" position
	lastSafeRaPos = 0;
	lastSafeDecPos = 0;

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
	unsigned short usRA, usDec;
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** info ()" << sendLog;
#endif
        raDrive->info ();
        decDrive->info ();

	remotesGetMCRegister (remotesRA, &remotesMCRegisterRAState);
	remotesGetMCRegister (remotesDec, &remotesMCRegisterDecState);
	remotesWormPressureLimiter->setValueBool ((remotesMCRegisterRAState & REMOTES_REG_MC_PRESSURELIMITENABLED) == 0x00 ? false : true);
	remotesWormStepsGenerator->setValueBool ((remotesMCRegisterRAState & REMOTES_REG_MC_STEPPERSIGNALENABLED) == 0x00 ? false : true);
	updateWormStepsFreq ();

	if (guidePulseRA == 0 && remotesWormStepsFreqTarget->getValueDouble () != remotesWormStepsFreqDefault->getValueDouble ())	// safe check
		setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble ());

	remotesGetAbsolutePosition (remotesRA, &usRA);
	remotesAbsEncRA->setValueInteger (usRA);
	remotesAbsEncDeg->setRa (ln_range_degrees ( (double) (usRA - remotesAbsEncRAOffset->getValueInteger ()) * remotesAbsEncDegsPerStep + 180.0) - 180.0);

	remotesGetAbsolutePosition (remotesDec, &usDec);
	remotesAbsEncDec->setValueInteger (usDec);
	remotesAbsEncDeg->setDec (ln_range_degrees ( (double) (usDec - remotesAbsEncDecOffset->getValueInteger ()) * remotesAbsEncDegsPerStep + 180.0) - 180.0);

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
	remotesAbsEncDegDifference->setRa ((double) (raPos / haCpd) + haZero - remotesAbsEncDeg->getRa ());	// it's probably better this way than getting and subtracting LST from ut_telRa
	remotesAbsEncDegDifference->setDec (ut_telDec - remotesAbsEncDeg->getDec ());

	//logStream (MESSAGE_DEBUG) << "info: position (drive_raw/drive_sky/sensor_raw): HA " << raPos << "/" << (double) (raPos / haCpd) + haZero << "/" << usRA << ", DEC " << decPos << "/" << (double) (decPos / decCpd) + decZero << "/" << usDec << sendLog;

	return Fork::info ();
}

int D50::idle ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** D50:idle ()" << sendLog;
#endif
	runD50 ();


	/*unsigned short usRA, usDec;
	remotesGetAbsolutePosition (remotesRA, &usRA);
	remotesGetAbsolutePosition (remotesDec, &usDec);
	if (remotesAbsEncRA->getValueInteger () != usRA || remotesAbsEncDec->getValueInteger () != usDec)
	{
		remotesAbsEncRA->setValueInteger (usRA);
		remotesAbsEncDec->setValueInteger (usDec);

	        int32_t raPos = raDrive->getPositionAct ();
	        int32_t decPos = decDrive->getPositionAct ();

		logStream (MESSAGE_DEBUG) << "idle: position (drive_raw/drive_sky/sensor_raw): HA " << raPos << "/" << (double) (raPos / haCpd) + haZero << "/" << usRA << ", DEC " << decPos << "/" << (double) (decPos / decCpd) + decZero << "/" << usDec << sendLog;
	}*/
	return Fork::idle ();
}

int D50::runD50 ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** D50:runD50 ()" << sendLog;
#endif
	if (!(getState () & TEL_MOVING))
	{
		if ((trackingRequested () == 0) && raDrive->isInStepperMode ())
		{
#ifdef DEBUG_BRUTAL
			logStream (MESSAGE_DEBUG) << "****** runD50 () - setting tracking to off - DS mode" << sendLog;
#endif
                        raDrive->setTargetSpeed ( 0, true );
                        raDrive->setMode (TGA_MODE_DS);
			setWormStepsGenerator (false);
                        //raDrive->info ();
		} else if (isTracking () == true && !raDrive->isInStepperMode ()) 
		{
#ifdef DEBUG_BRUTAL
			logStream (MESSAGE_DEBUG) << "****** runD50 () - setting tracking to on - SM mode" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** resetMount ()" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** startResync ()" << sendLog;
#endif
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

	//TODO: overit, jestli to "probouzeni" nema nejaky dusledky, pokud ano, tak udelat check jenom pro pripad, ze je montaz zaparkovana
	raDrive->wakeup ();
	decDrive->wakeup ();
	setWormPressureLimiter (true);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	raDrive->setPositionKp (raPosKpSlew->getValueFloat ());
	decDrive->setPositionKp (decPosKpSlew->getValueFloat ());
	addTimer (moveSpeedBacklashInterval->getValueDouble (), new rts2core::Event (RTS2_D50_BOOSTSPEED));
        raDrive->setTargetPos (tAc);
        decDrive->setTargetPos (dc);
        return 0;
}

int D50::isMoving ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** isMoving ()" << sendLog;
        //callAutosave ();
	logStream (MESSAGE_DEBUG) << "------ isMoving: getState=" << getState () << ", tracking=" << trackingRequested () << ", parking=" << parking << ", raDrive->isMoving=" << raDrive->isMoving () << ", raDrive->isInPositionMode=" << raDrive->isInPositionMode () << ", decDrive->isMoving=" << decDrive->isMoving () << sendLog;
#endif
        if ((trackingRequested () != 0) && raDrive->isInPositionMode ())
        {
                if (raDrive->isMovingPosition ())
                {
#ifdef DEBUG_BRUTAL
			logStream (MESSAGE_DEBUG) << "------ updating ac position!" << sendLog;
#endif
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
                                tAc = ac + USEC_SEC / 10 / 3600 / 24 * 360 * haCpd;	// setting value in advance - the sidereal tracking ("worm") will not be started sooner than in next cycle of isMoving () (the USEC_SEC / 10 corresponds to returned value below)
                                raDrive->setTargetPos (tAc);
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** endMove ()" << sendLog;
#endif
	//addTimer (5, new rts2core::Event (RTS2_D50_AUTOSAVE));
	setWormPressureLimiter (false);
	// snizeni rychlosti presunu obou motoru
	deleteTimers (RTS2_D50_BOOSTSPEED);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	raDrive->setPositionKp (raPosKpTrack->getValueFloat ());
	decDrive->setPositionKp (decPosKpTrack->getValueFloat ());
	setTimeout (USEC_SEC);
	info ();
	if (! remotesAbsEncDegDifferenceIgnore->getValueBool () && (remotesAbsEncDegDifference->getRa () > remotesAbsEncDegDifferenceLimit->getValueDouble () || remotesAbsEncDegDifference->getDec () > remotesAbsEncDegDifferenceLimit->getValueDouble ()))
	{
		// TODO: just scream for now, we will solve this more appropriately in the future
		logStream (MESSAGE_ERROR) << "Remotes ABS encoders difference above the limit: " << remotesAbsEncDegDifference->getRa () << ":" << remotesAbsEncDegDifference->getDec () << sendLog;
	}
	return Fork::endMove ();
}

void D50::telescopeAboveHorizon ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** telescopeAboveHorizon ()" << sendLog;
#endif
	lastSafeRaPos = raDrive->getPosition ();
	lastSafeDecPos = decDrive->getPosition ();
}

int D50::abortMoveTracking ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** abortMoveTracking ()" << sendLog;
#endif
	Telescope::abortMoveTracking ();	// stop tracking, also waits till stop is really finished

	// OK, this might be a bit controversial, let's try this approach - after being caught in a "below horizon" area, we stop and then return to the previously known safe position.
	// We use only a "low level" (and slow) movements for it, i.e. rts2 doesn't know we are moving. When there will be some moving-attempt in meantime, it will be either accepted
	// (which will lead to normal operations) or denied by a below-horizon situation again, in which case a new "extrication attempt" will follow in the next info cycle.
	// As this is only an emergency case (which should happen only rarely), this should be acceptable.
	// TODO: Solve a potential (temporarily) below-horizon unavailability for "park" operations. (Repeat the park command after an interval, maybe? Or use some advanced techniques.)
	if (labs (raDrive->getPosition () - lastSafeRaPos) < (int32_t) fabs (10.0 * haCpd) && labs (decDrive->getPosition () - lastSafeDecPos) < (int32_t) fabs (10.0 * decCpd)) // safety limit
	{
		logStream (MESSAGE_DEBUG) << "abortMoveTracking - returning back to last known safe position: " << lastSafeRaPos << " " << lastSafeDecPos << sendLog;
		raDrive->setTargetPos (lastSafeRaPos);
		decDrive->setTargetPos (lastSafeDecPos);
	}
	else
	{
		logStream (MESSAGE_DEBUG) << "abortMoveTracking - distance to lastSafePos too high:" << labs (raDrive->getPosition () - lastSafeRaPos) / haCpd << " " << labs (decDrive->getPosition () - lastSafeDecPos) / decCpd << sendLog;
	}
	return 0;
}

int D50::stopMove ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** stopMove ()" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** setTo ()" << sendLog;
#endif
        struct ln_equ_posn eq;
        eq.ra = set_ra;
        eq.dec = set_dec;
        int32_t ac;
        int32_t dc;
        int32_t off;
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "------ setting coordinates to RA: " << set_ra << ", DEC: " << set_dec << sendLog;
#endif
        getHomeOffset (off);
	zeroCorrRaDec ();
        int ret = sky2counts (&eq, ac, dc, ln_get_julian_from_sys ());
        if (ret)
                return -1;

#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "------ setting coordinates to ac: " << ac << ", dc: " << dc << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** setToPark ()" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** startPark ()" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** endPark ()" << sendLog;
#endif
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
	usleep (USEC_SEC * 2);
	raDrive->sleep ();
	decDrive->sleep ();
        return 0;
}

void D50::setDiffTrack (double dra, double ddec)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** setDiffTrack ()" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** updateLimits ()" << sendLog;
#endif
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** getHomeOffset ()" << sendLog;
#endif
	off = 0;
	return 0;
}

void D50::performGuideRA (int pulseLength)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** performGuideRA ()" << sendLog;
#endif
	//logStream (MESSAGE_DEBUG) << "***GUIDING: performGuideRA " << sendLog;
	deleteTimers (RTS2_D50_TIMER_GUIDE_RA);
	if (pulseLength == 0 )	// stop and reset guiding after realization
	{
		//logStream (MESSAGE_DEBUG) << "***GUIDING: RA resetting" << sendLog;
		setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble ());
		guidePulseRA->setValueInteger (0);
		sendValueAll (guidePulseRA);
		//logStream (MESSAGE_DEBUG) << "***GUIDING: RA resetted" << sendLog;
	}
	else
	{
		if (abs (pulseLength) > 3000 || ((getState () & TEL_MASK_MOVING) == TEL_PARKED || ((getState () & TEL_MASK_TRACK) != TEL_TRACKING) || getBlockMove () == true))	// don't really realize guiding in a case the guiding is not suitable - but the timer still has to be set to reset the pulse value in the next run
		{
			// comment to the sutability check above: the TRACKING check should be enough under normal circumstances, but as we use a low-level manipulation with the mount, it's better to add more conditions to solve potential bugs in a general teld code
			//logStream (MESSAGE_DEBUG) << "***GUIDING: RA addtimer minimal" << sendLog;
			addTimer (1.0/1000.0, new rts2core::Event (RTS2_D50_TIMER_GUIDE_RA));	// minimal timer interval
			return;
		}
		if (pulseLength>0)	// increase RA => decrease speed
			setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble () * (1.0 - guideSpeedRA->getValueDouble ()/100.0));
		else			// decrease RA => increase speed
			setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble () * (1.0 + guideSpeedRA->getValueDouble ()/100.0));
		if (abs (pulseLength)<30)
		{
			// realize using sleep for shorter intervals
			usleep (abs(pulseLength) * 1000);
			setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble ());
			//logStream (MESSAGE_DEBUG) << "***GUIDING: RA addtimer minimal" << sendLog;
			addTimer (1.0/1000.0, new rts2core::Event (RTS2_D50_TIMER_GUIDE_RA));	// minimal timer interval
		}
		else
		{
			//logStream (MESSAGE_DEBUG) << "***GUIDING: RA addtimer" << sendLog;
			addTimer ((double) abs(pulseLength)/1000.0, new rts2core::Event (RTS2_D50_TIMER_GUIDE_RA));
		}
	}
}

void D50::performGuideDEC (int pulseLength)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** performGuideDEC ()" << sendLog;
#endif
	//logStream (MESSAGE_DEBUG) << "***GUIDING: performGuideDEC " << sendLog;
	deleteTimers (RTS2_D50_TIMER_GUIDE_DEC);	// OK, using the timer is not necessary here, but still, to keep the same form as in RA and to fulfil expected behaviour...
	if (pulseLength == 0 )  // stop and reset guiding after realization
	{
		//logStream (MESSAGE_DEBUG) << "***GUIDING: DEC resetting" << sendLog;
		guidePulseDEC->setValueInteger (0);
		sendValueAll (guidePulseDEC);
		//logStream (MESSAGE_DEBUG) << "***GUIDING: DEC resetted" << sendLog;
	}
	else
	{
		if (abs (pulseLength) > 3000 || ((getState () & TEL_MASK_MOVING) == TEL_PARKED || ((getState () & TEL_MASK_TRACK) != TEL_TRACKING) || getBlockMove () == true))	// don't really realize guiding in a case the guiding is not suitable - but the timer still has to be set to reset the pulse value in the next run
		{
			// comment to the sutability check above: the TRACKING check should be enough under normal circumstances, but as we use a low-level manipulation with the mount, it's better to add more conditions to solve potential bugs in a general teld code
			return;
		}
		if ((getState () & TEL_MASK_MOVING) == TEL_OBSERVING && isTracking () == true)	// realize the movement only in a case we are really tracking (the movement is made on low-level base, rather take care of it duplicitly!)
		{
			//logStream (MESSAGE_DEBUG) << "***GUIDING: DEC pulselength " << pulseLength << ", would update decPosition: " << decDrive->getPositionTarget () << " + " << (int32_t)( (((double) pulseLength / 1000.0) / 240.0) * (guideSpeedDEC->getValueDouble ()/100.0) * (double) decCpd + 0.5) << " -> " << decDrive->getPositionTarget () + (int32_t)( (((double) pulseLength / 1000.0) / 240.0) * (guideSpeedDEC->getValueDouble ()/100.0) * (double) decCpd + 0.5) << sendLog;
			decDrive->setTargetPos ( decDrive->getPositionTarget () + (int32_t)( (((double) pulseLength / 1000.0) / 240.0) * (guideSpeedDEC->getValueDouble ()/100.0) * (double) decCpd + 0.5) );
		}
		//logStream (MESSAGE_DEBUG) << "***GUIDING: DEC addtimer" << sendLog;
		addTimer ((double) abs(pulseLength)/1000.0, new rts2core::Event (RTS2_D50_TIMER_GUIDE_DEC));
	}
}

void D50::callAutosave ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** callAutosave ()" << sendLog;
#endif
        if (info ())
                return;
        autosaveValues ();
}

int D50::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** setValue ()" << sendLog;
#endif
	if (old_value == guidePulseRA)
	{
		performGuideRA (new_value->getValueInteger ());
		return 0;
	}
	else if (old_value == guidePulseDEC)
	{
		performGuideDEC (new_value->getValueInteger ());
		return 0;
	}
	else if (old_value == guideSpeedRA)
	{
		// allowed values are only (0,16.4>
		double newspeed = new_value->getValueDouble ();
		if ( (newspeed > 0) && (newspeed <= 16.4))
			return 0;
		else
		{
			logStream (MESSAGE_ERROR) << "speed_guide_ra: allowed values are only (0,16.4>" << sendLog;
			return -2;
		}
	}
	else if (old_value == remotesMotorsPower)
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
	else if (old_value == remotesWormStepsFreqDefault)
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
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** scriptEnds ()" << sendLog;
#endif
	if (moveTurboSwitch->getValueBool ())
	{
		moveTurboSwitch->setValueBool (false);
	}
	setWormStepsFreq (remotesWormStepsFreqDefault->getValueDouble ());
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
		sendValueAll (remotesWormStepsGenerator);
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
	if (freq < 1465)	// especially for the case of 0 (used for guiding)
	{
		setWormStepsGenerator (false);
		return 0;
	}
	pulseLength = (unsigned short) (96000000.0/freq - 1.0 + 0.5);
	freqReal = 96000000/(1.0 + pulseLength);
	ret = remotesSetStepperPulseLength (remotesRA, pulseLength);
	if (ret == 0)
	{
		remotesWormStepsPulseLength->setValueInteger (pulseLength);
		remotesWormStepsFreqReal->setValueDouble (freqReal);
		remotesWormStepsFreqTarget->setValueDouble (freq);
		sendValueAll (remotesWormStepsPulseLength);
		sendValueAll (remotesWormStepsFreqReal);
		sendValueAll (remotesWormStepsFreqTarget);
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
