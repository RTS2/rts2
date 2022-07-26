/* 
 * Driver for Ondrejov, Astrolab D50 scope.
 * Copyright (C) 2007,2010 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010-2022 Jan Strobl
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


#define OPT_RA				OPT_LOCAL + 2201
#define OPT_DEC				OPT_LOCAL + 2202
#define OPT_CEILING			OPT_LOCAL + 2203
#define OPT_DOME_DEVICE			OPT_LOCAL + 2204
#define OPT_DOME_OPENSWITCH_VARIABLE	OPT_LOCAL + 2205

#define RTS2_D50_TIMER_GUIDE_RA		RTS2_LOCAL_EVENT + 1210
#define RTS2_D50_TIMER_GUIDE_DEC	RTS2_LOCAL_EVENT + 1211

#define RTS2_D50_AUTOSAVE		RTS2_LOCAL_EVENT + 1212
#define RTS2_D50_BOOSTSPEED		RTS2_LOCAL_EVENT + 1213
#define RTS2_D50_SUCCESSIVE_POINTS	RTS2_LOCAL_EVENT + 1214

// config pro D50:
#define SERVO_COUNTS_PER_ROUND	65536
#define RA_TRANSMISION		2304
// 24.11.2020 menime prevodovku 1:10 na 1:20, kolo ma 200 zubu
#define DEC_TRANSMISION		4000
// steps per full RA and DEC revolutions (360 degrees)
#define RA_TICKS		(-2304 * 65536)
#define DEC_TICKS		(4000 * 65536)

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

// timestep [s] for tracing path in checkPassage routine
#define CHECKPASSAGE_TIME_RESOLUTION	0.2
// timestep [s] for examining path in findPath routine
#define FINDPATH_TIME_RESOLUTION	1.0
// minimal step size in degrees for findPath routine
#define FINDPATH_MIN_STEP_SIZE		5
#define MAX_NUMBER_OF_SUCCESSIVE_POINTS	20

// backlashCrossStrategy selection values
#define BACKLASH_CROSS_STRATEGY_TIME_INTERVAL	0
#define BACKLASH_CROSS_STRATEGY_ENCODER_TICKS	1
#define BACKLASH_CROSS_STRATEGY_MIND_DIRECTION_AND_SIZE	2

// We should never get closer to Sun than... [deg]
#define SUN_AVOID_DISTANCE 10.0


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
		//virtual void beforeRun ();
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
		virtual int willConnect (rts2core::NetworkAddress * in_addr);
	private:
		int runD50 ();

		/**
		* Try to find path from actual telescope position to the target.
		* Sets values to successivePointsAC[], successivePointsDC[] fields.
		*
		* @return <0 when path not found, =0 when simple ("straight") movement strategy
		* is suitable, >0 returns number of successive points used to reach the target.
		*/
		int findPath ();

		/**
		* Find shortest possible successivePointInterval, after which can be movement from*->to* updated to target next*.
		*
		* @param fromAC		initial position of HA [counts] of preceding movement
		* @param fromDC		initial position of Dec [counts] of preceding movement
		* @param toAC		final position of HA [counts] of preceding movement
		* @param toDC		final position of Dec [counts] of preceding movement
		* @param initSpeedAC	initial speed in HA [counts/s] of preceding movement
		* @param initSpeedDC	initial speed in Dec [counts/s] of preceding movement
		* @param nextAC		next position of HA [counts]
		* @param nextDC		next position of Dec [counts]
		*
		* @return shortest found interval [s] for realizing the target transition, negative number when nothing found
		*/
		double computeSuccessivePointInterval (int32_t fromAC, int32_t fromDC, int32_t toAC, int32_t toDC, int32_t nextAC, int32_t nextDC, double initSpeedAC=0, double initSpeedDC=0);

		/**
		* Check, whether the considered simple "two points" passage of the telescope
		* will not conflict with the defined hard-horizon.
		* Also returns the expected time of movement.
		* Takes into account current values of acc/decc and max speed for both axes, as well as initial speeds in both axes.
		*
		* @param fromAC		initial position of HA [counts]
		* @param fromDC		initial position of Dec [counts]
		* @param toAC		final position of HA [counts]
		* @param toDC		final position of Dec [counts]
		* @param initSpeedAC	initial speed in HA [counts/s]
		* @param initSpeedDC	initial speed in Dec [counts/s]
		*
		* @return >0 success, expected time needed to finish movement [s]
		* @return <=0 conflict with horizon recognized at fabs() [s] from start
		*/
		double checkPassage (int32_t fromAC, int32_t fromDC, int32_t toAC, int32_t toDC, double initSpeedAC=0, double initSpeedDC=0);

		/**
		* Simply return time needed to fully complete complex both-axis movement, no checks performed.
		*
		* @param fromAC		initial position of HA [counts]
		* @param fromDC		initial position of Dec [counts]
		* @param toAC		final position of HA [counts]
		* @param toDC		final position of Dec [counts]
		* @param initSpeedAC	initial speed in HA [counts/s]
		* @param initSpeedDC	initial speed in Dec [counts/s]
		*
		* @return time [s] till the movement is done
		*/
		double computeFullMovementTime (int32_t fromAC, int32_t fromDC, int32_t toAC, int32_t toDC, double initSpeedAC=0, double initSpeedDC=0);

		/**
		* Compute actual position of specified movement at specific time inspectionTime, for one axis only.
		* Negative value of inspectionTime changes meaning, in this case return time in miliseconds needed to travel the full distance.
		* When double * speedAtInspectionTime is defined, also compute an actual speed.
		*
		* @param isDec				0 for HA, other (typically 1) for Dec axis
		* @param fromCount			initial position [counts]
		* @param toCount			final position [counts]
		* @param initSpeed			initial speed [counts/s]
		* @param inspectionTime			time (from the beginning of movement) for which the result is requested, value <0 means full travel time requested
		* @param *speedAtInspectionTime		where the computed speed at inspectionTime is stored (only when pointer differs from NULL)
		*
		* @return computed position [counts] at inspectionTime, or time [ms] needed to complete all movemens, if inspectionTime<0
		*/
		int32_t computeMovement (int isDec, int32_t fromCount, int32_t toCount, double initSpeed, double inspectionTime, double* speedAtInspectionTime = NULL);

		/**
		* Check potential conflict with a hard-horizon, Sun distance and dome's ceiling for the position in raw coordinates.
		*
		* @return 1 if position is ok, 0 when collision with hard-horizon, Sun proximity or dome's ceiling (when checked) detected
		*/
		int checkRawPosition (int32_t AC, int32_t DC, bool checkSunDistance = true, bool checkCeiling = false);

		int numberOfSuccesivePoints;
		int32_t successivePointsAC[MAX_NUMBER_OF_SUCCESSIVE_POINTS + 1], successivePointsDC[MAX_NUMBER_OF_SUCCESSIVE_POINTS + 1];
		double successivePointsIntervals[MAX_NUMBER_OF_SUCCESSIVE_POINTS];
		int successivePointsActualPosition;


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

		rts2core::ValueSelection *backlashCrossStrategy;
		rts2core::ValueInteger *backlashCrossEncoderTicks;
		unsigned short backlashCrossEncoderStartRA, backlashCrossEncoderStartDec;
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

		const char *closedDomeCeilingFile;		// ceiling "horizon" file (for closed dome)
		ObjectCheck hardCeiling;
		rts2core::ValueBool *insideDomeOnlyMovement;

		struct ln_lnlat_posn observerLongLat;		// we use this many times and Longitude/Latitude is constant for us, so...

		// variables willConnect () will use, for direct communication with dome (and ensuring the dome is really open)
		const char *domeDevice;
		const char *domeDeviceOpenswitchVariable;
		rts2core::ValueBool *disableDomeOpenCheck;

		/**
		* Get info from (external) dome device.
		*
		* @return zero (0) when the dome is open (i.e., the openswitch is triggered), -1 if an error occur, 1 otherwise
		*/
		int getDomeState ();
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

	createValue (backlashCrossStrategy, "backlash_cross_strategy", "strategy to get over backlash in gearings", false, RTS2_VALUE_WRITABLE);
	backlashCrossStrategy->addSelVal ("Time interval");
	backlashCrossStrategy->addSelVal ("Encoder ticks");
	//backlashCrossStrategy->addSelVal ("Mind direction and size"); // TBD (maybe), in principle, we can know/measure the size of a backlash, remember a last movement direction and use the backlash offset automatically
	backlashCrossStrategy->setValueInteger (BACKLASH_CROSS_STRATEGY_ENCODER_TICKS);

	createValue (backlashCrossEncoderTicks, "backlash_encoder_ticks", "number of encoder tics to consider backlash being overcome", false, RTS2_VALUE_WRITABLE);
	backlashCrossEncoderTicks->setValueInteger (2);

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

	addOption (OPT_CEILING, "ceiling", 1, "telescope hard ceiling (anti-horizon) when the dome is closed");

	addOption (OPT_DOME_DEVICE, "dome_device", 1, "name of dome device [DOME]");
	addOption (OPT_DOME_OPENSWITCH_VARIABLE, "dome-openswitch-variable", 1, "name of variable in dome, which holds the dome-open info [roof_open]");

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

	closedDomeCeilingFile = NULL;

	createValue (insideDomeOnlyMovement, "inside_dome_limit", "limit movement of the telescope to the inside-dome space, also enables closing of the dome", false, RTS2_VALUE_WRITABLE);
	insideDomeOnlyMovement->setValueBool (false);

	createValue (disableDomeOpenCheck, "disable_domeopen_check", "disable checking the dome's endswitch (requirement for movement)", false, RTS2_VALUE_WRITABLE);
	disableDomeOpenCheck->setValueBool (false);
	domeDevice = "DOME";
	domeDeviceOpenswitchVariable = "roof_opened";

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
				if (backlashCrossStrategy->getValueInteger () == BACKLASH_CROSS_STRATEGY_ENCODER_TICKS)
				{
					unsigned short encodersCurrentRA, encodersCurrentDec;
					remotesGetAbsolutePosition (remotesRA, &encodersCurrentRA);
					remotesGetAbsolutePosition (remotesDec, &encodersCurrentDec);
					encodersCurrentRA = (encodersCurrentRA - remotesAbsEncRAOffset->getValueInteger () + 12288)%8192;	// in encoder units, but normalized and zero in "zeto" (ha/dec = 0/0) position;
					encodersCurrentDec = (encodersCurrentDec - remotesAbsEncDecOffset->getValueInteger () + 12288)%8192;
					#ifdef DEBUG_EXTRA
						logStream (MESSAGE_DEBUG) << "------ RTS2_D50_BOOSTSPEED, encoders values: " << encodersCurrentRA << " - " << backlashCrossEncoderStartRA << ", " << encodersCurrentDec << " - " << backlashCrossEncoderStartDec << "." << sendLog;
					#endif
					if ((abs (encodersCurrentRA - backlashCrossEncoderStartRA) < backlashCrossEncoderTicks->getValueInteger () && raDrive->isMoving ()) || (abs (encodersCurrentDec - backlashCrossEncoderStartDec) < backlashCrossEncoderTicks->getValueInteger () && decDrive->isMoving ()))
					{
						addTimer (0.1, new rts2core::Event (RTS2_D50_BOOSTSPEED));
						return;
					}
				}

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
				if (numberOfSuccesivePoints > 0)
					addTimer (successivePointsIntervals[0], new rts2core::Event (RTS2_D50_SUCCESSIVE_POINTS));
                        }
                        break;
                case RTS2_D50_SUCCESSIVE_POINTS:
			successivePointsActualPosition++;
			//nastavit cilove souradnice, mozna nejaky ten check?
			tAc = successivePointsAC[successivePointsActualPosition];
			int32_t dc = successivePointsDC[successivePointsActualPosition];
			#ifdef DEBUG_EXTRA
				logStream (MESSAGE_DEBUG) << "------ RTS2_D50_SUCCESSIVE_POINT: successivePointsActualPosition: " << successivePointsActualPosition << ", numberOfSuccesivePoints: " << numberOfSuccesivePoints << ", tAc: " << tAc << ", dc: " << dc << "." << sendLog;
			#endif

			// this is a redundant local check, we still do it to be sure not to make a flip and harm ourselfs...
			if (dc < dcMin || dc > dcMax)
			{
				logStream (MESSAGE_ERROR) << "dc value out of limits!" << sendLog;
				return;
			}
			raDrive->setTargetPos (tAc);
			decDrive->setTargetPos (dc);
			if (successivePointsActualPosition < numberOfSuccesivePoints)
				addTimer (successivePointsIntervals[successivePointsActualPosition], new rts2core::Event (RTS2_D50_SUCCESSIVE_POINTS));
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
                case OPT_CEILING:
                        closedDomeCeilingFile = optarg;
                        break;
                case OPT_DOME_DEVICE:
                        domeDevice = optarg;
                        break;
                case OPT_DOME_OPENSWITCH_VARIABLE:
                        domeDeviceOpenswitchVariable = optarg;
                        break;
                default:
                        return Fork::processOption (opt);
        }
        return 0;
}

