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

#define DEBUG_EXTRA

#include "connection/tgdrive.h"
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

		virtual int stopWorm ();
		virtual int startWorm ();

		virtual void setDiffTrack (double dra, double ddec);

		virtual int updateLimits ();
		virtual int getHomeOffset (int32_t & off);

		virtual int setValue (rts2core::Value * old_value, rts2core::Value * new_value);
	private:
		int runD50 ();
		TGDrive *raDrive;
		TGDrive *decDrive;

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

		rts2core::ValueBool *remotesMotorsPower;
		rts2core::ValueBool *remotesMotorsExternalEnable;
		rts2core::ValueBool *remotesWormPressureLimiter;
		rts2core::ValueBool *remotesWormStepsGenerator;
		//rts2core::ValueInteger *remotesWormStepsSpeed;

		int32_t dcMin, dcMax;


		/*const char *device_name;
		rts2core::ConnSerial *d50Conn;

		rts2core::ValueBool *motorRa;
		rts2core::ValueBool *motorDec;

		rts2core::ValueBool *wormRa;
		rts2core::ValueBool *wormDec;

		rts2core::ValueInteger *wormRaSpeed;

		rts2core::ValueBool *moveSleep;

		rts2core::ValueInteger *unitRa;
		rts2core::ValueInteger *unitDec;

		rts2core::ValueInteger *utarRa;
		rts2core::ValueInteger *utarDec;

		rts2core::ValueInteger *procRa;
		rts2core::ValueInteger *procDec;

		rts2core::ValueInteger *velRa;
		rts2core::ValueInteger *velDec;

		rts2core::ValueInteger *accRa;
		rts2core::ValueInteger *accDec;

		int32_t ac, dc;*/
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
	
	createRaGuide ();
	createDecGuide ();
	
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

	createValue (remotesMotorsPower, "remotes_motors_power", "el. power to both motors", false, RTS2_VALUE_WRITABLE);
	remotesMotorsPower->setValueBool (false);
	createValue (remotesMotorsExternalEnable, "remotes_motors_external_enable", "external enable switch to both motors", false, RTS2_VALUE_WRITABLE);
	remotesMotorsExternalEnable->setValueBool (false);
	createValue (remotesWormPressureLimiter, "remotes_worm_pressure_limiter", "decrease pressure on worm (for slew)", false, RTS2_VALUE_WRITABLE);
	remotesWormPressureLimiter->setValueBool (false);
	createValue (remotesWormStepsGenerator, "remotes_worm_steps_generator", "generator of steps for HA tracking", false, RTS2_VALUE_WRITABLE);
	remotesWormStepsGenerator->setValueBool (false);
	//createValue (remotesWormStepsSpeed, "remotes_worm_steps_speed", "speed of stepper generator for HA tracking", false, RTS2_VALUE_WRITABLE);
	//remotesWormStepsSpeed->setValueInteger ();


	//createValue (wormRaSpeed, "worm_ra_speed", "speed in 25000/x steps per second", false, RTS2_VALUE_WRITABLE);

	//reateValue (moveSleep, "move_sleep", "sleep this number of seconds after finishing", false, RTS2_VALUE_WRITABLE);
	//moveSleep->setValueInteger (7);

	// apply all corrections by rts2 for mount
	setCorrections (true, true, true);
}

D50::~D50 (void)
{
	delete raDrive;
	delete decDrive;
}

void D50::postEvent (rts2core::Event *event)
{
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

	ret = Fork::init ();
	if (ret)
		return ret;

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

	rts2core::Configuration *config = rts2core::Configuration::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	telLongitude->setValueDouble (config->getObserver ()->lng);
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	// zero dec is on local meridian, 90 - telLatitude bellow (to nadir)
	decZero = 90.0 - fabs (telLatitude->getValueDouble ());
	if (telLatitude->getValueDouble () > 0)
		decZero *= -1.0;
								 // south hemispehere
	if (telLatitude->getValueDouble () < 0)
	{
		// swap values which are opposite for south hemispehere
	}

	snprintf (telType, 64, "D50");

	/*
	// switch both motors off
	ret = write_both ("D\x0d", 2);
	if (ret)
		return ret;
	motorRa->setValueBool (false);
	motorDec->setValueBool (false);

	ret = tel_write_unit (1, 'h', 25000);
	if (ret)
		return ret;
	wormRaSpeed->setValueInteger (25000);
	*/

	// TODO: mozna zastaveni (plynule) motoru, vypnuti hodinace, vypnuti limiteru pritlaku, uplne vypnuti serv, nastaveni rychlosti hodinace?
	
	//addTimer (1, new rts2core::Event (RTS2_D50_AUTOSAVE));

	return 0;
}

int D50::info ()
{
        raDrive->info ();
        decDrive->info ();

        double t_telRa;
        double t_telDec;
        int32_t raPos = raDrive->getPosition ();
        int32_t decPos = decDrive->getPosition ();
        counts2sky (raPos, decPos, t_telRa, t_telDec);
        setTelRa (t_telRa);
        setTelDec (t_telDec);

	return Fork::info ();
}

int D50::idle ()
{
	runD50 ();

	return Fork::idle ();
}

