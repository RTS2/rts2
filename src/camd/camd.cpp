/* 
 * Basic camera daemon
 * Copyright (C) 2001-2007 Petr Kubanek <petr@kubanek.net>
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "camd.h"
#include "rts2devcliwheel.h"
#include "rts2devclifocuser.h"

void
Rts2DevCamera::initData ()
{
	readoutConn = NULL;

	pixelX = nan ("f");
	pixelY = nan ("f");
	readoutLine = -1;
	sendLine = -1;
	shutter_state = -1;

	nAcc = 1;

	focusingData = NULL;
	focusingDataTop = NULL;
}


void
Rts2DevCamera::initCameraChip ()
{
	initData ();
}


void
Rts2DevCamera::initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY)
{
	initData ();
	setSize (in_width, in_height, 0, 0);
	pixelX = in_pixelX;
	pixelY = in_pixelY;
}


int
Rts2DevCamera::setBinning (int in_vert, int in_hori)
{
	return 0;
}


int
Rts2DevCamera::box (int in_x, int in_y, int in_width, int in_height)
{
	// tests for -1 -> full size
	if (in_x == -1)
		in_x = chipSize->getXInt ();
	if (in_y == -1)
		in_y = chipSize->getYInt ();
	if (in_width == -1)
		in_width = chipSize->getWidthInt ();
	if (in_height == -1)
		in_height = chipSize->getHeightInt ();
	if (in_x < chipSize->getXInt () || in_y < chipSize->getYInt ()
		|| ((in_x - chipSize->getXInt ()) + in_width) > chipSize->getWidthInt ()
		|| ((in_y - chipSize->getYInt ()) + in_height) > chipSize->getHeightInt ())
		return -1;
	chipUsedReadout->setInts (in_x, in_y, in_width, in_height);
	return 0;
}


int
Rts2DevCamera::center (int in_w, int in_h)
{
	int x, y, w, h;
	if (in_w > 0 && chipSize->getWidthInt () >= in_w)
	{
		w = in_w;
		x = chipSize->getWidthInt () / 2 - w / 2;
	}
	else
	{
		w = chipSize->getWidthInt () / 2;
		x = chipSize->getWidthInt () / 4;
	}
	if (in_h > 0 && chipSize->getHeightInt () >= in_h)
	{
		h = in_h;
		y = chipSize->getHeightInt () / 2 - h / 2;
	}
	else
	{
		h = chipSize->getHeightInt () / 2;
		y = chipSize->getHeightInt () / 4;
	}
	return box (x, y, w, h);
}


int
Rts2DevCamera::setExposure (float exptime, int in_shutter_state)
{
	exposureEnd->setValueDouble (getNow () + exptime);

	sendValueAll (exposureEnd);

	shutter_state = in_shutter_state;
	return 0;
}


/**
 * Check if exposure has ended.
 *
 * @return 0 if there was pending exposure which ends, -1 if there wasn't any exposure, > 0 time remainnign till end of exposure
 */
long
Rts2DevCamera::isExposing ()
{
	if (exposureEnd->getValueDouble () == 0)
		return -1;				 // no exposure running
	if (getNow () > exposureEnd->getValueDouble ())
	{
		return 0;				 // exposure ended
	}
								 // timeout
	return ((long int) ((exposureEnd->getValueDouble () - getNow ()) * USEC_SEC));
}


int
Rts2DevCamera::endExposure ()
{
	int ret;
	exposureEnd->setValueDouble (0);
	if (isFocusing ())
	{
		// fake readout - only to memory
		ret = camReadout ();
		if (ret)
			endFocusing ();
	}
	else if (exposureConn)
	{
		return camReadout (exposureConn);
	}
	return 0;
}


int
Rts2DevCamera::stopExposure ()
{
	return endExposure ();
}


