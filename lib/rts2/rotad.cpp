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

#define EVENT_TRACK_TIMER   RTS2_LOCAL_EVENT + 1600

using namespace rts2rotad;

Rotator::Rotator (int argc, char **argv, const char *defname, bool ownTimer):rts2core::Device (argc, argv, DEVICE_TYPE_ROTATOR, defname)
{
	lastTrackingRun = 0;
	hasTimer = ownTimer;

	autoPlus = false;
	autoOldTrack = false;

	createValue (zeroOffs, "zero_offs", "[deg] zero offset", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	zeroOffs->setValueDouble (0);

	createValue (offset, "OFFSET", "[deg] custom offset", true, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES);
	offset->setValueDouble (0);

	createValue(parkingPosition, "PARK", "[deg] parking position", false, RTS2_VALUE_WRITABLE | RTS2_DT_DEGREES | RTS2_VALUE_AUTOSAVE);
	parkingPosition->setValueDouble(0);

	createValue (currentPosition, "CUR_POS", "[deg] current rotator position", true, RTS2_DT_DEGREES);
	createValue (targetPosition, "TAR_POS", "[deg] target rotator position", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (tarMin, "MIN", "[deg] minimum angle", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (tarMax, "MAX", "[deg] maximum angle", false, RTS2_DT_DEGREES | RTS2_VALUE_WRITABLE);
	createValue (paOffset, "PA", "[deg] target parallactic angle offset", true, RTS2_DT_DEGREES);
	createValue (toGo, "TAR_DIFF", "[deg] difference between target and current position", false, RTS2_DT_DEG_DIST);
	createValue (paTracking, "parangle_track", "run parralactic tracking code", false, RTS2_DT_ONOFF | RTS2_VALUE_WRITABLE);
	createValue (negatePA, "negate_PA", "negate paralactic angle", false, RTS2_VALUE_WRITABLE);
	negatePA->setValueBool (false);
	createValue (parallacticAngleRef, "parangle_ref", "time when parallactic angle was calculated", false);
	createValue (parallacticAngle, "PARANGLE", "[deg] telescope parallactic angle", false, RTS2_DT_DEGREES);
	createValue (parallacticAngleRate, "PARATE", "[deg/hour] calculated change of parallactic angle", false, RTS2_DT_DEGREES);
	paTracking->setValueBool (true);

	createValue (trackingFrequency, "tracking_frequency", "[Hz] tracking frequency", false);
	createValue (trackingFSize, "tracking_num", "numbers of tracking request to calculate tracking stat", false, RTS2_VALUE_WRITABLE);
	createValue (trackingWarning, "tracking_warning", "issue warning if tracking frequency drops bellow this number", false, RTS2_VALUE_WRITABLE);
	trackingWarning->setValueFloat (1);
	trackingFSize->setValueInteger (20);
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
		if (paTracking->getValueBool ())
			maskState (ROT_MASK_PATRACK | ROT_MASK_AUTOROT, ROT_PA_TRACK, "started tracking");
		if (hasTimer == false)
			addTimer (0.01, new rts2core::Event (EVENT_TRACK_TIMER));
		return 0;
	}
	else if (conn->isCommand (COMMAND_ROTATOR_AUTO))
	{
		autoOldTrack = paTracking->getValueBool ();
		paTracking->setValueBool (false);
		autoPlus = true;
		maskState (ROT_MASK_AUTOROT | ROT_MASK_PATRACK, ROT_AUTO | ROT_PA_NOT, "autorotating");
		if (hasTimer == false)
			addTimer (0.1, new rts2core::Event (EVENT_TRACK_TIMER));
		return 0;
	}
	else if (conn->isCommand (COMMAND_ROTATOR_AUTOSTOP))
	{
		if ((getState () & ROT_MASK_AUTOROT) != ROT_AUTO)
			return -2;
		setTarget (getCurrentPosition ());
		paTracking->setValueBool (autoOldTrack);
		if (autoOldTrack)
		{
			maskState (ROT_MASK_PATRACK | ROT_MASK_AUTOROT, ROT_PA_TRACK, "restarted tracking");
			if (hasTimer == false)
				addTimer (0.01, new rts2core::Event (EVENT_TRACK_TIMER));
		}
		else
		{
			maskState (ROT_MASK_PATRACK | ROT_MASK_AUTOROT, ROT_PA_NOT, "restarted tracking");
		}
		return 0;
	}
	else if (conn->isCommand (COMMAND_ROTATOR_PARK))
	{
		maskState (ROT_MASK_PARK | ROT_MASK_PATRACK | ROT_MASK_AUTOROT, ROT_PARKING, "parking rotator");
		setTarget(parkingPosition->getValueDouble());
		return 0;
	}
	return rts2core::Device::commandAuthorized (conn);
}

int Rotator::idle ()
{
	checkRotators ();
	return rts2core::Device::idle ();
}

void Rotator::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_TRACK_TIMER:
			checkRotators ();
			addTimer (0.01, event);
			return;
	}
	return Device::postEvent (event);
}

