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

#define OPT_FLIP    OPT_LOCAL + 401
#define OPT_PLATE   OPT_LOCAL + 402

using namespace rts2camd;

void Camera::initData ()
{
	pixelX = rts2_nan ("f");
	pixelY = rts2_nan ("f");

	nAcc = 1;
}

int Camera::setPlate (const char *arg)
{
	double x, y;
	int ret = sscanf (arg, "%lf:%lf", &x, &y);
	if (ret != 2)
	{
		logStream (MESSAGE_ERROR) << "Cannot parse plate - expected two numbers separated by :" << sendLog;
		return -1;
	}
	setDefaultPlate (x, y);
	return 0;
}

void Camera::setDefaultPlate (double x, double y)
{
	if (xplate == NULL)
		createValue (xplate, "XPLATE", "[arcsec] X plate scale");
	if (yplate == NULL)
	  	createValue (yplate, "YPLATE", "[arcsec] Y plate scale");
	xplate->setValueDouble (x);
	yplate->setValueDouble (y);

	defaultXplate = x;
	defaultYplate = y;
}

void Camera::initCameraChip ()
{
	initData ();
}

void Camera::initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY)
{
	initData ();
	setSize (in_width, in_height, 0, 0);
	pixelX = in_pixelX;
	pixelY = in_pixelY;
}

int Camera::setBinning (int in_vert, int in_hori)
{
	if (xplate)
	{
		xplate->setValueDouble (defaultXplate * in_hori);
		sendValueAll (xplate);
	}
	if (yplate)
	{
		yplate->setValueDouble (defaultYplate * in_vert);
		sendValueAll (yplate);
	}
	return 0;
}

int Camera::box (int in_x, int in_y, int in_width, int in_height)
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

int Camera::center (int in_w, int in_h)
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

long Camera::isExposing ()
{
	double n = getNow ();
	if (n > exposureEnd->getValueDouble ())
	{
		return 0;				 // exposure ended
	}
								 // timeout
	return ((long int) ((exposureEnd->getValueDouble () - n) * USEC_SEC));
}

int Camera::endExposure ()
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

int Camera::stopExposure ()
{
	return endExposure ();
}

int Camera::processData (char *data, size_t size)
{
	return size;
}

int Camera::deleteConnection (Rts2Conn * conn)
{
	if (conn == exposureConn)
	{
		exposureConn = NULL;
	}
	return Rts2Device::deleteConnection (conn);
}

int Camera::endReadout ()
{
	clearReadout ();
	if (quedExpNumber->getValueInteger () > 0 && exposureConn)
	{
		// do not report that we start exposure
		camExpose (exposureConn, getStateChip(0) & CAM_MASK_EXPOSE, true);
	}
	return 0;
}

void Camera::clearReadout ()
{
}

int Camera::sendFirstLine ()
{
	int w, h;
	w = chipUsedReadout->getWidthInt () / binningHorizontal ();
	h = chipUsedReadout->getHeightInt () / binningVertical ();
	focusingHeader.data_type = htons (getDataType ());
	focusingHeader.naxes = htons (2);
	focusingHeader.sizes[0] = htonl (chipUsedReadout->getWidthInt () / binningHorizontal ());
	focusingHeader.sizes[1] = htonl (chipUsedReadout->getHeightInt () / binningVertical ());
	focusingHeader.binnings[0] = htons (binningVertical ());
	focusingHeader.binnings[1] = htons (binningHorizontal ());
	focusingHeader.x = htons (chipUsedReadout->getXInt ());
	focusingHeader.y = htons (chipUsedReadout->getYInt ());
	focusingHeader.filter = htons (getLastFilterNum ());
	// light - dark images
	if (expType)
		focusingHeader.shutter = htons (expType->getValueInteger ());
	else
		focusingHeader.shutter = 0;
	focusingHeader.subexp = subExposure->getValueDouble ();
	focusingHeader.nacc = htons (nAcc);

	sum->setValueDouble (0);
	average->setValueDouble (0);
	max->setValueDouble (-LONG_MAX);
	min->setValueDouble (LONG_MAX);
	computedPix->setValueLong (0);

	// send it out - but do not include it in average etc. calculations
	if (exposureConn && currentImageData >= 0)
		return exposureConn->sendBinaryData (currentImageData, (char *) &focusingHeader, sizeof (imghdr));
	return 0;
}

bool Camera::supportFrameTransfer ()
{
	return false;
}

int Camera::setSubExposure (double in_subexposure)
{
	subExposure->setValueDouble (in_subexposure);
	return 0;
}