int D50::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_DOME && in_addr->isAddress (domeDevice)) {
		logStream (MESSAGE_DEBUG) << "D50::willConnect to DEVICE_TYPE_DOME: "<< domeDevice << sendLog;
		return 1;
	}
	return Fork::willConnect (in_addr);
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

	numberOfSuccesivePoints = 0;
	successivePointsActualPosition = 0;

	// and finally, the remaining usual teld setup...
	rts2core::Configuration *config = rts2core::Configuration::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	observerLongLat.lng = config->getObserver ()->lng;
	observerLongLat.lat = config->getObserver ()->lat;
	setTelLongLat (observerLongLat.lng, observerLongLat.lat);
	setTelAltitude (config->getObservatoryAltitude ());

	// zero dec is on local meridian, 90 - telLatitude bellow (to nadir)
	decZero = - (90.0 - fabs (telLatitude->getValueDouble ()));

	if (telLatitude->getValueDouble () < 0)
	{
		decZero *= -1.0;
		// swap values which are opposite for south hemispehere
	}

	if (closedDomeCeilingFile)
	{
		if (hardCeiling.loadHorizon (closedDomeCeilingFile))
			return -1;
	}

	// TODO: mozna jeste poresit i uplne vypnuti serv? A take pridat check, jestli aktualni poloha ze serv odpovida te z cidel...

	//addTimer (1, new rts2core::Event (RTS2_D50_AUTOSAVE));

	return 0;
}