void Rotator::checkRotators ()
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
				maskState (ROT_MASK_ROTATING | BOP_EXPOSURE | DEVICE_ERROR_MASK, ROT_IDLE | DEVICE_ERROR_HW, "rotation finished with error");
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
		if (!std::isnan (pa))
		{
			setTarget (pa);
			int ret = isRotating ();
			if (ret >= 0)
			{
				if (paTracking->getValueBool () == true)
					maskState (BOP_EXPOSURE, BOP_EXPOSURE, "rotating to target");
			}
			else
			{
				maskState (BOP_EXPOSURE, 0, "rotated to target");
			}
		}
	}
	if ((getState() & ROT_MASK_PARK) == ROT_PARKING)
	{
		long ret = isParking();
		if (ret < 0)
		{
			if (ret == -2)
			{
				maskState(ROT_MASK_ROTATING, ROT_IDLE, "rotation finished with error");
			}
			else
			{
				maskState(ROT_MASK_ROTATING | DEVICE_ERROR_MASK, ROT_IDLE | DEVICE_ERROR_HW, "rotation finished with error");
			}
			setIdleInfoInterval (5);
		}
		else
		{
			setIdleInfoInterval(((float) ret) / USEC_SEC);
		}
	}
	if ((getState () & ROT_MASK_AUTOROT) == ROT_AUTO)
	{
		long ret = isRotating ();
		if (ret < 0)
		{
			if (ret == -2)
			{
				if (autoPlus == true)
				{
					setTarget (std::isnan (getTargetMax ()) ? 180 : getTargetMax () + getOffset ());
					autoPlus = false;
				}
				else
				{
					setTarget (std::isnan (getTargetMin ()) ? -180 : getTargetMin () + getOffset ());
					autoPlus = true;
				}
			}
			else
			{
				maskState (ROT_MASK_ROTATING, ROT_IDLE, "rotation finished with error");
			}
			setIdleInfoInterval (5);
		}
		else
		{
			setIdleInfoInterval (((float) ret) / USEC_SEC);
		}
		updateToGo ();
		sendValueAll (currentPosition);
		sendValueAll (toGo);
	}
}

int Rotator::setValue (rts2core::Value *old_value, rts2core::Value *new_value)
{
	if (old_value == targetPosition)
	{
		setTarget (new_value->getValueDouble ());
		targetPosition->setValueDouble (new_value->getValueDouble ());
		updateToGo ();
		maskState (ROT_MASK_AUTOROT | ROT_MASK_ROTATING | BOP_EXPOSURE, ROT_ROTATING | BOP_EXPOSURE, "rotator rotation started");
	}
	else if (old_value == paTracking)
	{
		maskState (ROT_MASK_AUTOROT | ROT_MASK_PATRACK | BOP_EXPOSURE, ((rts2core::ValueBool *) new_value)->getValueBool () ? ROT_PA_TRACK : ROT_PA_NOT, "change PA rotation");
	}
	else if (old_value == offset)
	{
		logStream (MESSAGE_INFO | INFO_ROTATOR_OFFSET) << new_value->getValueDouble () << " " << paOffset->getValueDouble ()<< sendLog;
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
	double ret = parallacticAngle->getValueDouble () + (parallacticAngleRate->getValueDouble () / 3600.0) * (t - parallacticAngleRef->getValueDouble ());
	if (negatePA->getValueBool ())
		return -ret;
	return ret;
}

long Rotator::isParking()
{
	return abs(getCurrentPosition() - parkingPosition->getValueDouble()) < 1 ? -2 : USEC_SEC;
}

void Rotator::setPAOffset (double paOff)
{
	if (paOffset->getValueDouble () != paOff)
	{
		paOffset->setValueDouble (paOff);
		logStream (MESSAGE_INFO | INFO_ROTATOR_OFFSET) << offset->getValueDouble () << " " << paOff << sendLog;
	}
}

void Rotator::updateToGo ()
{
	toGo->setValueDouble (getCurrentPosition () - getTargetPosition ());
}

void Rotator::updateTrackingFrequency ()
{
	double n = getNow ();
	if (!std::isnan (lastTrackingRun) && n != lastTrackingRun)
	{
		double tv = 1 / (n - lastTrackingRun);
		trackingFrequency->addValue (tv, trackingFSize->getValueInteger ());
		trackingFrequency->calculate ();
		if (tv < trackingWarning->getValueFloat ())
			logStream (MESSAGE_ERROR) << "lost rotator tracking, frequency is " << tv << sendLog;
	}
	lastTrackingRun = n;
}