int
Rts2DevCamera::sendReadoutData (char *data, size_t data_size)
{
	int ret;
	time_t now;
	if (!readoutConn)
	{
		if (isFocusing ())
		{
			return processData (data, data_size);
		}
		// completly strange then..
		return -1;
	}
	ret = readoutConn->sendMsg (data, data_size);
	if (ret == -2)
	{
		time (&now);
		if (now > readout_started + readoutConn->getConnTimeout ())
		{
			logStream (MESSAGE_ERROR) <<
				"Chip sendReadoutData connection not established within timeout"
				<< sendLog;
			return -1;
		}
	}
	if (ret == -1)
	{
		logStream (MESSAGE_ERROR) << "Chip sendReadoutData " << strerror (errno)
			<< sendLog;
	}
	return ret;
}


int
Rts2DevCamera::processData (char *data, size_t size)
{
	memcpy (focusingDataTop, data, size);
	focusingDataTop += size;
	return size;
}


int
Rts2DevCamera::doFocusing ()
{
	// dummy method to write data to see it's working as expected
	int f = open ("/tmp/focusing.dat", O_CREAT | O_TRUNC | O_WRONLY);
	write (f, focusingData, focusingHeader.sizes[0] * focusingHeader.sizes[1] * 2);
	close (f);
	return 0;
}


int
Rts2DevCamera::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
	char *msg;
	char address[200];
	setReadoutConn (dataConn);
	if (dataConn && conn)
	{
		dataConn->getAddress ((char *) &address, 200);

		asprintf (&msg, "D connect %s %i %i", address, dataConn->getLocalPort (),
			chipUsedSize () * usedPixelByteSize () + sizeof (imghdr));
		conn->sendMsg (msg);
		free (msg);
	}
	return 0;
}


void
Rts2DevCamera::setReadoutConn (Rts2DevConnData * dataConn)
{
	readoutConn = dataConn;
	readoutLine = 0;
	sendLine = 0;
	time (&readout_started);
}


int
Rts2DevCamera::deleteConnection (Rts2Conn * conn)
{
	if (conn == readoutConn)
	{
		readoutConn = NULL;
	}
	if (conn == exposureConn)
	{
		exposureConn = NULL;
	}
	return Rts2Device::deleteConnection (conn);
}


int
Rts2DevCamera::endReadout ()
{
	if (isFocusing ())
	{
		int ret;
		ret = doFocusing ();
		if (ret)
		{
			delete[]focusingData;
			focusingData = NULL;
			focusingDataTop = NULL;
			endFocusing ();
		}
	}
	clearReadout ();
	if (readoutConn)
	{
		readoutConn->endConnection ();
		readoutConn = NULL;
	}
	return 0;
}


void
Rts2DevCamera::clearReadout ()
{
	readoutLine = -1;
	sendLine = -1;
}


int
Rts2DevCamera::sendFirstLine ()
{
	int w, h;
	w = chipUsedReadout->getWidthInt () / binningHorizontal ();
	h = chipUsedReadout->getHeightInt () / binningVertical ();
	if (isFocusing ())
	{
		// alocate data..
		if (!focusingData || focusingHeader.sizes[0] < w
			|| focusingHeader.sizes[1] < h)
		{
			delete[]focusingData;
			focusingData = new char[w * h * 2];
		}
		focusingDataTop = focusingData;
	}
	focusingHeader.data_type = getDataType ();
	focusingHeader.naxes = 2;
	focusingHeader.sizes[0] = chipUsedReadout->getWidthInt () / binningHorizontal ();
	focusingHeader.sizes[1] = chipUsedReadout->getHeightInt () / binningVertical ();
	focusingHeader.binnings[0] = binningVertical ();
	focusingHeader.binnings[1] = binningHorizontal ();
	focusingHeader.x = chipUsedReadout->getXInt ();
	focusingHeader.y = chipUsedReadout->getYInt ();
	focusingHeader.filter = getLastFilterNum ();
	focusingHeader.shutter = shutter_state;
	focusingHeader.subexp = subExposure->getValueDouble ();
	focusingHeader.nacc = nAcc;
	if (readoutConn)
	{
		int ret;
		ret = sendReadoutData ((char *) &focusingHeader, sizeof (imghdr));
		if (ret == -2)
			return 100;			 // not yet connected, wait for connection..
		if (ret > 0)			 // data send sucessfully
			return 0;
		return ret;				 // can be -1 as well
	}
	else if (isFocusing ())
		return 0;
	return -1;
}