/* - finally, it looks like it's not needed, but I'll keep it here yet, for now at least.
void D50::beforeRun ()
{
	// this is because we need to have initialized the mount's positional values, primarily because of the initial changeMasterState () checks...
	info ();
	Telescope::info ();
}*/

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

	int retFP;
	int32_t dc;

	if (!disableDomeOpenCheck->getValueBool ())
	{
		if (getDomeState ())		// when DOME is not fully open or not reachable (AND domeopen_check is not disabled), limit the movement of the telescope to the "inside dome" space (only if we already are in this area - as setting this variable insideDomeOnlyMovement also implies the roof closing can start!)
		{
			logStream (MESSAGE_DEBUG) << "****** startResync () - domestate bad, limit the movement" << sendLog;
			struct ln_hrz_posn hrpos;
			hrpos.alt = telAltAz->getAlt ();
			hrpos.az = telAltAz->getAz ();
			//logStream (MESSAGE_DEBUG) << "****** getTelRa (): " << getTelRa () << ", getTelDec (): " << getTelDec () << sendLog;
			//logStream (MESSAGE_DEBUG) << "****** hrpos.alt: " << hrpos.alt << ", hrpos.az: " << hrpos.az << sendLog;
			if (hardCeiling.is_good (&hrpos))	// !!! we use the "horizon" object, but need a ceiling test, so the logic is reversed
			{
				logStream (MESSAGE_CRITICAL | MESSAGE_REPORTIT) << "Something is PRETTY WRONG!!! The dome is not open (or responding) and we are not below ceiling... Aborting movement (including park attempts)! It you insist, you can set the 'disable_domeopen_check' to on." << sendLog;
				return -1;
			}
			if (!remotesAbsEncDegDifferenceIgnore->getValueBool () && (remotesAbsEncDegDifference->getRa () > remotesAbsEncDegDifferenceLimit->getValueDouble () || remotesAbsEncDegDifference->getDec () > remotesAbsEncDegDifferenceLimit->getValueDouble ()))
			{
				logStream (MESSAGE_ERROR) << "The dome is not open, which would normaly lead to a movement within the inside_dome limits. But the position from ABS encoders differs too much, so we rather cancel the movement... You can use the 'ignore_remotes_abs_encoder_difference' switch for bypass." << sendLog;
				return -1;
			}
			insideDomeOnlyMovement->setValueBool (true);
		}
		else
		{
			insideDomeOnlyMovement->setValueBool (false);
			logStream (MESSAGE_DEBUG) << "****** startResync () - domestate good" << sendLog;
		}
	}

	retFP = findPath ();
	if (retFP < 0)
	{
		logStream (MESSAGE_ERROR) << "findPath () returned an error, maybe it didn't find a safe route?" << sendLog;
		return -1;
	}

	tAc = successivePointsAC[0];
	dc = successivePointsDC[0];

	// this is a redundant local check, we still do it to be sure not to make a flip and harm ourselfs...
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
	switch (backlashCrossStrategy->getValueInteger ())
	{
		case BACKLASH_CROSS_STRATEGY_TIME_INTERVAL:
			addTimer (moveSpeedBacklashInterval->getValueDouble (), new rts2core::Event (RTS2_D50_BOOSTSPEED));
			break;
		case BACKLASH_CROSS_STRATEGY_ENCODER_TICKS:
			remotesGetAbsolutePosition (remotesRA, &backlashCrossEncoderStartRA);
			remotesGetAbsolutePosition (remotesDec, &backlashCrossEncoderStartDec);
			backlashCrossEncoderStartRA = (backlashCrossEncoderStartRA - remotesAbsEncRAOffset->getValueInteger () + 12288)%8192;	// in encoder units, but normalized and zero in "zeto" (ha/dec = 0/0) position;
			backlashCrossEncoderStartDec = (backlashCrossEncoderStartDec - remotesAbsEncDecOffset->getValueInteger () + 12288)%8192;
			addTimer (0.1, new rts2core::Event (RTS2_D50_BOOSTSPEED));
			break;
	}
	raDrive->setTargetPos (tAc);
	decDrive->setTargetPos (dc);
	// we will realize the RTS2_D50_SUCCESSIVE_POINTS after the backlash will be crossed (in RTS2_D50_BOOSTSPEED timer)
	//if (retFP > 0)		// i.e., more complicated solution within "successive safe points" strategy must be realized
	//	addTimer (successivePointsIntervals[0], new rts2core::Event (RTS2_D50_SUCCESSIVE_POINTS));
	return 0;
}

