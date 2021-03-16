/*
 * MI CCD driver, compiled with official GXCCD drivers.
 * Copyright (C) 2016 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "camd.h"
#include "gxccd.h"
#include "utilsfunc.h"
#include <iomanip>

#include <sys/file.h>

#define EVENT_TE_RAMP	      RTS2_LOCAL_EVENT + 678

namespace rts2camd
{

/**
 * Driver for cameras from Moravian Instruments (using their closed-source gxccd library)
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class GXCCD:public Camera
{
	public:
		GXCCD (int argc, char **argv);
		virtual ~GXCCD ();

		virtual void postEvent (rts2core::Event *event);

		virtual int commandAuthorized (rts2core::Connection * conn);

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int initValues ();
		virtual void initDataTypes ();

		virtual void initBinnings ();

		virtual int info ();
		virtual int setValue (rts2core::Value *oldValue, rts2core::Value *newValue);

		virtual int setCoolTemp (float new_temp);
		virtual int switchCooling (bool cooling);

		virtual int setFilterNum (int new_filter, const char *fn = NULL);

		virtual int startExposure ();

		virtual int stopExposure ();

		virtual int doReadout ();

		virtual int scriptEnds ();

	private:
		int clearCCD (double pref_time, int nclears);

		int reinitCamera ();

		rts2core::ValueInteger *id;
		rts2core::ValueSelection *mode;
		rts2core::ValueIntegerMinMax *fan = NULL;
		rts2core::ValueIntegerMinMax *windowHeating;
		rts2core::ValueFloat *tempRamp;
		rts2core::ValueFloat *tempTarget;
		rts2core::ValueFloat *power = NULL;
		rts2core::ValueFloat *voltage;
		rts2core::ValueFloat *gain;

		rts2core::ValueInteger *filterFailed;

		camera_t *camera;

		bool reseted_shutter;
		bool internalTimer;

		bool hasCooling = false;

		// buffer for error messages
		char gx_err[200];
};

}

using namespace rts2camd;

GXCCD::GXCCD (int argc, char **argv):Camera (argc, argv)
{
	createExpType ();
	createFilter ();
	createTempAir ();
	createTempCCD ();
	createTempSet ();

	createValue (tempRamp, "TERAMP", "[C/min] temperature ramping", false, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	tempRamp->setValueFloat (1.0);
	createValue (tempTarget, "TETAR", "[C] current target temperature", false);

	createValue (voltage, "VOLTAGE", "[V] current voltage of the camera power supply", true);
	createValue (gain, "GAIN", "[e-ADU] gain", true);

	createValue (id, "product_id", "camera product identification", true);
	id->setValueInteger (0);

	createValue (filterFailed, "filter_err", "filter failed count", false);
	filterFailed->setValueInteger (0);

	addOption ('p', NULL, 1, "MI CCD product ID");
	addOption ('f', NULL, 1, "filter names (separated with :)");

	reseted_shutter = false;
	internalTimer = false;
}

GXCCD::~GXCCD ()
{
	gxccd_release (camera);
}

void GXCCD::postEvent (rts2core::Event *event)
{
	float change = 0;
	int res = 0;

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

				res = gxccd_set_temperature (camera, tempTarget->getValueFloat ());
				if (res)
				{
					gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
					logStream (MESSAGE_ERROR) << "cannot set target temperature: " << gx_err << sendLog;

					if (res == -1)
						reinitCamera ();
				}

				addTimer (60, event);
				return;
			}
			break;
	}
	Camera::postEvent (event);
}

int GXCCD::processOption (int opt)
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

int GXCCD::initHardware ()
{
	setIdleInfoInterval (10);

	// Crude hack to prevent simultaneous initialization of several cameras
	int fd = open ("/tmp/.gxccd.init.lock", O_RDWR | O_CREAT, 0666);
	flock (fd, LOCK_EX);
	logStream (MESSAGE_DEBUG) << "locked GXCCD driver for initialization of camera id " << id->getValueInteger () << sendLog;
	camera = gxccd_initialize_usb (id->getValueInteger ());
	flock (fd, LOCK_UN);
	close (fd);
	logStream (MESSAGE_DEBUG) << "unlocked GXCCD driver after initialization of camera id " << id->getValueInteger () << sendLog;

	if (camera == NULL)
	{
		logStream (MESSAGE_ERROR) << "cannot find device with id " << id->getValueInteger () << sendLog;
		return -1;
	}

	bool val;
	int valInt;
	/*gxccd_get_boolean_parameter (camera, GBP_FILTERS, &val);
	if (val)
	{
		// This way this is not functional (because of the need of parsing CLI parameters, function createFilter () has to be called much sooner).	// When needed, this could be solved the same way I did the "hasCooling" problem... However, it is not necessary in this case, as filters are only taken into account when specified on commandline...
		//createFilter ();
	}*/
	gxccd_get_boolean_parameter (camera, GBP_COOLER, &val);
	if (val)
	{
		hasCooling = true;
	}
	gxccd_get_boolean_parameter (camera, GBP_FAN, &val);
	if (val)
	{
		createValue (fan, "FAN", "camera fan", false, RTS2_VALUE_WRITABLE);
		gxccd_get_integer_parameter (camera, GIP_MAX_FAN, &valInt);
		fan->setMin (0);
		fan->setMax (valInt);
		updateMetaInformations (fan);
		fan->setValueInteger (valInt);
		gxccd_set_fan (camera, valInt);
	}
	gxccd_get_boolean_parameter (camera, GBP_WINDOW_HEATING, &val);
	if (val)
	{
		createValue (windowHeating, "WINDOW_HEATING", "CCD window anti-frost heating", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
		gxccd_get_integer_parameter (camera, GIP_MAX_WINDOW_HEATING, &valInt);
		windowHeating->setMin (0);
		windowHeating->setMax (valInt);
		updateMetaInformations (windowHeating);
		windowHeating->setValueInteger (0);
		gxccd_set_window_heating (camera, 0);
	}
	gxccd_get_boolean_parameter (camera, GBP_POWER_UTILIZATION, &val);
	if (val)
	{
		createValue (power, "TEMPPWR", "[%] utilization of cooling power", true);
	}

	gxccd_get_boolean_parameter (camera, GBP_READ_MODES, &val);
	if (val)
	{
		createValue (mode, "RDOUTM", "camera mode", true, RTS2_VALUE_WRITABLE, CAM_WORKING);

		int rdm = 0;
		while (true)
		{
			char moden[200];
			int ret = gxccd_enumerate_read_modes (camera, rdm, moden, sizeof (moden));
			if (ret)
				break;
			mode->addSelVal (moden);
			rdm++;
		}

		mode->setValueInteger (0);

		int ret = gxccd_set_read_mode (camera, mode->getValueInteger ());

		if (ret)
		{
			gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
			logStream (MESSAGE_ERROR) << "cannot set requested camera mode " << mode->getValueInteger () << " " << gx_err << sendLog;
			return -1;
		}
	}
	return 0;
}