int
Rts2DevCamera::readoutOneLine ()
{
	return -1;
}


bool
Rts2DevCamera::supportFrameTransfer ()
{
	return false;
}


int
Rts2DevCamera::setSubExposure (double in_subexposure)
{
	subExposure->setValueDouble (in_subexposure);
	return 0;
}


Rts2DevCamera::Rts2DevCamera (int in_argc, char **in_argv):
Rts2ScriptDevice (in_argc, in_argv, DEVICE_TYPE_CCD, "C0")
{
	tempAir = NULL;
	tempCCD = NULL;
	tempSet = NULL;
	tempRegulation = NULL;
	coolingPower = NULL;
	fan = NULL;
	filter = NULL;
	ccdType[0] = '\0';
	ccdRealType = ccdType;
	serialNumber[0] = '\0';

	createValue (exposureEnd, "exposure_end", "expected end of exposure", false);

	createValue (chipSize, "SIZE", "chip size", true, RTS2_VALUE_INTEGER);
	createValue (chipUsedReadout, "READT", "used chip subframe", true, RTS2_VALUE_INTEGER, CAM_WORKING);

	createValue (binning, "binning", "chip binning", true, 0, CAM_WORKING, true);
	createValue (dataType, "data_type", "used data type", true, 0, CAM_WORKING, true);

	createValue (exposure, "exposure", "current exposure time", false);
	createValue (subExposure, "subexposure", "current subexposure", false, 0, CAM_WORKING, true);
	createValue (camFilterVal, "filter", "used filter number", false, 0, CAM_EXPOSING, false);

	createValue (camFocVal, "focpos", "position of focuser", false, 0, CAM_EXPOSING, true);

	createValue (camShutterVal, "shutter", "shutter position", false);

	defaultSubExposure = nan ("f");

	nightCoolTemp = nan ("f");
	focuserDevice = NULL;
	wheelDevice = NULL;

	exposureConn = NULL;

	defBinning = 1;
	defFocusExposure = 10;

	createValue (rnoise, "RNOISE", "CCD readout noise");

	// cooling & other options..
	addOption ('c', "cooling_temp", 1, "default night cooling temperature");
	addOption ('F', "focuser", 1, "name of focuser device, which will be granted to do exposures without priority");
	addOption ('W', "filterwheel", 1, "name of device which is used as filter wheel");
	addOption ('b', "default_bin", 1, "default binning (ussualy 1)");
	addOption ('e', "focexp", 1, "starting focusing exposure duration");
	addOption ('s', "subexposure", 1, "default subexposure");
	addOption ('t', "type", 1, "specify camera type (in case camera do not store it in FLASH ROM)");
}


Rts2DevCamera::~Rts2DevCamera ()
{
	delete filter;

	delete focusingData;
}


int
Rts2DevCamera::willConnect (Rts2Address * in_addr)
{
	if (wheelDevice && in_addr->getType () == DEVICE_TYPE_FW
		&& in_addr->isAddress (wheelDevice))
		return 1;
	if (focuserDevice && in_addr->getType () == DEVICE_TYPE_FOCUS
		&& in_addr->isAddress (focuserDevice))
		return 1;
	return Rts2ScriptDevice::willConnect (in_addr);
}


Rts2DevClient *
Rts2DevCamera::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_FW:
			return new Rts2DevClientFilterCamera (conn);
		case DEVICE_TYPE_FOCUS:
			return new Rts2DevClientFocusCamera (conn);
	}
	return Rts2ScriptDevice::createOtherType (conn, other_device_type);
}