Camera::Camera (int in_argc, char **in_argv):Rts2ScriptDevice (in_argc, in_argv, DEVICE_TYPE_CCD, "C0")
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

	xplate = NULL;
	yplate = NULL;

	defaultXplate = rts2_nan ("f");
	defaultYplate = rts2_nan ("f");

	currentImageData = -1;

	createValue (calculateStatistics, "calculate_stat", "if statistics values should be calculated", false);
	calculateStatistics->addSelVal ("yes");
	calculateStatistics->addSelVal ("only statistics");
	calculateStatistics->addSelVal ("no");
	calculateStatistics->setValueInteger (STATISTIC_YES);

	createValue (average, "average", "image average", false);
	createValue (max, "max", "maximum pixel value", false);
	createValue (min, "min", "minimal pixel value", false);
	createValue (sum, "sum", "sum of pixels readed out", false);

	createValue (computedPix, "computed", "number of pixels so far computed", false);

	createValue (quedExpNumber, "que_exp_num", "number of exposures in que", false, 0, 0, true);
	quedExpNumber->setValueInteger (0);

	createValue (exposureNumber, "exposure_num", "number of exposures camera takes", false, 0, 0, false);
	exposureNumber->setValueLong (0);

	createValue (scriptExposureNum, "script_exp_num", "number of images taken in script", false, 0, 0, false);
	scriptExposureNum->setValueLong (0);

	createValue (exposureEnd, "exposure_end", "expected end of exposure", false);

	createValue (waitingForEmptyQue, "wait_for_que", "if camera is waiting for empty que", false);
	waitingForEmptyQue->setValueBool (false);

	createValue (waitingForNotBop, "wait_for_notbop", "if camera is waiting for not bop state", false);
	waitingForNotBop->setValueBool (false);

	createValue (chipSize, "SIZE", "chip size", true, RTS2_VALUE_INTEGER);
	createValue (chipUsedReadout, "WINDOW", "used chip subframe", true, RTS2_VALUE_INTEGER, CAM_WORKING, true);

	createValue (binning, "binning", "chip binning", true, 0, CAM_WORKING, true);
	createValue (dataType, "data_type", "used data type", false, 0, CAM_WORKING, true);

	createValue (exposure, "exposure", "current exposure time", false, 0, CAM_WORKING);
	exposure->setValueDouble (1);

	createValue (flip, "FLIP", "camera flip (since most astrometry devices works as mirrors", true);
	setDefaultFlip (1);

	sendOkInExposure = false;

	createValue (subExposure, "subexposure", "current subexposure", false, 0, CAM_WORKING, true);
	createValue (camFilterVal, "filter", "used filter number", false, 0, CAM_EXPOSING);

	createValue (camFocVal, "focpos", "position of focuser", false, 0, CAM_EXPOSING);

	createValue (rotang, "CCD_ROTA", "CCD rotang", true, RTS2_DT_ROTANG);
	rotang->setValueDouble (0);

	focuserDevice = NULL;
	wheelDevice = NULL;

	dataBuffer = NULL;
	dataBufferSize = -1;

	exposureConn = NULL;

	createValue (rnoise, "RNOISE", "CCD readout noise");

	// other options..
	addOption ('F', NULL, 1, "name of focuser device, which will be granted to do exposures without priority");
	addOption ('W', NULL, 1, "name of device which is used as filter wheel");
	addOption ('e', NULL, 1, "default exposure");
	addOption ('s', "subexposure", 1, "default subexposure");
	addOption ('t', "type", 1, "specify camera type (in case camera do not store it in FLASH ROM)");
	addOption ('r', NULL, 1, "camera rotang");
	addOption (OPT_FLIP, "flip", 1, "camera flip (default to 1)");
	addOption (OPT_PLATE, "plate", 1, "camera plate scale, x:y");
}

Camera::~Camera ()
{
	delete[] dataBuffer;
	delete filter;
}

int Camera::willConnect (Rts2Address * in_addr)
{
	if (wheelDevice && in_addr->getType () == DEVICE_TYPE_FW
		&& in_addr->isAddress (wheelDevice))
		return 1;
	if (focuserDevice && in_addr->getType () == DEVICE_TYPE_FOCUS
		&& in_addr->isAddress (focuserDevice))
		return 1;
	return Rts2ScriptDevice::willConnect (in_addr);
}

