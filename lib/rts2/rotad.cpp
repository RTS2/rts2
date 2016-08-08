/* 
 * Abstract rotator class.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "rotad.h"

using namespace rts2rotad;

Rotator::Rotator (int argc, char **argv, const char *defname):rts2core::Device (argc, argv, DEVICE_TYPE_ROTATOR, defname)
{
	createValue (currentPosition, "CUR_POS", "[deg] current rotator position", true, RTS2_DT_DEGREES);
	createValue (targetPosition, "TAR_POS", "[deg] target rotator position", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (toGo, "TAR_DIFF", "[deg] difference between target and current position", false, RTS2_DT_DEG_DIST);
	createValue (paTracking, "parangle_track", "run parralactic tracking code", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (parallacticAngleRef, "parangle_ref", "time when parallactic angle was calculated", false);
	createValue (parallacticAngle, "PARANGLE", "[deg] telescope parallactic angle", false, RTS2_DT_DEGREES);
	createValue (parallacticAngleRate, "PARATE", "[deg/hour] calculated change of parallactic angle", false, RTS2_DT_DEGREES);
}

int Rotator::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand (COMMAND_PARALLACTIC_UPDATE))
	{
		double reftime;
		double pa;
		double rate;
		if (conn->paramNextDouble (&reftime) || conn->paramNextDouble (&pa) || conn->paramNextDouble (&rate) || !conn->paramEnd ())
			return -2;
		parallacticAngleRef->setValueDouble (reftime);
		parallacticAngle->setValueDouble (pa);
		parallacticAngleRate->setValueDouble (rate);
		return 0;
	}
	return rts2core::Device::commandAuthorized (conn);
}

int Rotator::idle ()
{
	if ((getState () & ROT_MASK_ROTATING) == ROT_ROTATING)
	{
		long ret = isRotating ();
		if (ret < 0)
		{
			if (ret == -2)
			{
				maskState (ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_IDLE, "rotation finished");
			}
			else
			{
				// ends with error
				maskState (ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_IDLE, "rotation finished with error");
			}
			endRotation ();
			setIdleInfoInterval (60);
		}
		else
		{
			setIdleInfoInterval (((float) ret) / USEC_SEC);
		}
		updateToGo ();
		sendValueAll (currentPosition);
		sendValueAll (toGo);
	}
	if ((getState () & ROT_MASK_PATRACK) == ROT_PA_TRACK)
	{
		double t = getNow ();
		double pa = getPA (t);
		if (!isnan (pa))
			setTarget (getPA (t));
	}
	return rts2core::Device::idle ();
}

int Rotator::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == targetPosition)
	{
		setTarget (new_value->getValueDouble ());
		targetPosition->setValueDouble (new_value->getValueDouble ());
		updateToGo ();
		maskState (ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_ROTATING | BOP_EXPOSURE, "rotator rotation started");
		return 0;
	}
	if (old_value == paTracking)
	{
		maskState (ROT_MASK_PATRACK, ((rts2core::ValueBool *) new_value)->getValueBool () ? ROT_PA_TRACK : ROT_PA_NOT, "change PA rotation");
		return 0;
	}
	return rts2core::Device::setValue (old_value, new_value);
}

void Rotator::setCurrentPosition (double cp)
{
	currentPosition->setValueDouble (cp);
	updateToGo ();
}

double Rotator::getPA (double t)
{
	return parallacticAngle->getValueDouble () + (parallacticAngleRate->getValueDouble () / 3600.0) * (t - parallacticAngleRef->getValueDouble ());
}

void Rotator::updateToGo ()
{
	toGo->setValueDouble (getCurrentPosition () - getTargetPosition ());
}
