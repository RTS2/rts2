/*
 * Driver for Moravian Instruments (Moravske pristroje) CCDs.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "camd.h"
#include "miccd.h"

#include <sys/types.h>
#include <fcntl.h>

#define EVENT_TE_RAMP	RTS2_LOCAL_EVENT + 677

namespace rts2camd
{
/**
 * Driver for MI CCD. http://ccd.mii.cz
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class MICCD:public Camera
{
	public:
		MICCD (int argc, char **argv);
		virtual ~MICCD ();

		virtual void postEvent (Rts2Event *event);

		virtual int commandAuthorized (Rts2Conn * conn);

	protected:
		virtual int processOption (int opt);
		virtual int init ();
		virtual int initValues ();

		virtual void initBinnings ()
		{
			Camera::initBinnings ();

			addBinning2D (2, 2);
			addBinning2D (3, 3);
			addBinning2D (4, 4);
			addBinning2D (2, 1);
			addBinning2D (3, 1);
			addBinning2D (4, 1);
			addBinning2D (1, 2);
			addBinning2D (1, 3);
			addBinning2D (1, 4);
		}

		virtual int info ();
		virtual int setValue (Rts2Value *oldValue, Rts2Value *newValue);

		virtual int setCoolTemp (float new_temp);
		virtual void afterNight ();

		virtual int setFilterNum (int new_filter);
		virtual int getFilterNum () { return getCamFilterNum (); }

		virtual int startExposure ();
		virtual int endExposure ();

		virtual int stopExposure ();

		virtual int doReadout ();

		virtual int scriptEnds ();

	private:
		void addFilters (char *opt);
		int clearCCD (int nclears);

		int reinitCamera ();

		Rts2ValueInteger *id;
		Rts2ValueSelection *mode;
		Rts2ValueBool *fan;
		Rts2ValueFloat *tempRamp;
		Rts2ValueFloat *tempTarget;
		Rts2ValueInteger *power;
		Rts2ValueInteger *gain;

		Rts2ValueInteger *shift;

		camera_t camera;
		camera_info_t cami;

		bool reseted_shutter;
};

}

using namespace rts2camd;

MICCD::MICCD (int argc, char **argv):Camera (argc, argv)
{
	createExpType ();
	createTempAir ();
	createTempCCD ();
	createTempSet ();

	createValue (tempRamp, "TERAMP", "[C/min] temperature ramping", false, RTS2_VALUE_WRITABLE);
	tempRamp->setValueFloat (1.0);

	createValue (tempTarget, "TETAR", "[C] current target temperature", false);
	createValue (power, "TEMPPWR", "[%] utilization of cooling power", true);
	createValue (gain, "GAIN", "[e-ADU] gain", true);

	createValue (id, "product_id", "camera product identification", true);
	id->setValueInteger (0);

	createValue (shift, "SHIFT", "shift (for partial, not cleared readout)", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	shift->setValueInteger (0);

	addOption ('p', NULL, 1, "MI CCD product ID");
	addOption ('f', NULL, 1, "filter names (separated with :)");

	reseted_shutter = false;
}

MICCD::~MICCD ()
{
	miccd_close (&camera);
}

void MICCD::postEvent (Rts2Event *event)
{
	float change = 0;
	switch (event->getType ())
	{
		case EVENT_TE_RAMP:
			if (tempTarget->getValueFloat () < tempSet->getValueFloat ())
				change = tempRamp->getValueFloat ();
			else if (tempTarget->getValueFloat () > tempSet->getValueFloat ())
				change = -1 * tempRamp->getValueFloat ();
			if (change != 0)
			{
				tempTarget->setValueFloat (tempTarget->getValueFloat () + change);
				if (fabs (tempTarget->getValueFloat () - tempSet->getValueFloat ()) < fabs (change))
					tempTarget->setValueFloat (tempSet->getValueFloat ());
				sendValueAll (tempTarget);
				if (miccd_set_cooltemp (&camera, tempTarget->getValueFloat ()))
					logStream (MESSAGE_ERROR) << "cannot set target temperature" << sendLog;
				addTimer (60, event);
				return;
			}
			break;
	}
	Camera::postEvent (event);
}

int MICCD::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			id->setValueCharArr (optarg);
			break;
		case 'f':
			addFilters (optarg);
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int MICCD::init ()
{
	int ret;
	ret = Camera::init ();
	if (ret)
		return ret;
	if (miccd_open (id->getValueInteger (), &camera))
	{
		logStream (MESSAGE_ERROR) << "cannot find device with id " << id->getValueInteger () << sendLog;
		return -1;
	}

	switch (camera.model)
	{
		case G10800:
			mode = NULL;
			createValue (fan, "FAN", "camera fan", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
			miccd_fan (&camera, 1);
			fan->setValueBool (true);
			break;
		case G2:
		case G3:
			fan = NULL;
			createValue (mode, "RDOUTM", "camera mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			mode->addSelVal ("NORMAL");
			mode->addSelVal ("LOW NOISE");
			mode->addSelVal ("ULTRA LOW NOISE");

			mode->setValueInteger (1);

			if (miccd_mode (&camera, mode->getValueInteger ()))
			{
				logStream (MESSAGE_ERROR) << "cannot set requested camera mode " << mode->getValueInteger () << sendLog;
				return -1;
			}
			break;
	}

	return 0;
}

int MICCD::initValues ()
{
	int ret;
	ret = miccd_info (&camera, &cami);
	if (ret)
		return -1;

	cami.description[10] = '\0';
	cami.serial[10] = '\0';
	cami.chip[10] = '\0';

	addConstValue ("DESCRIPTION", "camera description", cami.description);
	addConstValue ("SERIAL", "camera serial number", cami.serial);
	addConstValue ("CHIP", "camera chip", cami.chip);

	id->setValueInteger (cami.id);

	initCameraChip (cami.w, cami.h, cami.pw, cami.ph);

	return Camera::initValues ();
}

int MICCD::info ()
{
	if (!isIdle ())
		return Camera::info ();
	int ret;
	float val;
	uint16_t ival;
	ret = miccd_chip_temperature (&camera, &val);
	if (ret)
	{
		if (camera.model != G10800)
			return -1;
		ret = reinitCamera ();
		if (ret)
			return -1;
		ret = miccd_chip_temperature (&camera, &val);
		if (ret)
			return -1;
	}
	tempCCD->setValueFloat (val);
	if (camera.model != G10800)
	{
		ret = miccd_environment_temperature (&camera, &val);
		if (ret)
			return -1;
		tempAir->setValueFloat (val);
		ret = miccd_gain (&camera, &ival);
		if (ret)
			return -1;
		gain->setValueInteger (ival);
	}
	ret = miccd_power_voltage (&camera, &ival);
	if (ret)
		return -1;
	power->setValueInteger (ival);
	return Camera::info ();
}

int MICCD::setValue (Rts2Value *oldValue, Rts2Value *newValue)
{
	if (oldValue == mode)
		return miccd_mode (&camera, newValue->getValueInteger ()) == 0 ? 0 : -2;
	if (oldValue == fan)
		return miccd_fan (&camera, newValue->getValueInteger ()) == 0 ? 0 : -2;
	return Camera::setValue (oldValue, newValue);
}

int MICCD::setCoolTemp (float new_temp)
{
	float val;
	int ret = miccd_chip_temperature (&camera, &val);
	if (ret)
	{
		logStream (MESSAGE_ERROR) << "cannot retrieve chip temperature, cannot start cooling" << sendLog;
		return -1;
	}
	deleteTimers (EVENT_TE_RAMP);
	tempTarget->setValueFloat (val);
	addTimer (1, new Rts2Event (EVENT_TE_RAMP));
	return Camera::setCoolTemp (new_temp);
}

void MICCD::afterNight ()
{
	setCoolTemp (+50);
}

int MICCD::setFilterNum (int new_filter)
{
	int ret = miccd_filter (&camera, new_filter) ? -1 : 0;
	checkQueuedExposures ();
	return ret;
}

int MICCD::startExposure ()
{
	int ret;

	if (shift->getValueInteger () == 0)
	{
		ret = miccd_clear (&camera);
		if (ret)
			return -1;
	}
	else if (shift->getValueInteger () < 0)
	{
		setUsedHeight (getHeight ());
	}
	else
	{
		setUsedHeight (shift->getValueInteger ());
	}
	
	switch (camera.model)
	{
		case G10800:
			// G10800 does not allow > 8 sec exposures
			ret = miccd_start_exposure (&camera, getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), getExposure ());
			if (ret < 0)
			{
				if (camera.model != G10800)	
					return -1;
				// else try to reinit..
				logStream (MESSAGE_WARNING) << "camera disappeared, trying to reinint.." << sendLog;
				if (reinitCamera ())
					return -1;
				ret = miccd_start_exposure (&camera, getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), getExposure ());
				if (ret < 0)
				{
					logStream (MESSAGE_ERROR) << "reinit failed" << sendLog;
					return -1;
				}
			}
			setExposure (((float) ret) / 8000.0);
			break;
		case G2:
		case G3:	
			if (getExpType () != 1)
			{
				ret = miccd_open_shutter (&camera);
				if (ret)
					return -1;
			}
			break;
	}
	return 0;
}

int MICCD::endExposure ()
{
	int ret;
	if (getExpType () != 1)
	{
		switch (camera.model)
		{
			case G10800:
				break;
			case G2:
			case G3:
				ret = miccd_close_shutter (&camera);
				if (ret)
					return ret;
				break;
		}
	}
	return Camera::endExposure ();
}

int MICCD::stopExposure ()
{
 	int ret;
	switch (camera.model)
	{
		case G10800:
			ret = miccd_abort_exposure (&camera);
			if (ret)
				return -1;
			break;
		case G2:
		case G3:
			ret = miccd_close_shutter (&camera);
			if (ret)
				return -1;
			break;
	}
	return Camera::stopExposure ();
}

int MICCD::doReadout ()
{
	int ret;

	switch (camera.model)
	{
		case G10800:
			ret = miccd_read_data (&camera, 2 * getUsedWidth () * getUsedHeight (), dataBuffer);
			break;
		case G2:
		case G3:
			ret = miccd_read_frame (&camera, binningHorizontal (), binningVertical (), getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), dataBuffer);
			break;
	}

	if (ret < 0)
		return -1;

	ret = sendReadoutData (dataBuffer, getWriteBinaryDataSize ());
	if (ret < 0)
		return ret;
	if (getWriteBinaryDataSize () == 0)
		return -2;
	return 0;
}

int MICCD::scriptEnds ()
{
	if (shift->getValueInteger () != 0)
	{
		setUsedHeight (getHeight ());
		shift->setValueInteger (0);
	}
	return Camera::scriptEnds ();
}

void MICCD::addFilters (char *opt)
{
	char *s = opt;
	char *o = s;
	for (; *o != '\0'; o++)
	{
		if (*o == ':')
		{
			*o = '\0';
			camFilterVal->addSelVal (s);
			s = o + 1;
		}
	}
	if (o != s)
		camFilterVal->addSelVal (s);
}

int MICCD::clearCCD (int nclear)
{
	int n = nclear;
	for (; nclear > 0; nclear--)
	{
		miccd_clear (&camera);
		for (int i = 0; i < n; i++)
			miccd_hclear (&camera);
	}
	return 0;
}

int MICCD::reinitCamera ()
{
	miccd_close (&camera);
	if (miccd_open (id->getValueInteger (), &camera))
		return -1;
	switch (camera.model)
	{
		case G10800:
			if (miccd_fan (&camera, fan->getValueInteger ()))
			{
				logStream (MESSAGE_ERROR) << "reinit failed - cannot set fan" << sendLog;
				return -1;
			}
			break;
		case G2:
		case G3:
			if (miccd_mode (&camera, mode->getValueInteger ()))
			{
				logStream (MESSAGE_ERROR) << "reinit failed - cannot set mode" << sendLog;
				return -1;
			}
			break;
	}
	maskState (DEVICE_ERROR_MASK, 0, "all errors cleared");
	return 0;
}

int MICCD::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("clear"))
	{
		int nclear;
		if (conn->paramNextInteger (&nclear)
			|| !conn->paramEnd ())
			return -2;
		return clearCCD (nclear);
	}
	return Camera::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	MICCD device (argc, argv);
	return device.run ();
}