void
Rts2DevCamera::cancelPriorityOperations ()
{
	stopExposure ();
	if (isFocusing ())
		endFocusing ();
	endReadout ();
	delete[]focusingData;
	focusingData = NULL;
	focusingDataTop = NULL;
	setSubExposure (nan ("f"));
	nAcc = 1;
	box (-1, -1, -1, -1);

	maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_DATA | CAM_MASK_READING,
		CAM_NOEXPOSURE | CAM_NODATA | CAM_NOTREADING,
		BOP_TEL_MOVE, 0, "chip exposure interrupted");
	setBinning (defBinning, defBinning);

	setTimeout (USEC_SEC);
	// init states etc..
	clearStatesPriority ();
	setSubExposure (defaultSubExposure);
	Rts2ScriptDevice::cancelPriorityOperations ();
}


int
Rts2DevCamera::info ()
{
	camFilterVal->setValueInteger (getFilterNum ());
	camFocVal->setValueInteger (getFocPos ());
	return Rts2ScriptDevice::info ();
}


int
Rts2DevCamera::scriptEnds ()
{
	box (-1, -1, -1, -1);
	setBinning (defBinning, defBinning);

	setTimeout (USEC_SEC);
	return Rts2ScriptDevice::scriptEnds ();
}


int
Rts2DevCamera::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'c':
			nightCoolTemp = atof (optarg);
			break;
		case 'F':
			focuserDevice = optarg;
			break;
		case 'W':
			wheelDevice = optarg;
			break;
		case 'b':
			defBinning = atoi (optarg);
			break;
		case 'e':
			defFocusExposure = atof (optarg);
			break;
		case 's':
			defaultSubExposure = atof (optarg);
			setSubExposure (defaultSubExposure);
			break;
		case 't':
			ccdRealType = optarg;
			break;
		default:
			return Rts2ScriptDevice::processOption (in_opt);
	}
	return 0;
}


int
Rts2DevCamera::initChips ()
{
	int ret;
	if (defBinning != 1)
		setBinning (defBinning, defBinning);
	// init filter
	if (filter)
	{
		ret = filter->init ();
		if (ret)
		{
			return ret;
		}
	}

	return 0;
}


void
Rts2DevCamera::addBinning2D (int bin_v, int bin_h)
{
	Binning2D *bin = new Binning2D (bin_v, bin_h);
	binning->addSelVal (bin->getDescription ().c_str (), bin);
}


void
Rts2DevCamera::initBinnings ()
{
	addBinning2D (1,1);
}


void
Rts2DevCamera::addDataType (int in_type)
{
	const struct
	{
		const char* typeName;
		int type;
	}
	*t, types [] =
	{
		{ "BYTE", RTS2_DATA_BYTE },
		{ "SHORT", RTS2_DATA_SHORT },
		{ "LONG", RTS2_DATA_LONG },
		{ "LONG LONG", RTS2_DATA_LONGLONG },
		{ "FLOAT", RTS2_DATA_FLOAT },
		{ "DOUBLE", RTS2_DATA_DOUBLE },
		{ "SIGNED BYTE", RTS2_DATA_SBYTE },
		{ "UNSIGNED SHORT", RTS2_DATA_USHORT },
		{ "UNSIGNED LONG", RTS2_DATA_ULONG },
	};
	for (t=types; t->typeName; t++)
	{
		if (t->type == in_type)
		{
			dataType->addSelVal (t->typeName, new DataType (in_type));
			return;
		}
	}
	std::cerr << "Cannot find type: " << in_type << std::endl;
	exit (1);
}


void
Rts2DevCamera::initDataTypes ()
{
	dataType->addSelVal ("UNSIGNED SHORT", new DataType (RTS2_DATA_USHORT));
}


int
Rts2DevCamera::initValues ()
{
	// TODO init focuser - try to read focuser offsets & initial position from file
	addConstValue ("focuser", focuserDevice);
	addConstValue ("wheel", wheelDevice);

	addConstValue ("CCD_TYPE", "camera type", ccdRealType);
	addConstValue ("CCD_SER", "camera serial number", serialNumber);

	addConstValue ("chips", 1);

	addConstValue ("pixelX", "X pixel size", pixelX);
	addConstValue ("pixelY", "Y pixel size", pixelY);

	initBinnings ();
	initDataTypes ();

	return Rts2ScriptDevice::initValues ();
}


