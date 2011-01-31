/* 
 * Driver for Hlohovec (Slovakia) 50cm telescope.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "tgdrive.h"
#include "gem.h"

#include "../utils/rts2config.h"

#define OPT_RA                   OPT_LOCALHOST + 2201
#define OPT_DEC                  OPT_LOCALHOST + 2202

#define RTS2_HLOHOVEC_TIMERRG    RTS2_LOCAL_EVENT + 1210
#define RTS2_HLOHOVEC_TIMERDG    RTS2_LOCAL_EVENT + 1211
#define RTS2_HLOHOVEC_TSTOPRG    RTS2_LOCAL_EVENT + 1212
#define RTS2_HLOHOVEC_TSTOPDG    RTS2_LOCAL_EVENT + 1213

// steps per full RA and DEC revolutions (360 degrees)
#define RA_TICKS                 1000000000
#define DEC_TICKS                1000000000

#define RAGSTEP                  100000000
#define DEGSTEP                  100000000

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

		virtual void postEvent (Rts2Event *event);
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
		virtual int startPark ();
		virtual int endPark ();

		virtual int updateLimits ();
		virtual int getHomeOffset (int32_t & off);

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);
	private:
		TGDrive *raDrive;
		TGDrive *decDrive;

		const char *devRA;
		const char *devDEC;

		void matchGuideRa (int rag);
		void matchGuideDec (int decg);
};

}

Hlohovec::Hlohovec (int argc, char **argv):GEM (argc, argv)
{
	raDrive = NULL;
	decDrive = NULL;

	devRA = NULL;
	devDEC = NULL;

	ra_ticks = RA_TICKS;
	dec_ticks = DEC_TICKS;

	haCpd = RA_TICKS / 180.0;
	decCpd = DEC_TICKS / 180.0;

	acMargin = haCpd;
	
	haZero = decZero = 0;

	createRaGuide ();
	createDecGuide ();

	addOption (OPT_RA, "ra", 1, "RA drive serial device");
	addOption (OPT_DEC, "dec", 1, "DEC drive serial device");
}

Hlohovec::~Hlohovec ()
{
	delete raDrive;
	delete decDrive;
}

void Hlohovec::postEvent (Rts2Event *event)
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
	}
	GEM::postEvent (event);
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
	if (printDebug ())
		raDrive->setDebug ();
	raDrive->setLogAsHex ();
	ret = raDrive->init ();
	if (ret)
		return ret;


	decDrive = new TGDrive (devDEC, "DEC_", this);
	if (printDebug ())
		decDrive->setDebug ();
	decDrive->setLogAsHex ();
	ret = decDrive->init ();
	if (ret)
		return ret;

	Rts2Config *config = Rts2Config::instance ();
	ret = config->loadFile ();
	if (ret)
		return -1;

	telLongitude->setValueDouble (config->getObserver ()->lng);
	telLatitude->setValueDouble (config->getObserver ()->lat);
	telAltitude->setValueDouble (config->getObservatoryAltitude ());

	return 0;
}

int Hlohovec::info ()
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
	int32_t ac;
	int32_t dc;
	int ret = sky2counts (ac, dc);
	if (ret)
		return -1;
	raDrive->setTargetPos (ac);
	decDrive->setTargetPos (dc);
	return 0;
}

int Hlohovec::isMoving ()
{
	if (raDrive->isMoving () || decDrive->isMoving ())
		return USEC_SEC / 100;
	return -2;
}

int Hlohovec::endMove ()
{
	return 0;
}

int Hlohovec::stopMove ()
{
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
	return 0;
}

int Hlohovec::startPark ()
{
	return 0;
}

int Hlohovec::endPark ()
{
	return 0;
}

int Hlohovec::updateLimits ()
{
	acMin = -RA_TICKS;
	acMax = RA_TICKS;
	dcMin = -DEC_TICKS;
	dcMax = DEC_TICKS;
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
		addTimer (0.1, new Rts2Event (RTS2_HLOHOVEC_TSTOPRG));
	}
	else
	{
		raDrive->setTargetPos (raDrive->getPosition () + ((rag == 1 ? -1 : 1) * RAGSTEP));
		addTimer (1, new Rts2Event (RTS2_HLOHOVEC_TIMERRG));
	}
}

void Hlohovec::matchGuideDec (int deg)
{
	decDrive->info ();
	if (deg == 0)
	{
		decDrive->stop ();
		addTimer (0.1, new Rts2Event (RTS2_HLOHOVEC_TSTOPDG));
	}
	else
	{
		decDrive->setTargetPos (decDrive->getPosition () + ((deg == 1 ? -1 : 1) * DEGSTEP));
		addTimer (1, new Rts2Event (RTS2_HLOHOVEC_TIMERDG));
	}
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