int GXCCD::initValues ()
{
	int ret;
	char buf[200];

	ret = gxccd_get_string_parameter (camera, GSP_CAMERA_DESCRIPTION, buf, sizeof (buf));
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "get camera description " << gx_err << sendLog;
		return -1;
	}

	ccdRealType->setValueCharArr (buf);

	ret = gxccd_get_string_parameter (camera, GSP_CAMERA_SERIAL, buf, sizeof (buf));
	if (ret)
		return -1;
	serialNumber->setValueCharArr (buf);

	ret = gxccd_get_string_parameter (camera, GSP_CHIP_DESCRIPTION, buf, sizeof (buf));
	if (ret)
		return -1;
	ccdChipType->setValueCharArr (buf);

	int ival;
	ret = gxccd_get_integer_parameter (camera, GIP_CAMERA_ID, &ival);
	if (ret)
		return -1;
	id->setValueInteger (ival);

	int w, h, pw, ph;
	ret = gxccd_get_integer_parameter (camera, GIP_CHIP_W, &w);
	if (ret)
		return -1;
	ret = gxccd_get_integer_parameter (camera, GIP_CHIP_D, &h);
	if (ret)
		return -1;
	ret = gxccd_get_integer_parameter (camera, GIP_PIXEL_W, &pw);
	if (ret)
		return -1;
	ret = gxccd_get_integer_parameter (camera, GIP_PIXEL_D, &ph);
	if (ret)
		return -1;

	initCameraChip (w, h, pw, ph);

	return Camera::initValues ();
}

void GXCCD::initDataTypes ()
{
	if (mode->selSize () >= 3)
	{
		addDataType (RTS2_DATA_USHORT);
		addDataType (RTS2_DATA_BYTE);
	}
	else if (mode->selSize () == 2)
	{
		addDataType (RTS2_DATA_USHORT);
	}
}