void
Rts2DevCamera::checkExposures ()
{
	long ret;
	if (getStateChip (0) & CAM_EXPOSING)
	{
		// try to end exposure
		ret = camWaitExpose ();
		if (ret >= 0)
		{
			setTimeout (ret);
		}
		else
		{
			if (ret == -2)
			{
				maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_DATA,
					CAM_NOEXPOSURE | CAM_DATA, BOP_TEL_MOVE, 0,
					"exposure chip finished");
				endExposure ();
			}
			if (ret == -1)
			{
				maskStateChip (0,
					DEVICE_ERROR_MASK | CAM_MASK_EXPOSE |
					CAM_MASK_DATA,
					DEVICE_ERROR_HW | CAM_NOEXPOSURE |
					CAM_NODATA, BOP_TEL_MOVE, 0,
					"exposure chip finished with error");
				stopExposure ();
			}
		}
	}
}


void
Rts2DevCamera::checkReadouts ()
{
	int ret;
	if ((getStateChip (0) & CAM_MASK_READING) != CAM_READING)
		return;
	ret = readoutOneLine ();
	if (ret >= 0)
	{
		setTimeout (ret);
	}
	else
	{
		endReadout ();
		afterReadout ();
		if (ret == -2)
			maskStateChip (0, CAM_MASK_READING, CAM_NOTREADING, BOP_TEL_MOVE,
				0, "chip readout ended");
		else
			maskStateChip (0, DEVICE_ERROR_MASK | CAM_MASK_READING,
				DEVICE_ERROR_HW | CAM_NOTREADING, BOP_TEL_MOVE, 0,
				"chip readout ended with error");
	}
}


void
Rts2DevCamera::afterReadout ()
{
	setTimeout (USEC_SEC);
}


int
Rts2DevCamera::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == camFocVal)
	{
		return setFocuser (new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == subExposure)
	{
		return setSubExposure (new_value->getValueDouble ()) == 0 ? 0 : -2;
	}
	if (old_value == camFilterVal)
	{
		return camFilter (new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == tempSet)
	{
		return camCoolTemp (new_value->getValueFloat ()) == 0 ? 0 : -2;
	}
	if (old_value == binning)
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		return setBinning (bin->horizontal, bin->vertical);
	}
	if (old_value == chipUsedReadout)
	{
		return 0;
	}
	return Rts2ScriptDevice::setValue (old_value, new_value);
}


void
Rts2DevCamera::deviceReady (Rts2Conn * conn)
{
	// if that's filter wheel
	if (wheelDevice && !strcmp (conn->getName (), wheelDevice)
		&& conn->getOtherDevClient ())
	{
		// copy content of device filter variable to our list..
		Rts2Value *val = conn->getValue ("filter");
		// it's filter and it's correct type
		if (val->getValueType () == RTS2_VALUE_SELECTION)
			camFilterVal->duplicateSelVals ((Rts2ValueSelection *) val);
	}
}


void
Rts2DevCamera::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_FILTER_MOVE_END:
		case EVENT_FOCUSER_END_MOVE:
			// update info about FW
			infoAll ();
			break;
	}
	Rts2ScriptDevice::postEvent (event);
}


int
Rts2DevCamera::idle ()
{
	int ret;
	checkExposures ();
	checkReadouts ();
	if (isIdle () && isFocusing ())
	{
		ret = camStartExposure (1, focusExposure);
		if (ret)
			endFocusing ();
	}
	return Rts2ScriptDevice::idle ();
}


int
Rts2DevCamera::changeMasterState (int new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_DUSK | SERVERD_STANDBY:
		case SERVERD_NIGHT | SERVERD_STANDBY:
		case SERVERD_DAWN | SERVERD_STANDBY:
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			camCoolHold ();
			break;
		case SERVERD_EVENING | SERVERD_STANDBY:
		case SERVERD_EVENING:
			camCoolMax ();
			break;
		default:
			camCoolShutdown ();
	}
	return Rts2ScriptDevice::changeMasterState (new_state);
}


