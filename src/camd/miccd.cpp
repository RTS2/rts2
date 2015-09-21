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

#ifdef WITH_K8055

#include <k8055.h>

#define K8055_SHUTTER_OPEN    250
#define K8055_SHUTTER_CLOSED  0
#endif

#define EVENT_TE_RAMP	      RTS2_LOCAL_EVENT + 677

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

		virtual void postEvent (rts2core::Event *event);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int initValues ();
		virtual void initDataTypes ();

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
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int setCoolTemp (float new_temp);
		virtual void afterNight ();

		virtual int setFilterNum (int new_filter, const char *fn = NULL);

		virtual int startExposure ();
		virtual int endExposure (int ret);

		virtual int stopExposure ();

		virtual int doReadout ();

		virtual int scriptEnds ();

	private:
		int clearCCD (int nclears);

		int reinitCamera ();

		rts2core::ValueInteger *id;
		rts2core::ValueSelection *mode;
		rts2core::ValueBool *fan;
		rts2core::ValueFloat *tempRamp;
		rts2core::ValueFloat *tempTarget;
		rts2core::ValueInteger *power;
		rts2core::ValueInteger *gain;

		rts2core::ValueInteger *shift;

		camera_t camera;
		camera_info_t cami;

		bool reseted_shutter;
		bool internalTimer;
};

}

using namespace rts2camd;

MICCD::MICCD (int argc, char **argv):Camera (argc, argv)
{
	createExpType ();
	createFilter ();
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
	internalTimer = false;
}

MICCD::~MICCD ()
{
#ifdef K8055
	CloseDevice ();
#endif
	miccd_close (&camera);
}

void MICCD::postEvent (rts2core::Event *event)
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
			setFilterWorking (true);
			break;
		default:
			return Camera::processOption (opt);
	}
	return 0;
}

int MICCD::initHardware ()
{
	if (miccd_open (id->getValueInteger (), &camera))
	{
		logStream (MESSAGE_ERROR) << "cannot find device with id " << id->getValueInteger () << sendLog;
		return -1;
	}

	switch (camera.model)
	{
	 	case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			createValue (fan, "FAN", "camera fan", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
			miccd_fan (&camera, 1);
			fan->setValueBool (true);
			createValue (mode, "RDOUTM", "camera mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
			mode->addSelVal ("NORMAL");
			mode->addSelVal ("LOW NOISE");
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
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
#ifdef WITH_K8055
	if (OpenDevice (0) < 0)
		logStream (MESSAGE_ERROR) << "cannot open K8055 device" << sendLog;
	expType->addSelVal ("OPEN", NULL);
#endif
	return 0;
}

int MICCD::initValues ()
{
	int ret;
	char *buf;
	int i;

	ret = miccd_info (&camera, &cami);
	if (ret)
		return -1;

	// remove trailing spaces
	for (i = sizeof (cami.description)/sizeof (cami.description[0]) - 1; i>=0; i--)
	{
		if (cami.description[i] == ' ')
			cami.description[i] = '\0';
		else
			break;
	}
	for (i = sizeof (cami.serial)/sizeof (cami.serial[0]) - 1; i>=0; i--)
	{
		if (cami.serial[i] == ' ')
			cami.serial[i] = '\0';
		else
			break;
	}
	for (i = sizeof (cami.chip)/sizeof (cami.chip[0]) - 1; i>=0; i--)
	{
		if (cami.chip[i] == ' ')
			cami.chip[i] = '\0';
		else
			break;
	}

	buf = (char *) malloc (100);

	sprintf (buf, "MI %.15s rev.%d", cami.description, cami.hwrevision);
	ccdRealType->setValueCharArr (buf);

	sprintf (buf, "%.15s", cami.serial);
	serialNumber->setValueCharArr (buf);

	sprintf (buf, "%.15s", cami.chip);
	ccdChipType->setValueCharArr (buf);

	free (buf);

	id->setValueInteger (cami.id);

	initCameraChip (cami.w, cami.h, cami.pw, cami.ph);

	return Camera::initValues ();
}

void MICCD::initDataTypes ()
{
	switch (camera.model)
	{
	 	case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			addDataType (RTS2_DATA_USHORT);
			addDataType (RTS2_DATA_BYTE);
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			addDataType (RTS2_DATA_USHORT);
			break;
	}
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
		switch (camera.model)
		{
			case G10300:
			case G10400:
			case G10800:
			case G11200:
			case G11400:
			case G12000:
			case G2:
			case G3:
			case G3_H:
                        case GX_BI:
				ret = reinitCamera ();
				if (ret)
					return -1;
				ret = miccd_chip_temperature (&camera, &val);
				if (ret)
					return -1;
				break;
		}
	}
	tempCCD->setValueFloat (val);
	switch (camera.model)
	{
		case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			ret = miccd_environment_temperature (&camera, &val);
			if (ret)
				return -1;
			tempAir->setValueFloat (val);
			ret = miccd_gain (&camera, &ival);
			if (ret)
				return -1;
			gain->setValueInteger (ival);
			break;
	}
	ret = miccd_power_voltage (&camera, &ival);
	if (ret)
		return -1;
	power->setValueInteger (ival);
	return Camera::info ();
}

int MICCD::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == mode)
	{
		switch (camera.model)
		{
			case G10300:
			case G10800:
			case G11200:
			case G11400:
			case G12000:
				return miccd_g1_mode (&camera, getDataType () == RTS2_DATA_USHORT, newValue->getValueInteger ()) == 0 ? 0 : -2;
			case G2:
			case G3:
			case G3_H:
                        case GX_BI:
			default:
				return miccd_mode (&camera, newValue->getValueInteger ()) == 0 ? 0 : -2;
		}
	}
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
	addTimer (1, new rts2core::Event (EVENT_TE_RAMP));
	return Camera::setCoolTemp (new_temp);
}

