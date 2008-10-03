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
	pixelX = nan ("f");
	pixelY = nan ("f");

	nAcc = 1;
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
		in_x = 0;
	if (in_y == -1)
		in_y = 0;
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


long
Rts2DevCamera::isExposing ()
{
	double n = getNow ();
	if (n > exposureEnd->getValueDouble ())
	{
		return 0;				 // exposure ended
	}
								 // timeout
	return ((long int) ((exposureEnd->getValueDouble () - n) * USEC_SEC));
}


int
Rts2DevCamera::endExposure ()
{
	if (exposureConn)
	{
		logStream (MESSAGE_INFO)
			<< "end exposure for " << exposureConn->getName ()
			<< sendLog;

		return camReadout (exposureConn);
	}
	if (getStateChip (0) & CAM_EXPOSING)
		logStream (MESSAGE_WARNING)
			<< "end exposure without exposure connection"
			<< sendLog;
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
	maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_READING | CAM_MASK_FT,
		CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT,
		BOP_TEL_MOVE, 0, "chip exposure interrupted");
	return 0;
}


int
Rts2DevCamera::stopExposure ()
{
	return endExposure ();
}


int
Rts2DevCamera::processData (char *data, size_t size)
{
	return size;
}


int
Rts2DevCamera::deleteConnection (Rts2Conn * conn)
{
	if (conn == exposureConn)
	{
		exposureConn = NULL;
	}
	return Rts2Device::deleteConnection (conn);
}


int
Rts2DevCamera::endReadout ()
{
	clearReadout ();
	if (quedExpNumber->getValueInteger () > 0 && exposureConn)
	{
		// do not report that we start exposure
		camExpose (exposureConn, getStateChip(0) & CAM_MASK_EXPOSE, true);
	}
	return 0;
}


void
Rts2DevCamera::clearReadout ()
{
}


int
Rts2DevCamera::sendFirstLine ()
{
	int w, h;
	w = chipUsedReadout->getWidthInt () / binningHorizontal ();
	h = chipUsedReadout->getHeightInt () / binningVertical ();
	focusingHeader.data_type = getDataType ();
	focusingHeader.naxes = 2;
	focusingHeader.sizes[0] = chipUsedReadout->getWidthInt () / binningHorizontal ();
	focusingHeader.sizes[1] = chipUsedReadout->getHeightInt () / binningVertical ();
	focusingHeader.binnings[0] = binningVertical ();
	focusingHeader.binnings[1] = binningHorizontal ();
	focusingHeader.x = chipUsedReadout->getXInt ();
	focusingHeader.y = chipUsedReadout->getYInt ();
	focusingHeader.filter = getLastFilterNum ();
	// light - dark images
	if (expType)
		focusingHeader.shutter = expType->getValueInteger ();
	else
		focusingHeader.shutter = 0;
	focusingHeader.subexp = subExposure->getValueDouble ();
	focusingHeader.nacc = nAcc;

	return sendReadoutData ((char *) &focusingHeader, sizeof (imghdr));
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
	expType = NULL;

	tempAir = NULL;
	tempCCD = NULL;
	tempSet = NULL;
	nightCoolTemp = NULL;
	filter = NULL;
	ccdType[0] = '\0';
	ccdRealType = ccdType;
	serialNumber[0] = '\0';

	currentImageData = -1;

	createValue (quedExpNumber, "que_exp_num", "number of exposures in que", false, 0, 0, true);
	quedExpNumber->setValueInteger (0);

	createValue (exposureNumber, "exposure_num", "number of exposures camera takes", false, 0, 0, false);
	exposureNumber->setValueLong (0);

	createValue (exposureEnd, "exposure_end", "expected end of exposure", false);

	createValue (waitingForEmptyQue, "wait_for_que", "if camera is waiting for empty que", false);
	waitingForEmptyQue->setValueBool (false);

	createValue (waitingForNotBop, "wait_for_notbop", "if camera is waiting for not bop state", false);
	waitingForNotBop->setValueBool (false);

	createValue (chipSize, "SIZE", "chip size", true, RTS2_VALUE_INTEGER);
	createValue (chipUsedReadout, "READT", "used chip subframe", true, RTS2_VALUE_INTEGER, CAM_WORKING, true);

	createValue (binning, "binning", "chip binning", true, 0, CAM_WORKING, true);
	createValue (dataType, "data_type", "used data type", false, 0, CAM_WORKING, true);

	createValue (exposure, "exposure", "current exposure time", false, 0, CAM_WORKING);
	exposure->setValueDouble (1);

	sendOkInExposure = false;

	createValue (subExposure, "subexposure", "current subexposure", false, 0, CAM_WORKING, true);
	createValue (camFilterVal, "filter", "used filter number", false, 0, CAM_EXPOSING, false);

	createValue (camFocVal, "focpos", "position of focuser", false, 0, CAM_EXPOSING, true);

	createValue (rotang, "CCD_ROTA", "CCD rotang", true, RTS2_DT_ROTANG);
	rotang->setValueDouble (0);

	focuserDevice = NULL;
	wheelDevice = NULL;

	dataBuffer = NULL;
	dataBufferSize = -1;

	exposureConn = NULL;

	createValue (rnoise, "RNOISE", "CCD readout noise");

	// other options..
	addOption ('F', "focuser", 1, "name of focuser device, which will be granted to do exposures without priority");
	addOption ('W', "filterwheel", 1, "name of device which is used as filter wheel");
	addOption ('e', NULL, 1, "default exposure");
	addOption ('s', "subexposure", 1, "default subexposure");
	addOption ('t', "type", 1, "specify camera type (in case camera do not store it in FLASH ROM)");
	addOption ('r', NULL, 1, "camera rotang");
}