int
Rts2DevCamera::camStartExposure (int light, float exptime)
{
	int ret;

	ret = camExpose (light, exptime);
	if (ret)
		return ret;

	exposure->setValueFloat (exptime);
	infoAll ();
	maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_DATA,
		CAM_EXPOSING | CAM_NODATA, BOP_TEL_MOVE, BOP_TEL_MOVE,
		"exposure chip started");
	setExposure (exptime, light ? SHUTTER_SYNCHRO : SHUTTER_CLOSED);
	lastFilterNum = getFilterNum ();
	// call us to check for exposures..
	long new_timeout;
	new_timeout = camWaitExpose ();
	if (new_timeout >= 0)
	{
		setTimeout (new_timeout);
	}
	return 0;
}


int
Rts2DevCamera::camExpose (Rts2Conn * conn, int light, float exptime)
{
	int ret;

	ret = camStartExposure (light, exptime);
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot exposure on chip");
	}
	else
	{
		exposureConn = conn;
	}
	return ret;
}


long
Rts2DevCamera::camWaitExpose ()
{
	int ret;
	ret = isExposing ();
	return (ret == 0 ? -2 : ret);
}


int
Rts2DevCamera::camStopExpose (Rts2Conn * conn)
{
	if (isExposing () >= 0)
	{
		maskStateChip (0, CAM_MASK_EXPOSE, CAM_NOEXPOSURE, BOP_TEL_MOVE,
			0, "exposure canceled");
		endExposure ();
		return camStopExpose ();
	}
	return -1;
}


int
Rts2DevCamera::camBox (Rts2Conn * conn, int x, int y, int width, int height)
{
	int ret;
	ret = box (x, y, width, height);
	if (!ret)
		return ret;
	conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "invalid box size");
	return ret;
}


int
Rts2DevCamera::camCenter (Rts2Conn * conn, int in_h, int in_w)
{
	int ret;
	ret = center (in_h, in_w);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "cannot set box size");
	return ret;
}


// when we don't have data connection - handy for exposing
int
Rts2DevCamera::camReadout ()
{
	int ret;
	maskStateChip (0, CAM_MASK_READING | CAM_MASK_DATA,
		CAM_READING | CAM_NODATA, BOP_TEL_MOVE, 0,
		"chip readout started");
	ret = startReadout (NULL, NULL);
	if (!ret)
	{
		return 0;
	}
	maskStateChip (0, DEVICE_ERROR_MASK | CAM_MASK_READING,
		DEVICE_ERROR_HW | CAM_NOTREADING, 0, 0,
		"chip readout failed");
	return -1;
}


int
Rts2DevCamera::camReadout (Rts2Conn * conn)
{
	int ret;
	// open data connection - wait socket

	Rts2DevConnData *data_conn;
	data_conn = new Rts2DevConnData (this, conn);

	ret = data_conn->init ();

	struct sockaddr_in our_addr;

	if (conn->getOurAddress (&our_addr) < 0)
	{
		delete data_conn;
		conn->sendCommandEnd (DEVDEM_E_SYSTEM, "cannot get our address");
		return -1;
	}

	// add data connection
	addConnection (data_conn);

	data_conn->setAddress (&our_addr.sin_addr);

	maskStateChip (0, CAM_MASK_READING | CAM_MASK_DATA,
		CAM_READING | CAM_NODATA, BOP_TEL_MOVE, 0,
		"chip readout started");
	ret = startReadout (data_conn, conn);
	if (!ret)
	{
		return 0;
	}
	maskStateChip (0, DEVICE_ERROR_MASK | CAM_MASK_READING,
		DEVICE_ERROR_HW | CAM_NOTREADING, 0, 0,
		"chip readout failed");
	conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
	return -1;
}


int
Rts2DevCamera::camBinning (Rts2Conn * conn, int x_bin, int y_bin)
{
	int ret;
	ret = setBinning (x_bin, y_bin);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set requested binning");
	return ret;
}


int
Rts2DevCamera::camStopRead (Rts2Conn * conn)
{
	int ret;
	ret = camStopRead ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot end readout");
	return ret;
}