void GXCCD::initBinnings ()
{
	int max_bin_x, max_bin_y;
	int ret = gxccd_get_integer_parameter (camera, GIP_MAX_BINNING_X, &max_bin_x);
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "cannot initBinnings " << gx_err << sendLog;
		return;
	}
	ret = gxccd_get_integer_parameter (camera, GIP_MAX_BINNING_Y, &max_bin_y);
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "cannot initBinnings " << gx_err << sendLog;
		return;
	}
	for (int i = 1; i <= max_bin_x; i++)
		for (int j = 1; j <= max_bin_y; j++)
			addBinning2D (i, j);
	setBinning (1, 1);
}

int GXCCD::info ()
{
	if (!isIdle ())
		return Camera::info ();
	int ret;
	float val;

	ret = gxccd_get_value (camera, GV_CHIP_TEMPERATURE, &val);
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		if (camera)
			logStream (MESSAGE_ERROR) << "error getting temperature: " << gx_err << sendLog;

		// The hardware sometimes returns an INDEX error for no objoius reason, it is mapped to ret=-2
		if (ret == -1 && reinitCamera ())
			return -1;
	}
	else
		tempCCD->setValueFloat (val);

	ret = gxccd_get_value (camera, GV_ENVIRONMENT_TEMPERATURE, &val);
	tempAir->setValueFloat (ret ? NAN : val);

	ret = gxccd_get_value (camera, GV_ADC_GAIN, &val);
	gain->setValueFloat (ret ? NAN : val);

	if (power)
	{
		ret = gxccd_get_value (camera, GV_POWER_UTILIZATION, &val);
		power->setValueFloat (ret ? NAN : val);
	}

	ret = gxccd_get_value (camera, GV_SUPPLY_VOLTAGE, &val);
	voltage->setValueFloat (ret ? NAN : val);

	return Camera::info ();
}

int GXCCD::setValue (rts2core::Value *oldValue, rts2core::Value *newValue)
{
	if (oldValue == mode)
	{
		int ret = gxccd_set_read_mode (camera, newValue->getValueInteger ());
		if (ret == 0)
			return 0;
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "cannot set camera mode " << gx_err << sendLog;

		if (ret == -1)
			reinitCamera ();

		return -2;
	}
	if (oldValue == fan)
	{
		int ret = gxccd_set_fan (camera, newValue->getValueInteger ());
		if (ret == 0)
			return 0;
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "cannot set fan " << gx_err << sendLog;

		if (ret == -1)
			reinitCamera ();

		return -2;
	}

	return Camera::setValue (oldValue, newValue);
}

int GXCCD::setCoolTemp (float new_temp)
{
	float val;

	if (hasCooling == false)
		return -2;

	int ret = gxccd_get_value (camera, GV_CHIP_TEMPERATURE, &val);
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "cannot retrieve chip temperature, cannot start cooling: " << gx_err << sendLog;

		if (ret == -1)
			reinitCamera ();

		return -1;
	}
	deleteTimers (EVENT_TE_RAMP);
	tempTarget->setValueFloat (val);
	addTimer (1, new rts2core::Event (EVENT_TE_RAMP));
	return Camera::setCoolTemp (new_temp);
}

int GXCCD::switchCooling (bool cooling)
{
	int ret;

	if (hasCooling == false)
		return -2;

	Camera::switchCooling (cooling);

	if (cooling)
	{
		if (std::isnan (tempSet->getValueFloat ()))
		{
			if (!nightCoolTemp || std::isnan (nightCoolTemp->getValueFloat ()))
			{
				logStream (MESSAGE_ERROR) << "nightCoolTemp not defined" << sendLog;
				ret = -1;
			}
			else
				ret = setCoolTemp (nightCoolTemp->getValueFloat ());
		}
		else
			ret = setCoolTemp (tempSet->getValueFloat ());
	}
	else
	{
		ret = setCoolTemp (50.0);
	}
	return ret;

}

int GXCCD::setFilterNum (int new_filter, const char *fn)
{
	if (fn != NULL)
		return Camera::setFilterNum (new_filter, fn);
	logStream (MESSAGE_INFO) << "moving filter from #" << camFilterVal->getValueInteger () << " (" << camFilterVal->getSelName () << ")" << " to #" << new_filter << " (" << camFilterVal->getSelName (new_filter) << ")" << sendLog;
	double last_filter_move = getNow ();
	int ret = gxccd_set_filter (camera, new_filter) ? -1 : 0;
	last_filter_move = getNow () - last_filter_move;
	if (ret == 0)
	{
		logStream (MESSAGE_INFO) << "filter moved to #" << new_filter << " (" << camFilterVal->getSelName (new_filter) << ")" << " in " << std::setprecision (3) << last_filter_move << "s" << sendLog;
	}
	else
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_INFO) << "filter movement error " << gx_err << sendLog;
		filterFailed->inc ();
		valueError (filterFailed);
		sendValueAll (filterFailed);
	}
	checkQueuedExposures ();
	return ret;
}

