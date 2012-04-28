#include "cupola.h"
#include "configuration.h"

#include <math.h>

Cupola::Cupola (int in_argc, char **in_argv):Dome (in_argc, in_argv, DEVICE_TYPE_CUPOLA)
{
	targetPos.ra = rts2_nan ("f");
	targetPos.dec = rts2_nan ("f");

	createValue (tarRa, "tar_ra", "cupola target ra", false, RTS2_DT_RA);
	createValue (tarDec, "tar_dec", "cupola target dec", false, RTS2_DT_DEC);
	createValue (tarAlt, "tar_alt", "cupola target altitude", false,
		RTS2_DT_DEC);
	createValue (tarAz, "tar_az", "cupola target azimut", false,
		RTS2_DT_DEGREES);

	createValue (currentAz, "CUP_AZ", "cupola azimut", true, RTS2_DT_DEGREES);

	targetDistance = 0;

	configFile = NULL;

	addOption ('c', "config", 1, "configuration file");
}

int Cupola::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'c':
			configFile = optarg;
			break;
		default:
			return Dome::processOption (in_opt);
	}
	return 0;
}

int Cupola::init ()
{
	int ret;
	ret = Dome::init ();
	if (ret)
		return ret;

	// get config values..
	rts2core::Configuration *config;
	config = rts2core::Configuration::instance ();
	config->loadFile ();
	observer = config->getObserver ();
	return 0;
}

int Cupola::info ()
{
	struct ln_hrz_posn hrz;
	// target ra+dec
	tarRa->setValueDouble (targetPos.ra);
	tarDec->setValueDouble (targetPos.dec);
	getTargetAltAz (&hrz);
	tarAlt->setValueDouble (hrz.alt);
	tarAz->setValueDouble (hrz.az);

	return Dome::info ();
}

int Cupola::idle ()
{
	long ret;
	if ((getState () & DOME_CUP_MASK_MOVE) == DOME_CUP_MOVE)
	{
		ret = isMoving ();
		if (ret >= 0)
			setTimeout (ret);
		else
		{
			infoAll ();
			if (ret == -2)
				moveEnd ();
			else
				moveStop ();
		}
	}
	else if (needSlitChange () > 0)
	{
		moveStart ();
		setTimeout (USEC_SEC);
	}
	else
	{
		setTimeout (10 * USEC_SEC);
	}
	return Dome::idle ();
}

int Cupola::moveTo (rts2core::Connection * conn, double ra, double dec)
{
	int ret;
	targetPos.ra = ra;
	targetPos.dec = dec;
	infoAll ();
	ret = moveStart ();
	if (ret)
		return ret;
	maskState (DOME_CUP_MASK_SYNC | BOP_EXPOSURE, DOME_CUP_NOT_SYNC | BOP_EXPOSURE);
	return 0;
}

int Cupola::moveStart ()
{
	maskState (DOME_CUP_MASK_MOVE, DOME_CUP_MOVE);
	return 0;
}

int Cupola::moveStop ()
{
	maskState (DOME_CUP_MASK | BOP_EXPOSURE,
		DOME_CUP_NOT_MOVE | DOME_CUP_NOT_SYNC);
	infoAll ();
	return 0;
}

void Cupola::synced ()
{
	infoAll ();
	maskState (DOME_CUP_MASK_SYNC | BOP_EXPOSURE, DOME_CUP_SYNC);
}

int Cupola::moveEnd ()
{
	maskState (DOME_CUP_MASK | BOP_EXPOSURE, DOME_CUP_NOT_MOVE | DOME_CUP_SYNC);
	infoAll ();
	return 0;
}

void Cupola::getTargetAltAz (struct ln_hrz_posn *hrz)
{
	double JD;
	JD = ln_get_julian_from_sys ();
	ln_get_hrz_from_equ (&targetPos, observer, JD, hrz);
}

bool Cupola::needSlitChange ()
{
	int ret;
	struct ln_hrz_posn targetHrz;
	double splitWidth;
	if (isnan (targetPos.ra) || isnan (targetPos.dec))
		return false;
	getTargetAltAz (&targetHrz);
	splitWidth = getSplitWidth (targetHrz.alt);
	if (splitWidth < 0)
		return false;
	// get current az
	ret = info ();
	if (ret)
		return false;
	// simple check; can be repleaced by some more complicated for more complicated setups
	targetDistance = getCurrentAz () - targetHrz.az;
	if (targetDistance > 180)
		targetDistance = (targetDistance - 360);
	else if (targetDistance < -180)
		targetDistance = (targetDistance + 360);
	if (fabs (targetDistance) < splitWidth)
	{
		if ((getState () & DOME_CUP_MASK_SYNC) == DOME_CUP_NOT_SYNC)
			synced ();
		return false;
	}
	return true;
}

int Cupola::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand (COMMAND_TELD_MOVE))
	{
		double tar_ra;
		double tar_dec;
		if (conn->paramNextDouble (&tar_ra) || conn->paramNextDouble (&tar_dec)
			|| !conn->paramEnd ())
			return -2;
		return moveTo (conn, tar_ra, tar_dec);
	}
	else if (conn->isCommand ("stop"))
	{
	        return moveStop() ;
	}
	return Dome::commandAuthorized (conn);
}