Rts2DevCamera::~Rts2DevCamera ()
{
	delete[] dataBuffer;
	delete filter;
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
Rts2DevCamera::checkQueChanges (int fakeState)
{
	// do not check if we have qued exposures
	if (quedExpNumber->getValueInteger () > 0)
		return;
	Rts2ScriptDevice::checkQueChanges (fakeState);
	if (queValues.empty ())
	{
		if (waitingForEmptyQue->getValueBool ())
		{
			waitingForEmptyQue->setValueBool (false);
			sendOkInExposure = true;
		}
		if (exposureConn)
		{
			connections_t::iterator iter;
			// ask all centralds for possible blocking devices
			for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
			{
				(*iter)->queCommand (new Rts2CommandDeviceStatusInfo (this, exposureConn));
			}
		}
	}
}


void
Rts2DevCamera::cancelPriorityOperations ()
{
	exposureConn = NULL;

	stopExposure ();
	endReadout ();
	nAcc = 1;

	maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_READING | CAM_MASK_FT,
		CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT,
		BOP_TEL_MOVE, 0, "chip exposure interrupted");

	setTimeout (USEC_SEC);
	// init states etc..
	clearStatesPriority ();
	// cancel any pending exposures
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
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

	setTimeout (USEC_SEC);
	return Rts2ScriptDevice::scriptEnds ();
}


int
Rts2DevCamera::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'F':
			focuserDevice = optarg;
			break;
		case 'W':
			wheelDevice = optarg;
			break;
		case 'e':
			exposure->setValueDouble (atof (optarg));
			break;
		case 's':
			setSubExposure (atof (optarg));
			break;
		case 't':
			ccdRealType = optarg;
			break;
		case 'c':
			nightCoolTemp->setValueFloat (atof (optarg));
			break;
		case 'r':
			rotang->setValueCharArr (optarg);
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


int
Rts2DevCamera::sendImage (char *data, size_t dataSize)
{
	if (!exposureConn)
		return -1;
	currentImageData = exposureConn->startBinaryData (dataSize + sizeof (imghdr), dataType->getValueInteger ());
	if (currentImageData == -1)
		return -1;
	sendFirstLine ();
	return exposureConn->sendBinaryData (currentImageData, data, dataSize);
}