void MICCD::afterNight ()
{
	setCoolTemp (+50);
}

int MICCD::setFilterNum (int new_filter, const char *fn)
{
	if (fn != NULL)
		return Camera::setFilterNum (new_filter, fn);
	logStream (MESSAGE_INFO) << "moving filter from #" << camFilterVal->getValueInteger () << " (" << camFilterVal->getSelName () << ")" << " to #" << new_filter << " (" << camFilterVal->getSelName (new_filter) << ")" << sendLog;
	int ret = miccd_filter (&camera, new_filter) ? -1 : 0;
	if (ret == 0)
		logStream (MESSAGE_INFO) << "filter moved to #" << new_filter << " (" << camFilterVal->getSelName (new_filter) << ")" << sendLog;
	else
		logStream (MESSAGE_INFO) << "filter movement error?" << sendLog;
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
		{
			logStream (MESSAGE_ERROR) << "MICCD::startExposure error calling miccd_clear, trying reinit" << sendLog;
			ret = initHardware ();
			if (ret)
				return -1;
		}
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
	 	case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			ret = miccd_g1_mode (&camera, getDataType () == RTS2_DATA_USHORT, mode->getValueInteger ());
			if (ret)
			{
				logStream (MESSAGE_ERROR) << "MICCD::startExposure error calling miccd_g1_mode" << sendLog;
				return ret;
			}
			if (getExposure () <= 8)
			{
				ret = miccd_start_exposure (&camera, getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), getExposure ());
				if (ret < 0)
				{
					// else try to reinit..
					logStream (MESSAGE_WARNING) << "camera disappeared, trying to reinintiliaze it.." << sendLog;
					if (reinitCamera ())
						return -1;
					ret = miccd_start_exposure (&camera, getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), getExposure ());
					if (ret < 0)
					{
						logStream (MESSAGE_ERROR) << "reinitilization failed" << sendLog;
						return -1;
					}
				}
				setExposure (((float) ret) / 8000.0);
				internalTimer = true;
			}
			else
			{
				internalTimer = false;
			}
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			if (getExpType () != 1)
			{
				ret = miccd_open_shutter (&camera);
				if (ret)
					return -1;
			}
			break;
	}
#ifdef WITH_K8055
	ret = OutputAnalogChannel (1, getExpType () == 1 ? K8055_SHUTTER_CLOSED : K8055_SHUTTER_OPEN);
	if (ret)
		logStream (MESSAGE_ERROR) << "cannot open K8055 shutter" << sendLog;
#endif
	return 0;
}

int MICCD::endExposure (int ret)
{
	int cret;
	switch (camera.model)
	{
		case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			if (internalTimer == false)
			{
				cret = miccd_start_exposure (&camera, getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), -1);
				if (cret)
					return -1;
			}
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			if (getExpType () != 1)
			{
				cret = miccd_close_shutter (&camera);
				if (cret)
					return cret;
			}
			break;
	}
#ifdef WITH_K8055
	if (getExpType () == 0)
	{
		cret = OutputAnalogChannel (1, K8055_SHUTTER_CLOSED);
		if (cret)
			logStream (MESSAGE_ERROR) << "cannot close K8055 shutter" << sendLog;
	}
#endif
	return Camera::endExposure (ret);
}

int MICCD::stopExposure ()
{
 	int ret;
	switch (camera.model)
	{
		case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			if (internalTimer == false)
				ret = miccd_clear (&camera);
			else
				ret = miccd_abort_exposure (&camera);
			if (ret)
				return -1;
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			ret = miccd_close_shutter (&camera);
			if (ret)
				return -1;
			break;
	}
	#ifdef WITH_K8055
	ret = OutputAnalogChannel (1, K8055_SHUTTER_CLOSED);
	if (ret)
		logStream (MESSAGE_ERROR) << "cannot open K8055 shutter" << sendLog;
	#endif
	return Camera::stopExposure ();
}

int MICCD::doReadout ()
{
	int ret;

	switch (camera.model)
	{
		case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			ret = miccd_read_data (&camera, (getDataType () == RTS2_DATA_USHORT) ? 2 * getUsedWidth () * getUsedHeight () : getUsedWidth () * getUsedHeight (), getDataBuffer (0), getUsedWidth (), getUsedHeight ());
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			ret = miccd_read_frame (&camera, binningHorizontal (), binningVertical (), getUsedX (), getUsedY (), getUsedWidth (), getUsedHeight (), getDataBuffer (0));
			break;
	}

	if (ret < 0)
		return -1;

	ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
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
	logStream (MESSAGE_WARNING) << "reinitiliazing camera - this should not happen" << sendLog;
	miccd_close (&camera);
	if (miccd_open (id->getValueInteger (), &camera))
		return -1;
	switch (camera.model)
	{
		case G10300:
		case G10400:
		case G10800:
		case G11200:
		case G11400:
		case G12000:
			if (miccd_fan (&camera, fan->getValueInteger ()))
			{
				logStream (MESSAGE_ERROR) << "reinitilization failed - cannot set fan" << sendLog;
				return -1;
			}
			break;
		case G2:
		case G3:
		case G3_H:
                case GX_BI:
			if (miccd_mode (&camera, mode->getValueInteger ()))
			{
				logStream (MESSAGE_ERROR) << "reinitilization failed - cannot set mode" << sendLog;
				return -1;
			}
			break;
	}
	maskState (DEVICE_ERROR_MASK, 0, "all errors cleared");
	return 0;
}

int MICCD::commandAuthorized (rts2core::Connection * conn)
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