Rts2DevClient *Camera::createOtherType (Rts2Conn * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_FW:
			return new ClientFilterCamera (conn);
		case DEVICE_TYPE_FOCUS:
			return new ClientFocusCamera (conn);
	}
	return Rts2ScriptDevice::createOtherType (conn, other_device_type);
}

void Camera::checkQueChanges (int fakeState)
{
	// do not check if we have qued exposures
	if (quedExpNumber->getValueInteger () > 0)
		return;
	Rts2ScriptDevice::checkQueChanges (fakeState);
	if (queValues.empty ())
	{
		if (waitingForEmptyQue->getValueBool () == true)
		{
			waitingForEmptyQue->setValueBool (false);
			sendOkInExposure = true;
			if (exposureConn)
			{
				quedExpNumber->inc ();
				waitingForNotBop->setValueBool (true);
				connections_t::iterator iter;
				// ask all centralds for possible blocking devices
				for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
				{
					(*iter)->queCommand (new Rts2CommandDeviceStatusInfo (this, exposureConn));
				}
			}
		}
	}
}

int Camera::killAll ()
{
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
	
	if (isExposing ())
		stopExposure ();
	
	return Rts2ScriptDevice::killAll ();
}

int Camera::scriptEnds ()
{
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);

	scriptExposureNum->setValueLong (0);
	sendValueAll (scriptExposureNum);

	calculateStatistics->setValueInteger (0);
	sendValueAll (calculateStatistics);

	return Rts2ScriptDevice::scriptEnds ();
}

int Camera::info ()
{
	camFilterVal->setValueInteger (getFilterNum ());
	camFocVal->setValueInteger (getFocPos ());
	return Rts2ScriptDevice::info ();
}

int Camera::processOption (int in_opt)
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
			exposure->setValueCharArr (optarg);
			break;
		case 's':
			setSubExposure (atof (optarg));
			break;
		case 't':
			ccdRealType = optarg;
			break;
		case 'c':
			if (nightCoolTemp == NULL)
				return Rts2ScriptDevice::processOption (in_opt);
			nightCoolTemp->setValueCharArr (optarg);
			break;
		case 'r':
			rotang->setValueCharArr (optarg);
			break;
		case OPT_FLIP:
			flip->setValueCharArr (optarg);
			break;
		case OPT_PLATE:
			return setPlate (optarg);
		default:
			return Rts2ScriptDevice::processOption (in_opt);
	}
	return 0;
}

int Camera::initChips ()
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

int Camera::sendImage (char *data, size_t dataSize)
{
	if (!exposureConn)
		return -1;
	if (calculateStatistics->getValueInteger () != STATISTIC_ONLY)
	{
		currentImageData = exposureConn->startBinaryData (dataSize + sizeof (imghdr), dataType->getValueInteger ());
		if (currentImageData == -1)
			return -1;
	}
	sendFirstLine ();
	return sendReadoutData (data, dataSize);
}

int Camera::sendReadoutData (char *data, size_t dataSize)
{
	// calculated..
	if (calculateStatistics->getValueInteger () != STATISTIC_NO)
	{
		int totPix;
		// update sum. min and max
		switch (getDataType ())
		{
			case RTS2_DATA_BYTE:
				totPix = updateStatistics ((int8_t *) data, dataSize);
				break;
			case RTS2_DATA_SHORT:
				totPix = updateStatistics ((int16_t *) data, dataSize);
				break;
			case RTS2_DATA_LONG:
				totPix = updateStatistics ((int16_t *) data, dataSize);
				break;
			case RTS2_DATA_LONGLONG:
				totPix = updateStatistics ((int64_t *) data, dataSize);
				break;
			case RTS2_DATA_FLOAT:
				totPix = updateStatistics ((float *) data, dataSize);
				break;
			case RTS2_DATA_DOUBLE:
				totPix = updateStatistics ((double *) data, dataSize);
				break;
			case RTS2_DATA_SBYTE:
				totPix = updateStatistics ((uint8_t *) data, dataSize);
				break;
			case RTS2_DATA_USHORT:
				totPix = updateStatistics ((uint16_t *) data, dataSize);
				break;
			case RTS2_DATA_ULONG:
				totPix = updateStatistics ((uint32_t *) data, dataSize);
				break;
		}
		computedPix->setValueLong (computedPix->getValueLong () + totPix);
		average->setValueDouble (sum->getValueDouble () / computedPix->getValueLong ());

		sendValueAll (average);
		sendValueAll (max);
		sendValueAll (min);
		sendValueAll (sum);
		sendValueAll (computedPix);
	}
	if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
		calculateDataSize -= dataSize;

	if (exposureConn && currentImageData >= 0)
		return exposureConn->sendBinaryData (currentImageData, data, dataSize);
	return 0;
}