int D50::isMoving ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** isMoving ()" << sendLog;
        //callAutosave ();
	logStream (MESSAGE_DEBUG) << "------ isMoving: getState=" << getState () << ", tracking=" << trackingRequested () << ", parking=" << parking << ", raDrive->isMoving=" << raDrive->isMoving () << ", raDrive->isInPositionMode=" << raDrive->isInPositionMode () << ", decDrive->isMoving=" << decDrive->isMoving () << sendLog;
#endif
        if ((trackingRequested () != 0) && raDrive->isInPositionMode () && successivePointsActualPosition == numberOfSuccesivePoints)
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
                                tAc = ac + 1/10 / 3600 * 15 * haCpd;	// setting value in advance - the sidereal tracking ("worm") will not be started sooner than in next cycle of isMoving () (the 1/10s corresponds to returned value below)
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
	deleteTimers (RTS2_D50_SUCCESSIVE_POINTS);
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

        parkPos->getAltAz (&park);

        ln_get_equ_from_hrz (&park, &observerLongLat, ln_get_julian_from_sys (), &epark);

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

	struct ln_hrz_posn hrpos;
	hrpos.alt = telAltAz->getAlt ();
	hrpos.az = telAltAz->getAz ();
	if (hardCeiling.is_good (&hrpos))	// !!! we use the "horizon" object, but need a ceiling test, so the logic is reversed
	{
		logStream (MESSAGE_CRITICAL | MESSAGE_REPORTIT) << "THE DOME WILL NOT BE CLOSED, EMERGENCY INTERVENTION NECESSARY!!! Reason: the telescope has parked, but it's position prevents to close the dome." << sendLog;
		return -1;
	}

	// we don't refresh the remotesAbsEncDegDifference values, it's made within the "info ()" function (it's called every 1s during the movement).
	if (!remotesAbsEncDegDifferenceIgnore->getValueBool () && (remotesAbsEncDegDifference->getRa () > 10.0 || remotesAbsEncDegDifference->getDec () > 6.0))	// we set this VERY loose, we do not want to obstruct dome-closing, unless it's really necessary
	{
		logStream (MESSAGE_CRITICAL | MESSAGE_REPORTIT) << "THE DOME WILL NOT BE CLOSED, EMERGENCY INTERVENTION NECESSARY!!! Reason: the telescope has parked (according to the servos), but the position from ABS sensors differs too much... You can use the 'ignore_remotes_abs_encoder_difference' switch to bypass this check." << sendLog;
		return -1;
	}
	else
	{
		insideDomeOnlyMovement->setValueBool (true);
		sendValueAll (insideDomeOnlyMovement);		// to let the DOME know we will not cross it's border :-) so the roof can be closed
	}

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
	else if (old_value == insideDomeOnlyMovement)
	{
		if (new_value->getValueInteger ())
		{
			struct ln_hrz_posn hrpos;
			hrpos.alt = telAltAz->getAlt ();
			hrpos.az = telAltAz->getAz ();
			if (hardCeiling.is_good (&hrpos))	// !!! we use the "horizon" object, but need a ceiling test, so the logic is reversed
			{
				logStream (MESSAGE_ERROR) << "Cannot limit movement to 'inside dome', while not being in this area!" << sendLog;
				return -2;
			}
			if (!remotesAbsEncDegDifferenceIgnore->getValueBool () && (remotesAbsEncDegDifference->getRa () > remotesAbsEncDegDifferenceLimit->getValueDouble () || remotesAbsEncDegDifference->getDec () > remotesAbsEncDegDifferenceLimit->getValueDouble ()))
			{
				logStream (MESSAGE_ERROR) << "Cannot limit movement to 'inside dome', the position from ABS encoders differs too much... If you insist, use the 'ignore_remotes_abs_encoder_difference' switch." << sendLog;
				return -2;
			}
		}
		return 0;
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

int D50::findPath ()
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** findPath ()" << sendLog;
#endif
	int32_t acStart, dcStart, acFin, dcFin;

	double acOnlyMoveTime, dcOnlyMoveTime, movementDelay;

        acStart = raDrive->getPosition ();
        dcStart = decDrive->getPosition ();

	int ret = sky2counts (acFin, dcFin);
	if (ret)
		return -1;

	// The essential approach used for dealing the problematic movements is to change the final target positon of servos gradually,
	// while all the temporary targets (as well as the way to them) are safe and don't collide with the hard-horizon (as we cannot 
	// guarantee that the following change of coordinates will really happen). We call this approach "Successive Safe Targets" strategy.
	//
	// The chosen strategy for finding the path is quite simple, we don't fully solve a complexity of the problem, as our hard-horizon is 
	// quite decent (however, we still add more complexity for potential future usage of "closed dome" constrains, which would add other 
	// "hard-horizon-like" definition for ceiling).
	//
	// For finding the path we prefer movement in HA and enable only it's "toward target" direction, while for Dec axis we accept 
	// variantion in both directions.

	if (checkPassage(acStart, dcStart, acFin, dcFin) > 0)	// simple (straight) movement is possible
	{
		successivePointsAC[0] = acFin;
		successivePointsDC[0] = dcFin;
		numberOfSuccesivePoints = 0;

		successivePointsActualPosition = 0;
		return numberOfSuccesivePoints;
	}

	acOnlyMoveTime = checkPassage(acStart, dcStart, acFin, dcStart);

	if (acOnlyMoveTime > 0)	// HA-only movement is possible, we will follow a "HA-first, Dec-delayed" strategy
	{
		movementDelay = computeSuccessivePointInterval (acStart, dcStart, acFin, dcStart, acFin, dcFin);
		if (movementDelay > 0)
		{
			successivePointsAC[0] = acFin;
			successivePointsDC[0] = dcStart;
			successivePointsIntervals[0] = movementDelay;
			successivePointsAC[1] = acFin;
			successivePointsDC[1] = dcFin;
			numberOfSuccesivePoints = 1;

			successivePointsActualPosition = 0;
			return numberOfSuccesivePoints;
		}
		// We should never get to this point, as the D50's hard horizon is quite decent :-).
		// However, if this happens, it should safe to pass through to the other attempts.
		// TODO: add some debug warning?
	}

	dcOnlyMoveTime = checkPassage(acStart, dcStart, acStart, dcFin);
	if (dcOnlyMoveTime > 0)	// Dec-only movement is possible, we will follow "Dec-first, HA-delayed" strategy (OK, this is probably just for completeness, for current configuration this probably will not be needed)
	{
		movementDelay = computeSuccessivePointInterval (acStart, dcStart, acStart, dcFin, acFin, dcFin);
		if (movementDelay > 0)
		{
			successivePointsAC[0] = acStart;
			successivePointsDC[0] = dcFin;
			successivePointsIntervals[0] = movementDelay;
			successivePointsAC[1] = acFin;
			successivePointsDC[1] = dcFin;
			numberOfSuccesivePoints = 1;

			successivePointsActualPosition = 0;
			return numberOfSuccesivePoints;
		}
		// We should never get to this point, as the D50's hard horizon is quite decent :-).
		// However, if this happens, it should safe to pass through to the other attempts.
		// TODO: add some debug warning?
	}

	// TODO: need to add surely the "SAFE POSITION" or to implement more robust search, which will try to do at least several "jumps" (in both directions) in Dec, yet better would be not to search for the acFin, but enabling multiple successivePoints and to try to minimize abs(acReachable - acFin) by changing the reachable values of Dec (in steps)...

	return -1;
}