int GXCCD::startExposure ()
{
	int ret;

	ret = gxccd_set_read_mode (camera, mode->getValueInteger ());
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "GXCCD::startExposure error calling gxccd_set_read_mode " << gx_err << sendLog;
		if (ret == -1)
			reinitCamera ();
		return -1;
	}

	ret = gxccd_set_binning (camera, binningHorizontal (), binningVertical ());
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "GXCCD::startExposure error calling gxccd_set_binning " << gx_err << sendLog;
		if (ret == -1)
			reinitCamera ();
		return -1;
	}

	// The GXCCD library expects pre-binned region coordinates and size
	ret = gxccd_start_exposure (camera, getExposure (), getExpType () == 0,
								getUsedXBinned (), getUsedYBinned (),
								getUsedWidthBinned (), getUsedHeightBinned ());
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "GXCCD::startExposure error calling gxccd_start_exposure " << gx_err << sendLog;
		if (ret == -1)
			reinitCamera ();
		return -1;
	}
	return 0;
}

int GXCCD::stopExposure ()
{
 	int ret;
	ret = gxccd_abort_exposure (camera, false);
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "cannot stop exposure " << gx_err << sendLog;
		if (ret == -1)
			reinitCamera ();
		return -1;
	}
	return Camera::stopExposure ();
}

int GXCCD::doReadout ()
{
	int ret;
	bool ready;

	ret = gxccd_image_ready (camera, &ready);
	if (ret)
		return -1;
	if (ready == false)
		return 100;

	ssize_t s = 2 * getUsedWidthBinned () * getUsedHeightBinned ();

	if (getWriteBinaryDataSize () == s)
	{
		ret = gxccd_read_image (camera, getDataBuffer (0), s);
		if (ret < 0)
		{
			gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
			logStream (MESSAGE_ERROR) << "data read error: " << gx_err << sendLog;
			return -1;
		}
	}

	ret = sendReadoutData (getDataBuffer (0), getWriteBinaryDataSize ());
	if (ret < 0)
		return ret;
	if (getWriteBinaryDataSize () == 0)
		return -2;
	return 0;
}

int GXCCD::scriptEnds ()
{
	return Camera::scriptEnds ();
}

int GXCCD::clearCCD (double pref_time, int nclear)
{
	int ret = gxccd_set_preflash (camera, pref_time, nclear);
	if (ret == -1)
		reinitCamera ();
	if (ret)
		return -1;
	return 0;
}

int GXCCD::reinitCamera ()
{
	int ret = 0;
	float val;
	bool quiet = false;

	raiseHWError ();

	if (camera)
	{
		logStream (MESSAGE_WARNING) << "reinitializing camera" << sendLog;
		gxccd_release (camera);
	}
	else
		quiet = true;

	camera = gxccd_initialize_usb (id->getValueInteger ());
	if (camera == NULL) {
		if (!quiet)
			logStream (MESSAGE_ERROR) << "reinitialization failed: " << gx_err << sendLog;
		return -1;
	}

	if (fan)
	{
		ret = gxccd_set_fan (camera, fan->getValueInteger ());
		if (ret)
		{
			gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
			logStream (MESSAGE_ERROR) << "reinitilization failed - cannot set fan: " << gx_err << sendLog;
			return -1;
		}
	}

	if (mode)
	{
		ret = gxccd_set_read_mode (camera, mode->getValueInteger ());
		if (ret)
		{
			gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
			logStream (MESSAGE_ERROR) << "reinitilization failed - cannot set read mode: " << gx_err << sendLog;
			return -1;
		}
	}

	ret = gxccd_get_value (camera, GV_CHIP_TEMPERATURE, &val);
	if (ret)
	{
		gxccd_get_last_error (camera, gx_err, sizeof (gx_err));
		logStream (MESSAGE_ERROR) << "reinitilization failed - cannot get temperature: " << gx_err << sendLog;
		return -1;
	}

	logStream (MESSAGE_WARNING) << "reinitialization succeeded" << sendLog;

	clearHWError ();
	return 0;
}

int GXCCD::commandAuthorized (rts2core::Connection * conn)
{
	if (conn->isCommand ("clear"))
	{
		double pref_time;
		int nclear;
		if (conn->paramNextDouble (&pref_time) || conn->paramNextInteger (&nclear) || !conn->paramEnd ())
			return -2;
		return clearCCD (pref_time, nclear);
	}
	return Camera::commandAuthorized (conn);
}

int main (int argc, char **argv)
{
	GXCCD device (argc, argv);
	return device.run ();
}