int D50::runD50 ()
{
	if (!(getState () & TEL_MOVING))
	{
		if ((!tracking->getValueBool ()) && raDrive->isInStepperMode ())
		{
                        raDrive->setTargetSpeed ( 0, true );
                        raDrive->setMode (TGA_MODE_DS);
                        raDrive->info ();
		} else if (tracking->getValueBool () && !raDrive->isInStepperMode ()) 
		{
			if (!raDrive->isMoving ())
			{
				raDrive->setMode (TGA_MODE_SM);
                        	raDrive->info ();
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
        //deleteTimers (RTS2_D50_AUTOSAVE);
        if (!tracking->getValueBool () && !parking)
	{
                startWorm ();
	}
        int32_t dc;
        int ret = sky2counts (tAc, dc);
        if (ret)
                return -1;
	// this maight be redundant local check, we make to be sure not make flip and harm ourselfs...
	if (dc < dcMin || dc > dcMax)
	{
		logStream (MESSAGE_ERROR) << "dc value out of limits!" << sendLog;
		return -1;
	}
	//TODO: zapnout snizeni pritlaku sneku
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	addTimer (moveSpeedBacklashInterval->getValueDouble (), new rts2core::Event (RTS2_D50_BOOSTSPEED));
        raDrive->setTargetPos (tAc);
        decDrive->setTargetPos (dc);
        return 0;
}

int D50::isMoving ()
{
        callAutosave ();
        /*if ((getState () & TEL_MOVING) && !tracking->getValueBool () && !parking)
	{
                startWorm ();
	}*/
        if (tracking->getValueBool () && raDrive->isInPositionMode ())
        {
                if (raDrive->isMovingPosition ())
                {
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
                                return 0;
                        }
                }
                else
                {
                        //raDrive->setTargetSpeed (TRACK_SPEED);
                        raDrive->setMode (TGA_MODE_SM);
                }
        }
        if ((tracking->getValueBool () && raDrive->isInPositionMode ()) || (!tracking->getValueBool () && raDrive->isMoving ()) || decDrive->isMoving ())
                return 0;
        return -2;
}

int D50::endMove ()
{
	//addTimer (5, new rts2core::Event (RTS2_D50_AUTOSAVE));
	// TODO: tady bude vypnuti snizovani pritlaku sneku
	// snizeni rychlosti presunu obou motoru
	deleteTimers (RTS2_D50_BOOSTSPEED);
	raDrive->setMaxSpeed (RA_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	decDrive->setMaxSpeed (DEC_TRANSMISION / 360.0 * moveSpeedBacklash->getValueDouble ());
	setTimeout (USEC_SEC);
	return Fork::endMove ();
}

int D50::stopMove ()
{
        //addTimer (5, new rts2core::Event (RTS2_D50_AUTOSAVE));
	stopWorm ();
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
        return 0;
}

int D50::setTo (double set_ra, double set_dec)
{
        struct ln_equ_posn eq;
        eq.ra = set_ra;
        eq.dec = set_dec;
        int32_t ac;
        int32_t dc;
        int32_t off;
        getHomeOffset (off);
	zeroCorrRaDec ();
        int ret = sky2counts (&eq, ac, dc, ln_get_julian_from_sys (), off);
        if (ret)
                return -1;
        //if (tracking->getValueBool ())
	//{
	//	raDrive->setMode (TGA_MODE_DS);
	//	raDrive->setTargetSpeed ( 0, true );
	//}
        raDrive->setCurrentPos (ac);
        decDrive->setCurrentPos (dc);
        callAutosave ();
        if (tracking->getValueBool ())
                raDrive->setMode (TGA_MODE_SM);
                //raDrive->setTargetSpeed (TRACK_SPEED);
        return 0;
}

int D50::setToPark ()
{
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
        stopWorm ();
        if (parkPos)
        {
                parking = true;
                setTargetAltAz (parkPos->getAlt (), parkPos->getAz ());
                int ret = moveAltAz ();
                return ret;
        }
        else
                return -1;
}

int D50::endPark ()
{
        callAutosave ();
        parking = false;
	setTimeout (USEC_SEC * 10);
        return 0;
}

int D50::stopWorm ()
{
	if (tracking->getValueBool ())
	{
		tracking->setValueBool (false);
		sendValueAll (tracking);
	}
        return 0;
}

int D50::startWorm ()
{
	if (!tracking->getValueBool ())
	{
		tracking->setValueBool (true);
		sendValueAll (tracking);
	}
        return 0;
}

void D50::setDiffTrack (double dra, double ddec)
{
        if (parking)
                return;
        if (info ())
                throw rts2core::Error ("cannot call info in setDiffTrack");
        if (!raDrive->isInPositionMode () || !raDrive->isMovingPosition ())
        {
                if (tracking->getValueBool ())
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
        if (info ())
                return;
        autosaveValues ();
}

int D50::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
        if (old_value == raGuide)
        {
                //matchGuideRa (new_value->getValueInteger ());
                return 0;
        }
        /*else if (old_value == decGuide)
        {
                matchGuideDec (new_value->getValueInteger ());
                return 0;
        }*/
        else if (old_value == tracking)
        {
                //raDrive->setTargetSpeed (((rts2core::ValueBool *) new_value)->getValueBool () ? TRACK_SPEED : 0, false);
                if (((rts2core::ValueBool *) new_value)->getValueBool ())
                {
                        startWorm ();
                }
                else
                {
			stopWorm ();
                }
		return 0;
        }
	/* else if (old_value == wormRa)
	{
		return tel_write_unit (1,
			((rts2core::ValueBool *) new_value)->getValueBool ()? "o0" : "c0", 2) == 0 ? 0 : -2;

	}
	else if (old_value == wormRaSpeed)
	{
		return tel_write_unit (1, 'h', new_value->getValueInteger ()) == 0 ? 0 : -2;
	}*/
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
	if (moveTurboSwitch->getValueBool ())
	{
		moveTurboSwitch->setValueBool (false);
	}
	return Telescope::scriptEnds ();
}

int main (int argc, char **argv)
{
	D50 device = D50 (argc, argv);
	return device.run ();
}