int
Rts2DevCamera::sendReadoutData (char *data, size_t dataSize)
{
	if (exposureConn && currentImageData >= 0)
		return exposureConn->sendBinaryData (currentImageData, data, dataSize);
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
	addDataType (RTS2_DATA_USHORT);
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
			int expNum;
			switch (ret)
			{
				case -3:
					exposureConn = NULL;
					endExposure ();
					break;
				case -2:
					// remember exposure number
					expNum = exposureNumber->getValueInteger ();
					endExposure ();
					// if new exposure does not start during endExposure (camReadout) call, drop exposure state
					if (expNum == exposureNumber->getValueInteger ())
						maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_FT,
							CAM_NOEXPOSURE | CAM_NOFT,
							BOP_TEL_MOVE, 0,
							"exposure chip finished");

					// drop FT flag
					else
						maskStateChip (0, CAM_MASK_FT, CAM_NOFT,
							0, 0, "ft exposure chip finished");
					break;
				case -1:
					maskStateChip (0,
						DEVICE_ERROR_MASK | CAM_MASK_EXPOSE | CAM_MASK_READING,
						DEVICE_ERROR_HW | CAM_NOEXPOSURE | CAM_NOTREADING,
						BOP_TEL_MOVE, 0,
						"exposure chip finished with error");
					stopExposure ();
					if (quedExpNumber->getValueInteger () > 0)
					{
						logStream (MESSAGE_DEBUG) << "starting new exposure after camera failure" << sendLog;
						camExpose (exposureConn, getStateChip (0) & ~CAM_MASK_EXPOSE, true);
					}
					break;
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
			maskStateChip (0, CAM_MASK_READING, CAM_NOTREADING,
				0, 0, "chip readout ended");
		else
			maskStateChip (0, DEVICE_ERROR_MASK | CAM_MASK_READING,
				DEVICE_ERROR_HW | CAM_NOTREADING,
				0, 0, "chip readout ended with error");
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
	if (old_value == exposure
		|| old_value == quedExpNumber
		|| old_value == expType
		|| old_value == rotang)
	{
		return 0;
	}
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
		return camFilter (new_value->getValueInteger ()) == 0 ? 1 : -2;
	}
	if (old_value == tempSet)
	{
		return setCoolTemp (new_value->getValueFloat ()) == 0 ? 0 : -2;
	}
	if (old_value == binning)
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		return setBinning (bin->horizontal, bin->vertical);
	}
	if (old_value == chipUsedReadout)
	{
		Rts2ValueRectangle *rect = (Rts2ValueRectangle *) new_value;
		return box (rect->getXInt (), rect->getYInt (), rect->getWidthInt (), rect->getHeightInt ()) == 0 ? 0 : -2;
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
		{
			camFilterVal->duplicateSelVals ((Rts2ValueSelection *) val);
			// sends filter metainformations to all connected devices
			updateMetaInformations (camFilterVal);
		}
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
	checkExposures ();
	checkReadouts ();
	return Rts2ScriptDevice::idle ();
}


int
Rts2DevCamera::changeMasterState (int new_state)
{
	switch (new_state & (SERVERD_STATUS_MASK | SERVERD_STANDBY_MASK))
	{
		case SERVERD_EVENING | SERVERD_STANDBY:
		case SERVERD_DUSK | SERVERD_STANDBY:
		case SERVERD_NIGHT | SERVERD_STANDBY:
		case SERVERD_DAWN | SERVERD_STANDBY:
		case SERVERD_EVENING:
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
			beforeNight ();
			break;
		default:
			afterNight ();
	}
	return Rts2ScriptDevice::changeMasterState (new_state);
}


int
Rts2DevCamera::camStartExposure ()
{
	// check if we aren't blocked
	if ((!expType || expType->getValueInteger () == 0)
		&& (getDeviceBopState () & BOP_EXPOSURE))
	{
		if (!waitingForNotBop->getValueBool ())
		{
			quedExpNumber->inc ();
			sendValueAll (quedExpNumber);
			waitingForNotBop->setValueBool (true);
			sendValueAll (waitingForNotBop);
		}

		return 0;
	}

	return camStartExposureWithoutCheck ();
}


int
Rts2DevCamera::camStartExposureWithoutCheck ()
{
	int ret;

	exposureNumber->inc ();
	sendValueAll (exposureNumber);

	ret = startExposure ();
	if (ret)
		return ret;

	infoAll ();
	maskStateChip (0, CAM_MASK_EXPOSE, CAM_EXPOSING,
		BOP_TEL_MOVE, BOP_TEL_MOVE, "exposure chip started");

	exposureEnd->setValueDouble (getNow () + exposure->getValueDouble ());
	sendValueAll (exposureEnd);

	lastFilterNum = getFilterNum ();
	// call us to check for exposures..
	long new_timeout;
	new_timeout = camWaitExpose ();
	if (new_timeout >= 0)
	{
		setTimeout (new_timeout);
	}

	// increas buffer size
	if (dataBufferSize < suggestBufferSize ())
	{
		delete[] dataBuffer;
		dataBufferSize = suggestBufferSize ();
		dataBuffer = new char[dataBufferSize];
	}

	return 0;
}


