/* 
 * Driver for Hlohovec (Slovakia) 50cm telescope.
 * Copyright (C) 2009,2011 Petr Kubanek <petr@kubanek.net>
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

#include "connection/tgdrive.h"
#include "gem.h"

#include "configuration.h"

#define OPT_RA                OPT_LOCAL + 2201
#define OPT_DEC               OPT_LOCAL + 2202

#define RTS2_HLOHOVEC_TIMERRG    RTS2_LOCAL_EVENT + 1210
#define RTS2_HLOHOVEC_TIMERDG    RTS2_LOCAL_EVENT + 1211
#define RTS2_HLOHOVEC_TSTOPRG    RTS2_LOCAL_EVENT + 1212
#define RTS2_HLOHOVEC_TSTOPDG    RTS2_LOCAL_EVENT + 1213

#define RTS2_HLOHOVEC_AUTOSAVE   RTS2_LOCAL_EVENT + 1214

// steps per full RA and DEC revolutions (360 degrees)
#define RA_TICKS                 (-14350 * 65535)
#define DEC_TICKS                (-10400 * 65535)

#define RAGSTEP                  1000
#define DEGSTEP                  1000

// track is 15 arcsec/second
#define TRACK_SPEED              (-TGA_SPEEDFACTOR / 6.0)
// track one arcdeg in second
#define SPEED_ARCDEGSEC		 (TRACK_SPEED * (4.0 * 60.0))

using namespace rts2teld;

namespace rts2teld
{

/**
 * Class for Hlohovec (50cm) telescope.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Hlohovec:public GEM
{
	public:
		Hlohovec (int argc, char **argv);
		virtual ~Hlohovec ();

		virtual void postEvent (rts2core::Event *event);

		virtual int commandAuthorized (rts2core::Connection * conn);
	protected:
		virtual void usage ();
		virtual int processOption (int opt);
		virtual int init ();
		virtual int info ();

		virtual int resetMount ();

		virtual int startResync ();
		virtual int isMoving ();
		virtual int endMove ();
		virtual int stopMove ();
		virtual int setTo (double set_ra, double set_dec);
		virtual int setToPark ();
		virtual int startPark ();
		virtual int isParking () { return isMoving (); }
		virtual int endPark ();

		virtual void setDiffTrack (double dra, double ddec);

		virtual int updateLimits ();
		virtual int getHomeOffset (int32_t & off);

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
	private:
		TGDrive *raDrive;
		TGDrive *decDrive;

		int32_t tAc;

		const char *devRA;
		const char *devDEC;

		rts2core::ValueDouble *moveTolerance;

		void matchGuideRa (int rag);
		void matchGuideDec (int decg);

		void callAutosave ();

		bool parking;
};

}

Hlohovec::Hlohovec (int argc, char **argv):GEM (argc, argv, true, true)
{
	raDrive = NULL;
	decDrive = NULL;

	devRA = NULL;
	devDEC = NULL;

	ra_ticks = RA_TICKS;
	dec_ticks = DEC_TICKS;

	haCpd = RA_TICKS / 360.0;
	decCpd = DEC_TICKS / 360.0;

	acMargin = haCpd;
	
	haZero = decZero = 0;

	parking = false;

	createRaGuide ();
	createDecGuide ();

	createValue (moveTolerance, "move_tolerance", "[deg] minimal movement distance", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	moveTolerance->setValueDouble (4.0 / 60.0);

	addOption (OPT_RA, "ra", 1, "RA drive serial device");
	addOption (OPT_DEC, "dec", 1, "DEC drive serial device");

	addParkPosOption ();
}

Hlohovec::~Hlohovec ()
{
	delete raDrive;
	delete decDrive;
}

void Hlohovec::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case RTS2_HLOHOVEC_TIMERRG:
			matchGuideRa (raGuide->getValueInteger ());
			break;
		case RTS2_HLOHOVEC_TIMERDG:
			matchGuideDec (decGuide->getValueInteger ());
			break;
		case RTS2_HLOHOVEC_TSTOPRG:
			if (raDrive->checkStop ())
			{
				addTimer (0.1, event);
				return;
			}
			break;
		case RTS2_HLOHOVEC_TSTOPDG:
			if (decDrive->checkStop ())
			{
				addTimer (0.1, event);
				return;
			}
			break;
		case RTS2_HLOHOVEC_AUTOSAVE:
			callAutosave ();
			if ((getState () & TEL_MASK_MOVING) == TEL_OBSERVING)
			{
				addTimer (5, event);
				return;
			}
			break;
	}
	GEM::postEvent (event);
}

int Hlohovec::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("writeeeprom"))
	{
		raDrive->write2b (TGA_MASTER_CMD, 3);
		decDrive->write2b (TGA_MASTER_CMD, 3);
		return 0;
	}
	return GEM::commandAuthorized (conn);
}

void Hlohovec::usage ()
{
	std::cout << "\t" << getAppName () << " --ra /dev/ttyS0 --dec /dev/ttyS1" << std::endl;
}

int Hlohovec::processOption (int opt)
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
			return GEM::processOption (opt);
	}
	return 0;
}

int Hlohovec::init ()
{
	int ret;
	ret = GEM::init ();
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

	addTimer (1, new rts2core::Event (RTS2_HLOHOVEC_AUTOSAVE));

	return 0;
}

int Hlohovec::info ()
{
	raDrive->info ();
	decDrive->info ();

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

	return GEM::info ();
}

int Hlohovec::resetMount ()
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

	return GEM::resetMount ();
}

int Hlohovec::startResync ()
{
	deleteTimers (RTS2_HLOHOVEC_AUTOSAVE);
	if (getState () & TEL_MOVING)
		tracking->setValueBool (true);
	int32_t dc;
	int ret = sky2counts (tAc, dc);
	if (ret)
		return -1;
	raDrive->setTargetPos (tAc);
	decDrive->setTargetPos (dc);
	return 0;
}

int Hlohovec::isMoving ()
{
	callAutosave ();
	if (getState () & TEL_MOVING)
		tracking->setValueBool (true);
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
			// if difference in H is greater then 1 arcmin..
			if (fabs (diffAc) > haCpd * moveTolerance->getValueDouble ())
			{
				raDrive->setTargetPos (ac);
				tAc = ac;
				return 0;
			}
		}
		else
		{
			raDrive->setTargetSpeed (TRACK_SPEED);
		}
	}
	if ((tracking->getValueBool () && raDrive->isInPositionMode ()) || (!tracking->getValueBool () && raDrive->isMoving ()) || decDrive->isMoving ())
		return 0;
	return -2;
}

int Hlohovec::endMove ()
{
	addTimer (5, new rts2core::Event (RTS2_HLOHOVEC_AUTOSAVE));
	return Telescope::endMove ();
}

int Hlohovec::stopMove ()
{
	addTimer (5, new rts2core::Event (RTS2_HLOHOVEC_AUTOSAVE));
	parking = false;
	raDrive->stop ();
	decDrive->stop ();
	return 0;
}

int Hlohovec::setTo (double set_ra, double set_dec)
{
	struct ln_equ_posn eq;
	eq.ra = set_ra;
	eq.dec = set_dec;
	int32_t ac;
	int32_t dc;
	int32_t off;
	getHomeOffset (off);
	int ret = sky2counts (&eq, ac, dc, ln_get_julian_from_sys (), off);
	if (ret)
		return -1;
	raDrive->setCurrentPos (ac);
	decDrive->setCurrentPos (dc);
	callAutosave ();
	if (tracking->getValueBool ())
		raDrive->setTargetSpeed (TRACK_SPEED);
	return 0;
}

int Hlohovec::setToPark ()
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

int Hlohovec::startPark ()
{
	tracking->setValueBool (false);
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

int Hlohovec::endPark ()
{
	callAutosave ();
	parking = false;
	return 0;
}

void Hlohovec::setDiffTrack (double dra, double ddec)
{
	if (parking)
		return;
	if (info ())
		throw rts2core::Error ("cannot call info in setDiffTrack");
	if (!raDrive->isInPositionMode () || !raDrive->isMovingPosition ())
	{
		if (tracking->getValueBool ())
			raDrive->setTargetSpeed (TRACK_SPEED + dra * SPEED_ARCDEGSEC);
		else
			raDrive->setTargetSpeed (dra * SPEED_ARCDEGSEC);
	}
	if (!decDrive->isInPositionMode () || !decDrive->isMovingPosition ())
	{
		decDrive->setTargetSpeed (ddec * SPEED_ARCDEGSEC);
	}
}

int Hlohovec::updateLimits ()
{
	acMin = -1 * labs (RA_TICKS);
	acMax = labs (RA_TICKS);
	dcMin = -1 * labs (DEC_TICKS);
	dcMax = labs (DEC_TICKS);
	return 0;
}

int Hlohovec::getHomeOffset (int32_t & off)
{
	off = 0;
	return 0;
}

void Hlohovec::matchGuideRa (int rag)
{
	raDrive->info ();
	if (rag == 0)
	{
		raDrive->stop ();
		addTimer (0.1, new rts2core::Event (RTS2_HLOHOVEC_TSTOPRG));
	}
	else
	{
		raDrive->setTargetPos (raDrive->getPosition () + ((rag == 1 ? -1 : 1) * RAGSTEP));
		addTimer (1, new rts2core::Event (RTS2_HLOHOVEC_TIMERRG));
	}
}

void Hlohovec::matchGuideDec (int deg)
{
	decDrive->info ();
	if (deg == 0)
	{
		decDrive->stop ();
		addTimer (0.1, new rts2core::Event (RTS2_HLOHOVEC_TSTOPDG));
	}
	else
	{
		decDrive->setTargetPos (decDrive->getPosition () + ((deg == 1 ? -1 : 1) * DEGSTEP));
		addTimer (1, new rts2core::Event (RTS2_HLOHOVEC_TIMERDG));
	}
}

void Hlohovec::callAutosave ()
{
	if (info ())
		return;
	autosaveValues ();
}

int Hlohovec::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == raGuide)
	{
		matchGuideRa (new_value->getValueInteger ());
		return 0;
	}
	else if (old_value == decGuide)
	{
		matchGuideDec (new_value->getValueInteger ());
		return 0;
	}
	else if (old_value == tracking)
	{
		raDrive->setTargetSpeed (((rts2core::ValueBool *) new_value)->getValueBool () ? TRACK_SPEED : 0, false);
	}
	int ret = raDrive->setValue (old_value, new_value);
	if (ret != 1)
		return ret;
	ret = decDrive->setValue (old_value, new_value);
	if (ret != 1)
		return ret;
	
	return GEM::setValue (old_value, new_value);
}

int main (int argc, char **argv)
{
	Hlohovec device = Hlohovec (argc, argv);
	return device.run ();
}