double D50::computeSuccessivePointInterval (int32_t fromAC, int32_t fromDC, int32_t toAC, int32_t toDC, int32_t nextAC, int32_t nextDC, double initSpeedAC, double initSpeedDC)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** computeSuccessivePointInterval ()" << sendLog;
#endif
	double initialMoveTime, movementDelay, acSpeedTemp, dcSpeedTemp;
	int32_t acTemp, dcTemp;

	initialMoveTime = computeFullMovementTime (fromAC, fromDC, toAC, toDC, initSpeedAC, initSpeedDC);

	for (movementDelay = FINDPATH_TIME_RESOLUTION; movementDelay <= initialMoveTime; movementDelay += FINDPATH_TIME_RESOLUTION)
	{
		acTemp = computeMovement (0, fromAC, toAC, initSpeedAC, movementDelay, &acSpeedTemp);
		dcTemp = computeMovement (1, fromDC, toDC, initSpeedDC, movementDelay, &dcSpeedTemp);
		if (checkPassage(acTemp, dcTemp, nextAC, nextDC, acSpeedTemp, dcSpeedTemp) > 0)	// we've found the earliest delay of setting the next target!
			return movementDelay;
	}

	return -1;
}

double D50::checkPassage (int32_t fromAC, int32_t fromDC, int32_t toAC, int32_t toDC, double initSpeedAC, double initSpeedDC)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** checkPassage ()" << sendLog;
#endif
	double fullMovementTime, inspectionTime;
	int32_t posAC, posDC;
	bool checkSunDistance;

	if ((getMasterState () & SERVERD_STATUS_MASK) == SERVERD_NIGHT)
		checkSunDistance = false;
	else
		checkSunDistance = true;

	fullMovementTime = computeFullMovementTime (fromAC, fromDC, toAC, toDC, initSpeedAC, initSpeedDC);

	for (inspectionTime = CHECKPASSAGE_TIME_RESOLUTION; inspectionTime <= fullMovementTime; inspectionTime += CHECKPASSAGE_TIME_RESOLUTION)
	{
		posAC = computeMovement (0, fromAC, toAC, initSpeedAC, inspectionTime);
		posDC = computeMovement (1, fromDC, toDC, initSpeedDC, inspectionTime);
#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "------ checkPassage () fromAC/fromDC: " << fromAC << "/" << fromDC << ", toAC:toDC " << toAC << "/" << toDC << ", posAC/posDC: " << posAC << "/" << posDC << ", inspectionTime: " << inspectionTime << sendLog;
#endif
		if (!checkRawPosition (posAC, posDC, checkSunDistance, insideDomeOnlyMovement->getValueBool ()))
			return -1.0 * inspectionTime;
	}

	return fullMovementTime;
}