int
Rts2DevCamera::camCoolMax (Rts2Conn * conn)
{
	int ret = camCoolMax ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set cooling mode to cool max");
	return ret;
}


int
Rts2DevCamera::camCoolHold (Rts2Conn * conn)
{
	int ret;
	ret = camCoolHold ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot set cooling mode to cool max");
	return ret;
}


int
Rts2DevCamera::camCoolTemp (Rts2Conn * conn, float new_temp)
{
	int ret;
	ret = camCoolTemp (new_temp);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW,
			"cannot set cooling temp to requested temperature");
	return ret;
}


int
Rts2DevCamera::camCoolShutdown (Rts2Conn * conn)
{
	int ret;
	ret = camCoolShutdown ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW,
			"cannot shutdown camera cooling system");
	return ret;
}


int
Rts2DevCamera::camFilter (int new_filter)
{
	int ret = -1;
	if (wheelDevice)
	{
		struct filterStart fs;
		fs.filterName = wheelDevice;
		fs.filter = new_filter;
		postEvent (new Rts2Event (EVENT_FILTER_START_MOVE, (void *) &fs));
		// filter move will be performed
		if (fs.filter == -1)
		{
			ret = 0;
		}
		else
		{
			ret = -1;
		}
	}
	else
	{
		ret = filter->setFilterNum (new_filter);
		Rts2ScriptDevice::infoAll ();
	}
	return ret;
}


int
Rts2DevCamera::getStateChip (int chip)
{
	return (getState () & (CAM_MASK_CHIP << (chip * 4))) >> (0 * 4);
}


void
Rts2DevCamera::maskStateChip (
int chip_num,
int chip_state_mask,
int chip_new_state,
int state_mask,
int new_state,
char *description
)
{
	maskState (state_mask | (chip_state_mask << (4 * chip_num)),
		new_state | (chip_new_state << (4 * chip_num)), description);
}


int
Rts2DevCamera::getFilterNum ()
{
	if (wheelDevice)
	{
		struct filterStart fs;
		fs.filterName = wheelDevice;
		fs.filter = -1;
		postEvent (new Rts2Event (EVENT_FILTER_GET, (void *) &fs));
		return fs.filter;
	}
	else if (filter)
	{
		return filter->getFilterNum ();
	}
	return 0;
}


int
Rts2DevCamera::setFocuser (int new_set)
{
	if (!focuserDevice)
	{
		return -1;
	}
	struct focuserMove fm;
	fm.focuserName = focuserDevice;
	fm.value = new_set;
	postEvent (new Rts2Event (EVENT_FOCUSER_SET, (void *) &fm));
	if (fm.focuserName)
		return -1;
	return 0;
}


int
Rts2DevCamera::stepFocuser (int step_count)
{
	if (!focuserDevice)
	{
		return -1;
	}
	struct focuserMove fm;
	fm.focuserName = focuserDevice;
	fm.value = step_count;
	postEvent (new Rts2Event (EVENT_FOCUSER_STEP, (void *) &fm));
	if (fm.focuserName)
		return -1;
	return 0;
}


int
Rts2DevCamera::getFocPos ()
{
	if (!focuserDevice)
		return -1;
	struct focuserMove fm;
	fm.focuserName = focuserDevice;
	postEvent (new Rts2Event (EVENT_FOCUSER_GET, (void *) &fm));
	if (fm.focuserName)
		return -1;
	return fm.value;
}


int
Rts2DevCamera::startFocus (Rts2Conn * conn)
{
	if (isFocusing ())
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "already performing autofocus");
		return -1;
	}
	focusExposure = defFocusExposure;
	// idle routine will check for that..
	maskState (CAM_MASK_FOCUSING, CAM_FOCUSING);
	return 0;
}


int
Rts2DevCamera::endFocusing ()
{
	maskState (CAM_MASK_FOCUSING, CAM_NOFOCUSING);
	// to reset binnings etc..
	scriptEnds ();
	return 0;
}