int
Rts2DevCamera::camExpose (Rts2Conn * conn, int chipState, bool fromQue)
{
	int ret;

	// if it is currently exposing
	// or performing other op that can block command execution
	if ((chipState & CAM_EXPOSING)
		|| (((chipState & CAM_READING)
		&& !supportFrameTransfer ()))
		)
	{
		if (!fromQue)
		{
			quedExpNumber->inc ();
			sendValueAll (quedExpNumber);
			if (queValues.empty ())
			{
				return 0;
			}
		}
		// need to wait to empty que of value changes
		waitingForEmptyQue->setValueBool (true);
		sendValueAll (waitingForEmptyQue);
		// do not send OK, send it after we finish
		return -1;
	}
	if (quedExpNumber->getValueInteger () > 0)
	{
		quedExpNumber->dec ();
		sendValueAll (quedExpNumber);
	}

	ret = camStartExposure ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot exposure on chip");
	}
	else
	{
		// check if that comes from old request
		if (sendOkInExposure && exposureConn)
		{
			sendOkInExposure = false;
			exposureConn->sendCommandEnd (DEVDEM_OK, "Executing exposure from que");
		}
		exposureConn = conn;
		logStream (MESSAGE_INFO) << "exposing for '"
			<< (conn ? conn->getName () : "null") << "'" << sendLog;
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


int
Rts2DevCamera::readoutStart ()
{
	return sendFirstLine ();
}


int
Rts2DevCamera::camReadout (Rts2Conn * conn)
{
	// if we can do exposure, do it..
	if (quedExpNumber->getValueInteger () > 0 && exposureConn && supportFrameTransfer ())
	{
		quedExpNumber->dec ();
		// update que changes..
		checkQueChanges (getStateChip (0) & ~CAM_EXPOSING);
		quedExpNumber->inc ();
		maskStateChip (0, CAM_MASK_READING | CAM_MASK_FT, CAM_READING | CAM_FT,
			0, 0, "starting frame transfer");
		currentImageData = conn->startBinaryData (chipByteSize () + sizeof (imghdr), dataType->getValueInteger ());
		if (queValues.empty ())
			// remove exposure flag from state
			camExpose (exposureConn, getStateChip (0) & ~CAM_MASK_EXPOSE, true);
	}
	else
	{
		// open data connection - wait socket
		// end exposure as well..
		// do not signal BOP_TEL_MOVE down if there are exposures in que
		maskStateChip (0, CAM_MASK_EXPOSE | CAM_MASK_READING,
			CAM_NOEXPOSURE | CAM_READING,
			BOP_TEL_MOVE, 0,
			"chip readout started");
		currentImageData = conn->startBinaryData (chipByteSize () + sizeof (imghdr), dataType->getValueInteger ());
	}

	if (currentImageData >= 0)
	{
		return readoutStart ();
	}
	maskStateChip (0, DEVICE_ERROR_MASK | CAM_MASK_READING,
		DEVICE_ERROR_HW | CAM_NOTREADING, 0, 0,
		"chip readout failed");
	conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
	return -1;
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
const char *description
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


bool Rts2DevCamera::isIdle ()
{
	return ((getStateChip (0) &
		(CAM_MASK_EXPOSE | CAM_MASK_READING)) ==
		(CAM_NOEXPOSURE | CAM_NOTREADING));
}


int
Rts2DevCamera::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("help"))
	{
		conn->sendMsg ("ready - is camera ready?");
		conn->sendMsg ("info - information about camera");
		conn->sendMsg ("chipinfo <chip> - information about chip");
		conn->sendMsg ("expose - start exposition");
		conn->sendMsg ("stopexpo <chip> - stop exposition on given chip");
		conn->sendMsg ("progexpo <chip> - query exposition progress");
		conn->sendMsg ("mirror <open|close> - open/close mirror");
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
		if (!conn->paramEnd ())
			return -2;
		return camExpose (conn, getStateChip (0), false);
	}
	else if (conn->isCommand ("stopexpo"))
	{
		CHECK_PRIORITY;
		if (!conn->paramEnd ())
			return -2;
		return stopExposure ();
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
		//TODO send data
		return -2;
	}
	else if (conn->isCommand ("stopread"))
	{
		CHECK_PRIORITY;
		if (!conn->paramEnd ())
			return -2;
		return camStopRead (conn);
	}
	return Rts2ScriptDevice::commandAuthorized (conn);
}


int
Rts2DevCamera::maskQueValueBopState (int new_state, int valueQueCondition)
{
	if (valueQueCondition & CAM_EXPOSING)
		new_state |= BOP_EXPOSURE;
	if (valueQueCondition & CAM_READING)
		new_state |= BOP_READOUT;
	return new_state;
}


void
Rts2DevCamera::setFullBopState (int new_state)
{
	Rts2Device::setFullBopState (new_state);
	if (!(new_state & BOP_EXPOSURE) 
		&& quedExpNumber->getValueInteger () > 0
		&& waitingForNotBop->getValueBool ())
	{
		waitingForNotBop->setValueBool (false);
		quedExpNumber->dec ();
		camStartExposureWithoutCheck ();
	}
}