void Camera::addBinning2D (int bin_v, int bin_h)
{
	Binning2D *bin = new Binning2D (bin_v, bin_h);
	binning->addSelVal (bin->getDescription ().c_str (), bin);
}

void Camera::initBinnings ()
{
	addBinning2D (1,1);
}

void Camera::addDataType (int in_type)
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

void Camera::initDataTypes ()
{
	addDataType (RTS2_DATA_USHORT);
}

int Camera::initValues ()
{
	// TODO init focuser - try to read focuser offsets & initial position from file
	addConstValue ("focuser", focuserDevice);
	addConstValue ("wheel", wheelDevice);

	addConstValue ("CCD_TYPE", "camera type", ccdRealType);
	addConstValue ("CCD_SER", "camera serial number", serialNumber);

	addConstValue ("chips", 1);

	initBinnings ();
	initDataTypes ();

	defaultFlip = flip->getValueInteger ();

	return Rts2ScriptDevice::initValues ();
}

void Camera::checkExposures ()
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
			logStream (MESSAGE_DEBUG) << "camWaitExpose return " << ret << " quedExpNumber " << quedExpNumber->getValueInteger () << sendLog;
			switch (ret)
			{
				case -3:
					exposureConn = NULL;
					endExposure ();
					break;
				case -2:
					// remember exposure number
					expNum = getExposureNumber ();
					endExposure ();
					// if new exposure does not start during endExposure (camReadout) call, drop exposure state
					if (expNum == getExposureNumber ())
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

void Camera::checkReadouts ()
{
	int ret;
	if ((getStateChip (0) & CAM_MASK_READING) != CAM_READING)
		return;
	ret = doReadout ();
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

void Camera::afterReadout ()
{
	setTimeout (USEC_SEC);
}

int Camera::setValue (Rts2Value * old_value, Rts2Value * new_value)
{
	if (old_value == exposure
		|| old_value == quedExpNumber
		|| old_value == expType
		|| old_value == rotang
		|| old_value == nightCoolTemp
		|| old_value == binning
		|| old_value == calculateStatistics)
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
	if (old_value == chipUsedReadout)
	{
		Rts2ValueRectangle *rect = (Rts2ValueRectangle *) new_value;
		return box (rect->getXInt (), rect->getYInt (), rect->getWidthInt (), rect->getHeightInt ()) == 0 ? 0 : -2;
	}
	if (old_value == xplate)
	{
		setDefaultPlate (new_value->getValueDouble (), yplate->getValueDouble ());
		return 0;
	}
	if (old_value == yplate)
	{
		setDefaultPlate (xplate->getValueDouble (), new_value->getValueDouble ());
		return 0;
	}
	return Rts2ScriptDevice::setValue (old_value, new_value);
}

void Camera::valueChanged (Rts2Value *changed_value)
{
	if (changed_value == binning)
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		setBinning (bin->horizontal, bin->vertical);
	}
}

void Camera::deviceReady (Rts2Conn * conn)
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

void Camera::postEvent (Rts2Event * event)
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

int Camera::idle ()
{
	checkExposures ();
	checkReadouts ();
	return Rts2ScriptDevice::idle ();
}

int Camera::changeMasterState (int new_state)
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

int Camera::camStartExposure ()
{
	// check if we aren't blocked
	if ((!expType || expType->getValueInteger () == 0)
		&& (getDeviceBopState () & BOP_EXPOSURE))
	{
		if (waitingForNotBop->getValueBool () == false)
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

int Camera::camStartExposureWithoutCheck ()
{
	int ret;

	incExposureNumber ();

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

	// check if that comes from old request
	if (sendOkInExposure && exposureConn)
	{
		sendOkInExposure = false;
		exposureConn->sendCommandEnd (DEVDEM_OK, "Executing exposure from queue");
	}

	logStream (MESSAGE_INFO) << "exposing for '"
		<< (exposureConn ? exposureConn->getName () : "null") << "'" << sendLog;

	return 0;
}

int Camera::camExpose (Rts2Conn * conn, int chipState, bool fromQue)
{
	int ret;

	// if it is currently exposing
	// or performing other op that can block command execution
	// or there are qued values which needs to be dealed before we can start exposing
	if ((chipState & CAM_EXPOSING)
		|| ((chipState & CAM_READING) && !supportFrameTransfer ())
		|| (!queValues.empty () && fromQue == false)
		)
	{
		if (fromQue == false)
		{
			if (queValues.empty ())
			{
				quedExpNumber->inc ();
				sendValueAll (quedExpNumber);
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

	exposureConn = conn;

	ret = camStartExposure ();
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot exposure on chip");
	}
	else
	{
	}
	return ret;
}

long Camera::camWaitExpose ()
{
	int ret;
	ret = isExposing ();
	return (ret == 0 ? -2 : ret);
}

int Camera::camBox (Rts2Conn * conn, int x, int y, int width, int height)
{
	int ret;
	ret = box (x, y, width, height);
	if (!ret)
		return ret;
	conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "invalid box size");
	return ret;
}

int Camera::camCenter (Rts2Conn * conn, int in_h, int in_w)
{
	int ret;
	ret = center (in_h, in_w);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "cannot set box size");
	return ret;
}

int Camera::readoutStart ()
{
	return sendFirstLine ();
}

int Camera::camReadout (Rts2Conn * conn)
{
	// if we can do exposure, do it..
	if (quedExpNumber->getValueInteger () > 0 && exposureConn && supportFrameTransfer ())
	{
		checkQueChanges (getStateChip (0) & ~CAM_EXPOSING);
		maskStateChip (0, CAM_MASK_READING | CAM_MASK_FT, CAM_READING | CAM_FT,
			0, 0, "starting frame transfer");
		if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
			currentImageData = -1;
		else
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
		if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
			currentImageData = -1;
		else
			currentImageData = conn->startBinaryData (chipByteSize () + sizeof (imghdr), dataType->getValueInteger ());
	}

	if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
		calculateDataSize = chipByteSize ();

	if (currentImageData >= 0 || calculateStatistics->getValueInteger () == STATISTIC_ONLY)
	{
		return readoutStart ();
	}
	maskStateChip (0, DEVICE_ERROR_MASK | CAM_MASK_READING,
		DEVICE_ERROR_HW | CAM_NOTREADING, 0, 0,
		"chip readout failed");
	conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
	return -1;
}

int Camera::camStopRead (Rts2Conn * conn)
{
	int ret;
	ret = camStopRead ();
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot end readout");
	return ret;
}

int Camera::camFilter (int new_filter)
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

int Camera::getStateChip (int chip)
{
	return (getState () & (CAM_MASK_CHIP << (chip * 4))) >> (0 * 4);
}

void Camera::maskStateChip (int chip_num, int chip_state_mask, int chip_new_state, int state_mask, int new_state, const char *description)
{
	maskState (state_mask | (chip_state_mask << (4 * chip_num)),
		new_state | (chip_new_state << (4 * chip_num)), description);
}

int Camera::getFilterNum ()
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

int Camera::setFocuser (int new_set)
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

int Camera::stepFocuser (int step_count)
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

int Camera::getFocPos ()
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

bool Camera::isIdle ()
{
	return ((getStateChip (0) &
		(CAM_MASK_EXPOSE | CAM_MASK_READING)) ==
		(CAM_NOEXPOSURE | CAM_NOTREADING));
}

int Camera::commandAuthorized (Rts2Conn * conn)
{
	if (conn->isCommand ("help"))
	{
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
	else if (conn->isCommand ("expose"))
	{
		if (!conn->paramEnd ())
			return -2;
		return camExpose (conn, getStateChip (0), false);
	}
	else if (conn->isCommand ("stopexpo"))
	{
		if (!conn->paramEnd ())
			return -2;
		return stopExposure ();
	}
	else if (conn->isCommand ("box"))
	{
		int x, y, w, h;
		if (conn->paramNextInteger (&x)
			|| conn->paramNextInteger (&y)
			|| conn->paramNextInteger (&w) || conn->paramNextInteger (&h)
			|| !conn->paramEnd ())
			return -2;
		return camBox (conn, x, y, w, h);
	}
	else if (conn->isCommand ("center"))
	{
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
		if (!conn->paramEnd ())
			return -2;
		return camStopRead (conn);
	}
	return Rts2ScriptDevice::commandAuthorized (conn);
}

int Camera::maskQueValueBopState (int new_state, int valueQueCondition)
{
	if (valueQueCondition & CAM_EXPOSING)
		new_state |= BOP_EXPOSURE;
	if (valueQueCondition & CAM_READING)
		new_state |= BOP_READOUT;
	return new_state;
}

void Camera::setFullBopState (int new_state)
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