double D50::computeFullMovementTime (int32_t fromAC, int32_t fromDC, int32_t toAC, int32_t toDC, double initSpeedAC, double initSpeedDC)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** computeFullMovementTime ()" << sendLog;
#endif
	double movementTimeAC, movementTimeDC;

	movementTimeAC = (double) computeMovement (0, fromAC, toAC, initSpeedAC, -1) / 1000.0;
	movementTimeDC = (double) computeMovement (1, fromDC, toDC, initSpeedDC, -1) / 1000.0;
	if (movementTimeAC > movementTimeDC)
		return movementTimeAC;
	return movementTimeDC;
}

int32_t D50::computeMovement (int isDec, int32_t fromCount, int32_t toCount, double v0, double inspectionTime, double* speedAtInspectionTime)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** computeMovement ()" << sendLog;
#endif
	if (fromCount == toCount)
	{
		if (inspectionTime < 0)         // i.e., only the time of whole movement is expected from this routine
			return 0;
		return toCount;
	}

	double v_max, a1, a2, v_physmax;

	if (moveTurboSwitch->getValueBool ())	// first get current maxspeed in degrees/s, we will convert it to counts/s later
		v_max = moveSpeedTurbo->getValueDouble ();
	else
		v_max = moveSpeedFull->getValueDouble ();


	if (isDec == 0)		// i.e. this all relates to RA axis
	{
		v_max = v_max * fabs (haCpd);
		v_physmax = raDrive->getPhysicalSpeedLimit () * (double) SERVO_COUNTS_PER_ROUND;
		a1 = raDrive->getAccel () * (double) SERVO_COUNTS_PER_ROUND;
		a2 = raDrive->getDecel () * (double) SERVO_COUNTS_PER_ROUND;
	}
	else			// i.e. this all relates to Dec axis
	{
		v_max = v_max * fabs (decCpd);
		v_physmax = decDrive->getPhysicalSpeedLimit () * (double) SERVO_COUNTS_PER_ROUND;
		a1 = decDrive->getAccel () * (double) SERVO_COUNTS_PER_ROUND;
		a2 = decDrive->getDecel () * (double) SERVO_COUNTS_PER_ROUND;
	}

	if (v_max > v_physmax)
		v_max = v_physmax;

	double T_ramp1, T_ramp2;
	int32_t S_ramp1, S_ramp2, S_const, movementDirection;

	T_ramp1 = (v_max - v0) / a1;
	T_ramp2 = v_max / a2;
	S_ramp1 = T_ramp1 * (v_max + v0) / 2;
	S_ramp2 = T_ramp2 * v_max / 2;
	S_const = abs (toCount - fromCount) - S_ramp1 - S_ramp2;

	if (toCount > fromCount)
		movementDirection = (int32_t) 1;
	else
		movementDirection = (int32_t) -1;

	if (S_const >= 0)	// i.e., we reach the full speed during the transition
	{
		double T_const = (double) S_const/v_max;
		if (inspectionTime < 0)		// i.e., only the time of whole movement is expected from this routine
		{
			return (int32_t) ((T_ramp1 + T_ramp2 + T_const) * 1000.0 + 0.5);
		}

		if (inspectionTime <= T_ramp1)
		{
			if (speedAtInspectionTime != NULL)
				*speedAtInspectionTime = v0 + a1 * inspectionTime;
			return fromCount + movementDirection * (int32_t) ((v0 + a1 * inspectionTime / 2) * inspectionTime + 0.5);
		}
		if (inspectionTime <= T_ramp1 + T_const)
		{
			if (speedAtInspectionTime != NULL)
				*speedAtInspectionTime = v_max;
			return fromCount + movementDirection * (int32_t) (S_ramp1 + v_max * (inspectionTime - T_ramp1) + 0.5);
		}
		if (inspectionTime <= T_ramp1 + T_const + T_ramp2)
		{
			double T_reversed = T_ramp1 + T_ramp2 + T_const - inspectionTime;
			if (speedAtInspectionTime != NULL)
				*speedAtInspectionTime = a2 * T_reversed;
			return fromCount + movementDirection * (int32_t) (S_ramp1 + S_const + S_ramp2 - a2 * T_reversed * T_reversed / 2.0 + 0.5);
		}
		if (speedAtInspectionTime != NULL)
			*speedAtInspectionTime = 0.0;
		return toCount;
	}
	// case when S_const<0, i.e., full speed is not reached during the transition
	double T_full = ((1.0 + a1/a2) * sqrt ((v0*v0 + 2*a1*abs(toCount-fromCount)) / (1.0 + a1/a2)) - v0) / a1;
	double T1 = (T_full * a2 - v0)/(a1 + a2);
	if (inspectionTime < 0)		// i.e., only the time of whole movement is expected from this routine
		return (int32_t) (T_full * 1000.0 + 0.5);
	if (inspectionTime <=  T1)
	{
		if (speedAtInspectionTime != NULL)
			*speedAtInspectionTime = v0 + a1 * inspectionTime;
		return fromCount + movementDirection * (int32_t) ((v0 + a1 * inspectionTime / 2) * inspectionTime + 0.5);
	}
	if (inspectionTime <=  T_full)
	{
		double T_reversed = T_full - inspectionTime;
		if (speedAtInspectionTime != NULL)
			*speedAtInspectionTime = a2 * T_reversed;
		return fromCount + movementDirection * (int32_t) (abs(toCount-fromCount) - a2 * T_reversed * T_reversed / 2.0 + 0.5);
	}
	if (speedAtInspectionTime != NULL)
		*speedAtInspectionTime = 0.0;
	return toCount;
}

