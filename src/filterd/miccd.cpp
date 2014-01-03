/*
 * Driver for Moravian Instruments (Moravske pristroje) Filter wheel.
 * Copyright (C) 2014 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "filterd.h"
#include "miccd.h"

#include <sys/types.h>
#include <fcntl.h>

namespace rts2filterd
{
/**
 * Driver for MI Filter wheel. http://ccd.mii.cz
 *
 * @author Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
 */
class MICCD:public Filterd
{
	public:
		MICCD (int argc, char **argv);
		virtual ~MICCD ();

	protected:
		virtual int processOption (int opt);
		virtual int initHardware ();
		virtual int initValues ();

		virtual int info ();

		virtual int setFilterNum (int new_filter);

	private:
		int reinitFW ();

		rts2core::ValueInteger *id;
		rts2core::ValueInteger *power;

		rts2core::ValueFloat *tempCCD;
		rts2core::ValueFloat *tempAir;

		camera_t camera;
		camera_info_t cami;
};

}

using namespace rts2filterd;

MICCD::MICCD (int argc, char **argv):Filterd (argc, argv)
{
	createValue (id, "product_id", "camera product identification", true);
	id->setValueInteger (0);

	createValue (power, "TEMPPWR", "[%] utilization of cooling power", true);
	createValue (tempCCD, "TEMPCCD", "[C] CCD temperature", true);
	createValue (tempAir, "TEMPAIR", "[C] ambient temperature", true);

	addOption ('p', NULL, 1, "MI CCD product ID");
}

MICCD::~MICCD ()
{
	miccd_close (&camera);
}

int MICCD::processOption (int opt)
{
	switch (opt)
	{
		case 'p':
			id->setValueCharArr (optarg);
			break;
		default:
			return Filterd::processOption (opt);
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
	setFilterNum (0);
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

	id->setValueInteger (cami.id);

//	initCameraChip (cami.w, cami.h, cami.pw, cami.ph);

	return Filterd::initValues ();
}

int MICCD::info ()
{
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
			case G11400:
			case G12000:
			case G2:
			case G3:
				ret = reinitFW ();
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
		case G11400:
		case G12000:
			break;
		case G2:
		case G3:
			ret = miccd_environment_temperature (&camera, &val);
			if (ret)
				return -1;
			tempAir->setValueFloat (val);
			break;
	}
	ret = miccd_power_voltage (&camera, &ival);
	if (ret)
		return -1;
	power->setValueInteger (ival);
	return Filterd::info ();
}

int MICCD::setFilterNum (int new_filter)
{
	int ret = miccd_filter (&camera, new_filter) ? -1 : 0;
	if (ret)
		return ret;
	return Filterd::setFilterNum (new_filter);
}

int MICCD::reinitFW ()
{
	logStream (MESSAGE_WARNING) << "reinitiliazing camera - this should not happen" << sendLog;
	miccd_close (&camera);
	if (miccd_open (id->getValueInteger (), &camera))
		return -1;
	maskState (DEVICE_ERROR_MASK, 0, "all errors cleared");
	return 0;
}

int main (int argc, char **argv)
{
	MICCD device (argc, argv);
	return device.run ();
}