bool Rts2DevCamera::isIdle ()
{
	return ((getStateChip (0) &
		(CAM_MASK_EXPOSE | CAM_MASK_DATA | CAM_MASK_READING)) ==
		(CAM_NOEXPOSURE | CAM_NODATA | CAM_NOTREADING));
}


bool Rts2DevCamera::isFocusing ()
{
	return ((getStateChip (0) & CAM_MASK_FOCUSING) == CAM_FOCUSING);
}


int
Rts2DevCamera::commandAuthorized (Rts2Conn * conn)
{
	float exptime;
	int light;

	if (conn->isCommand ("help"))
	{
		conn->sendMsg ("ready - is camera ready?");
		conn->sendMsg ("info - information about camera");
		conn->sendMsg ("chipinfo <chip> - information about chip");
		conn->sendMsg ("expose <chip> <light> <exposure> - start exposition on given chip");
		conn->sendMsg ("stopexpo <chip> - stop exposition on given chip");
		conn->sendMsg ("progexpo <chip> - query exposition progress");
		conn->sendMsg ("mirror <open|close> - open/close mirror");
		conn->sendMsg ("binning <chip> <binning_id> - set new binning; actual from next readout on");
		conn->sendMsg ("stopread <chip> - stop reading given chip");
		conn->sendMsg ("cooltemp <temp> - cooling temperature");
		conn->sendMsg ("focus - try to autofocus picture");
		conn->sendMsg ("exit - exit from connection");
		conn->sendMsg ("help - print, what you are reading just now");
		return 0;
	}
	// commands which requires priority
	// priority test must come after command string test,
	// otherwise we will be unable to answer DEVDEM_E_PRIORITY
	else if (conn->isCommand ("expose"))
	{
		CHECK_PRIORITY;
		if (conn->paramNextInteger (&light)
			|| conn->paramNextFloat (&exptime) || !conn->paramEnd ())
			return -2;
		return camExpose (conn, light, exptime);
	}
	else if (conn->isCommand ("stopexpo"))
	{
		CHECK_PRIORITY;
		if (!conn->paramEnd ())
			return -2;
		return camStopExpose (conn);
	}
	else if (conn->isCommand ("box"))
	{
		int x, y, w, h;
		CHECK_PRIORITY;
		if (conn->paramNextInteger (&x)
			|| conn->paramNextInteger (&y)
			|| conn->paramNextInteger (&w) || conn->paramNextInteger (&h)
			|| !conn->paramEnd ())
			return -2;
		return camBox (conn, x, y, w, h);
	}
	else if (conn->isCommand ("center"))
	{
		CHECK_PRIORITY;
		int w, h;
		if (conn->paramEnd ())
		{
			w = -1;
			h = -1;
		}
		else
		{
			if (conn->paramNextInteger (&w) || conn->paramNextInteger (&h)
				|| !conn->paramEnd ())
				return -2;
		}
		return camCenter (conn, w, h);
	}
	else if (conn->isCommand ("readout"))
	{
		if (!conn->paramEnd ())
			return -2;
		return -2;
		return camReadout (conn);
	}
	else if (conn->isCommand ("binning"))
	{
		int vertical, horizontal;
		CHECK_PRIORITY;
		if (conn->paramNextInteger (&vertical)
			|| conn->paramNextInteger (&horizontal) || !conn->paramEnd ())
			return -2;
		return camBinning (conn, vertical, horizontal);
	}
	else if (conn->isCommand ("stopread"))
	{
		CHECK_PRIORITY;
		if (!conn->paramEnd ())
			return -2;
		return camStopRead (conn);
	}
	else if (conn->isCommand ("coolmax"))
	{
		return camCoolMax (conn);
	}
	else if (conn->isCommand ("coolhold"))
	{
		return camCoolHold (conn);
	}
	else if (conn->isCommand ("cooltemp"))
	{
		float new_temp;
		if (conn->paramNextFloat (&new_temp) || !conn->paramEnd ())
			return -2;
		return camCoolTemp (conn, new_temp);
	}
	else if (conn->isCommand ("focus"))
	{
		return startFocus (conn);
	}
	return Rts2ScriptDevice::commandAuthorized (conn);
}