int D50::checkRawPosition (int32_t AC, int32_t DC, bool checkSunDistance, bool checkCeiling)
{
#ifdef DEBUG_BRUTAL
	logStream (MESSAGE_DEBUG) << "****** checkRawPosition ()" << sendLog;
#endif
	struct ln_equ_posn equPosition;
	struct ln_hrz_posn hrzPosition;

	equPosition.ra = - ( (double) (AC / haCpd) + haZero);
	equPosition.dec = (double) (DC / decCpd) + decZero;

	ln_get_hrz_from_equ_sidereal_time (&equPosition, &observerLongLat, -observerLongLat.lng/15.0, &hrzPosition);
#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "------ checkRawPosition () ac/dc: " << AC << "/" << DC << ", ra0/dec: " << equPosition.ra << "/" << equPosition.dec << ", alt/az: " << hrzPosition.alt << "/" << hrzPosition.az << sendLog;
#endif

	if (!hardHorizon.is_good (&hrzPosition))
		return 0;

	if (checkCeiling)
	{
#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "------ checkRawPosition () checkCeiling"  << sendLog;
#endif
		if (hardCeiling.is_good (&hrzPosition))		// !!! we use the "horizon" object, but need a ceiling test, so the logic is reversed
			return 0;
	}
	else if (checkSunDistance)
	{
		double JD;
		struct ln_equ_posn eq_sun;
		JD = ln_get_julian_from_sys ();
		ln_get_solar_equ_coords (JD, &eq_sun);
		equPosition.ra = equPosition.ra + ln_get_mean_sidereal_time (JD)*15.0 + observerLongLat.lng;
#ifdef DEBUG_EXTRA
		logStream (MESSAGE_DEBUG) << "------ checkRawPosition () angular distance to Sun: " << ln_get_angular_separation (&eq_sun, &equPosition) << ", ra/dec: " << equPosition.ra << "/" << equPosition.dec << sendLog;
#endif
		if (ln_get_angular_separation (&eq_sun, &equPosition) < SUN_AVOID_DISTANCE)
			return 0;
	}
	return 1;
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

int D50::getDomeState ()
{
	rts2core::Connection *connDome = NULL;
	rts2core::Value *tmpValue = NULL;

	logStream (MESSAGE_DEBUG) << "getDomeState: A1" << sendLog;
	connDome = getOpenConnection (domeDevice);
	if (connDome == NULL)
	{
		logStream (MESSAGE_ERROR) << "getDomeState: device connection error!" << sendLog;
		return -1;
	}
        if (connDome->getConnState () != CONN_AUTH_OK && connDome->getConnState () != CONN_CONNECTED)
	{
		logStream (MESSAGE_ERROR) << "getDomeState: device connection not ready!" << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "getDomeState: A2" << sendLog;
	tmpValue = connDome->getValue ("infotime");
	if (tmpValue == NULL)
	{
		logStream (MESSAGE_ERROR) << "getDomeState: infotime is NULL..." << sendLog;
		return -1;
	}
	if (getNow () - tmpValue->getValueDouble () > 12.0)
	{
		logStream (MESSAGE_ERROR) << "getDomeState: infotime too old: " << getNow () - tmpValue->getValueDouble () << "s" << sendLog;
		return -1;
	}

	logStream (MESSAGE_DEBUG) << "getDomeState: A3" << sendLog;
	tmpValue = NULL;
	tmpValue = connDome->getValue (domeDeviceOpenswitchVariable);
	if (tmpValue == NULL)
	{
		logStream (MESSAGE_ERROR) << "getDomeState: wrong openswitch variable name?" << sendLog;
		return -1;
	}
	logStream (MESSAGE_DEBUG) << "getDomeState: A4" << sendLog;
	if (tmpValue->getValueInteger () == 1)
	{
		return 0;	// dome is fully open!
	}
	logStream (MESSAGE_DEBUG) << "getDomeState: A5" << sendLog;
	return 1; // dome is closed or not fully open
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
