/* 
 * Basic camera daemon
 * Copyright (C) 2001-2010 Petr Kubanek <petr@kubanek.net>
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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <iomanip>

#include "camd.h"
#include "cliwheel.h"
#include "clifocuser.h"
#include "timestamp.h"

#define OPT_FLIP              OPT_LOCAL + 401
#define OPT_PLATE             OPT_LOCAL + 402
#define OPT_FOCUS             OPT_LOCAL + 403
#define OPT_WHEEL             OPT_LOCAL + 404
#define OPT_WITHSHM           OPT_LOCAL + 405
#define OPT_FILTER_OFFSETS    OPT_LOCAL + 406

#define EVENT_TEMP_CHECK      RTS2_LOCAL_EVENT + 676

using namespace rts2camd;

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
		createValue (xplate, "XPLATE", "[arcsec] X plate scale", true, RTS2_VALUE_WRITABLE);
	if (yplate == NULL)
	  	createValue (yplate, "YPLATE", "[arcsec] Y plate scale", true, RTS2_VALUE_WRITABLE);
	xplate->setValueDouble (x);
	yplate->setValueDouble (y);

	defaultXplate = x;
	defaultYplate = y;
}

void Camera::initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY)
{
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
	binningX->setValueInteger (((Binning2D *)(binning->getData ()))->horizontal);
	binningY->setValueInteger (((Binning2D *)(binning->getData ()))->vertical);
	sendValueAll (binningX);
	sendValueAll (binningY);
	return 0;
}

int Camera::box (int _x, int _y, int _width, int _height, rts2core::ValueRectangle *retv)
{
	// tests for -1 -> full size
	if (_x == -1)
		_x = chipSize->getXInt ();
	if (_y == -1)
		_y = chipSize->getYInt ();
	if (_width == -1)
		_width = chipSize->getWidthInt ();
	if (_height == -1)
		_height = chipSize->getHeightInt ();
	if (((_x - chipSize->getXInt ()) + _width) > chipSize->getWidthInt ()
		|| ((_y - chipSize->getYInt ()) + _height) > chipSize->getHeightInt ())
		return -1;
	chipUsedReadout->setInts (_x, _y, _width, _height);
	if (retv)
		retv->setInts (_x, _y, _width, _height);
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
	if (n >= exposureEnd->getValueDouble ())
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
		logStream (MESSAGE_INFO) << "end exposure for " << exposureConn->getName () << sendLog;
		return camReadout (exposureConn);
	}
	if (getStateChip (0) & CAM_EXPOSING)
	{
		stopExposure ();
		logStream (MESSAGE_WARNING) << "end exposure without exposure connection" << sendLog;
	}

	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
	maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | CAM_MASK_FT | BOP_TEL_MOVE | BOP_WILL_EXPOSE, CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT, "chip exposure interrupted", NAN, NAN, exposureConn);
	return 0;
}

int Camera::stopExposure ()
{
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
	maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | CAM_MASK_FT | BOP_TEL_MOVE | BOP_WILL_EXPOSE, CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT, "exposure interrupted", NAN, NAN, exposureConn);
	return 0;
}

int Camera::processData (char *data, size_t size)
{
	return size;
}

int Camera::deleteConnection (rts2core::Connection * conn)
{
	if (conn == exposureConn)
	{
		exposureConn = NULL;
	}
	return rts2core::ScriptDevice::deleteConnection (conn);
}

int Camera::endReadout ()
{
	TimeDiff td (timeReadoutStart, getNow ());
	pixelsSecond->setValueDouble (readoutPixels / td.getTimeDiff ());
	sendValueAll (pixelsSecond);

	logStream (MESSAGE_INFO) << "readout " <<  readoutPixels << " pixels in " << td
		<< " (" << std::setiosflags (std::ios_base::fixed) << pixelsSecond->getValueFloat () << " pixels per second)" << sendLog;

	clearReadout ();
	if (currentImageData == -2 && exposureConn)
	{
		exposureConn->endSharedData (sharedMemId);
	}
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

int Camera::getPhysicalChannel (int ch)
{
	if (channels == NULL)
		return ch;
	size_t j = 0;
	for (int i = 0; i < ch && j < channels->size(); j++)
	{
		if ((*channels)[j])
			i++;
	}
	return j;
}

void Camera::startImageData (rts2core::Connection * conn)
{
	if (sharedMemId >= 0)
	{
		currentImageData = -2;
		conn->startSharedData (sharedMemId);
		exposureConn = conn;
	}
	else
	{
		int chnTot = dataChannels ? dataChannels->getValueInteger () : 1;
		long chansize[chnTot];
		for (int i = 0; i < chnTot; i++)
			chansize[i] = chipByteSize () + sizeof (imghdr);
		currentImageData = conn->startBinaryData (dataType->getValueInteger (), chnTot, chansize);
		exposureConn = conn;
	}
}

int Camera::sendFirstLine (int chan, int pchan)
{
	//int w, h;
	// w = chipUsedReadout->getWidthInt () / binningHorizontal ();
	// h = chipUsedReadout->getHeightInt () / binningVertical ();
	focusingHeader->data_type = htons (getDataType ());
	focusingHeader->naxes = htons (2);
	focusingHeader->sizes[0] = htonl (chipUsedReadout->getWidthInt () / binningHorizontal ());
	focusingHeader->sizes[1] = htonl (chipUsedReadout->getHeightInt () / binningVertical ());
	focusingHeader->binnings[0] = htons (binningVertical ());
	focusingHeader->binnings[1] = htons (binningHorizontal ());
	focusingHeader->x = htons (chipUsedReadout->getXInt ());
	focusingHeader->y = htons (chipUsedReadout->getYInt ());
	focusingHeader->filter = htons (getLastFilterNum ());
	// light - dark images
	if (expType)
		focusingHeader->shutter = htons (expType->getValueInteger ());
	else
		focusingHeader->shutter = 0;

	focusingHeader->channel = htons (pchan);

	sum->setValueDouble (0);
	average->setValueDouble (0);
	max->setValueDouble (-LONG_MAX);
	min->setValueDouble (LONG_MAX);
	computedPix->setValueLong (0);

	if (shmBuffer != NULL)
		*((unsigned long *) shmBuffer) = 0;
	// send it out - but do not include it in average etc. calculations
	if (exposureConn && currentImageData >= 0)
		return exposureConn->sendBinaryData (currentImageData, chan, (char *) focusingHeader, sizeof (imghdr));
	return 0;
}

bool Camera::supportFrameTransfer ()
{
	return false;
}

Camera::Camera (int in_argc, char **in_argv):rts2core::ScriptDevice (in_argc, in_argv, DEVICE_TYPE_CCD, "C0")
{
	expType = NULL;

	dataChannels = NULL;
	channels = NULL;

	tempAir = NULL;
	tempCCD = NULL;
	tempCCDHistory = NULL;
	tempCCDHistoryInterval = NULL;
	tempCCDHistorySize = NULL;
	tempSet = NULL;
	nightCoolTemp = NULL;
	ccdType[0] = '\0';
	ccdRealType = ccdType;
	serialNumber[0] = '\0';

	timeReadoutStart = NAN;

	pixelX = NAN;
	pixelY = NAN;

	nAcc = 1;

	shmBuffer = NULL;
	sharedMemId = -2;

	xplate = NULL;
	yplate = NULL;

	defaultXplate = NAN;
	defaultYplate = NAN;

	currentImageData = -1;

	createValue (calculateStatistics, "calculate_stat", "if statistics values should be calculated", false, RTS2_VALUE_WRITABLE);
	calculateStatistics->addSelVal ("yes");
	calculateStatistics->addSelVal ("only statistics");
	calculateStatistics->addSelVal ("no");
	calculateStatistics->setValueInteger (STATISTIC_YES);

	createValue (average, "average", "image average", false);
	createValue (max, "max", "maximum pixel value", false);
	createValue (min, "min", "minimal pixel value", false);
	createValue (sum, "sum", "sum of pixels readed out", false);

	createValue (computedPix, "computed", "number of pixels so far computed", false);

	createValue (calculateCenter, "center_cal", "calculate center box statistics", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	calculateCenter->setValueBool (false);

	createValue (centerBox, "center_box", "calculate center box coordinates", false, RTS2_VALUE_INTEGER | RTS2_VALUE_WRITABLE);
	centerBox->setInts (-1, -1, -1, -1);

	createValue (centerCutLevel, "center_cut_level", "[ADU] lower limit for ADU value to be considered for pixels", false, RTS2_VALUE_WRITABLE);
	centerCutLevel->setValueDouble (0);

	createValue (sumsX, "sums_Xs", "sums along X axis", false, RTS2_VALUE_DOUBLE);
	createValue (sumsY, "sums_Ys", "sums along Y axis", false, RTS2_VALUE_DOUBLE);

	createValue (centerX, "center_X", "center pixel in X", false);
	createValue (centerY, "center_Y", "center pixel in Y", false);

	createValue (quedExpNumber, "que_exp_num", "number of exposures in que", false, RTS2_VALUE_WRITABLE, 0);
	quedExpNumber->setValueInteger (0);

	createValue (exposureNumber, "exposure_num", "number of exposures camera takes", false, 0, 0);
	exposureNumber->setValueLong (0);

	createValue (scriptExposureNum, "script_exp_num", "number of images taken in script", false, 0, 0);
	scriptExposureNum->setValueLong (0);

	createValue (exposureEnd, "exposure_end", "expected end of exposure", false);

	createValue (waitingForEmptyQue, "wait_for_que", "if camera is waiting for empty que", false);
	waitingForEmptyQue->setValueBool (false);

	createValue (waitingForNotBop, "wait_for_notbop", "if camera is waiting for not bop state", false);
	waitingForNotBop->setValueBool (false);

	createValue (chipSize, "SIZE", "chip size", true, RTS2_VALUE_INTEGER);
	createValue (chipUsedReadout, "WINDOW", "used chip subframe", true, RTS2_VALUE_INTEGER | RTS2_VALUE_WRITABLE, CAM_WORKING);

	createValue (binning, "binning", "[pixelX x pixelY] chip binning", true, RTS2_VALUE_WRITABLE, CAM_WORKING);
	createValue (binningX, "BINX", "[pixels] binning along X axis", true, 0, CAM_WORKING);
	createValue (binningY, "BINY", "[pixels] binning along X axis", true, 0, CAM_WORKING);

	binningX->setValueInteger (-1);
	binningY->setValueInteger (-1);

	createValue (dataType, "data_type", "used data type", false, 0, CAM_WORKING);

	createValue (exposure, "exposure", "current exposure time", false, RTS2_VALUE_WRITABLE, CAM_WORKING);
	exposure->setValueDouble (1);

	createValue (flip, "FLIP", "camera flip (since most astrometry devices works as mirrors", true);
	setDefaultFlip (1);

	sendOkInExposure = false;

	createValue (pixelsSecond, "pixels_second", "[pixels/second] average readout speed", false, RTS2_DT_KMG);

	createValue (camFocVal, "focpos", "position of focuser", false, RTS2_VALUE_WRITABLE, CAM_EXPOSING);

	createValue (rotang, "CCD_ROTA", "CCD rotang", true, RTS2_DT_ROTANG | RTS2_VALUE_WRITABLE);
	rotang->setValueDouble (0);

        camFilterVal = NULL;
	camFilterOffsets = NULL;
	focuserDevice = NULL;

	dataBuffer = NULL;
	dataBufferSize = -1;

	exposureConn = NULL;

	createValue (filterMoving, "fw_moving", "number of moving filter wheel(s)", false);
	filterMoving->setValueInteger (0);

	createValue (focuserMoving, "foc_moving", "if focuser is moving", false);
	focuserMoving->setValueBool (false);

	// other options..
	addOption (OPT_FOCUS, "focdev", 1, "name of focuser device, which will be granted to do exposures without priority");
	addOption (OPT_WHEEL, "wheeldev", 1, "name of device which is used as filter wheel");
	addOption (OPT_FILTER_OFFSETS, "filter-offsets", 1, "camera filter offsets, separated with :");
	addOption ('e', NULL, 1, "default exposure");
	addOption ('t', "type", 1, "specify camera type (in case camera do not store it in FLASH ROM)");
	addOption ('r', NULL, 1, "camera rotang");
	addOption (OPT_FLIP, "flip", 1, "camera flip (default to 1)");
	addOption (OPT_PLATE, "plate", 1, "camera plate scale, x:y");
	addOption (OPT_WITHSHM, "with-shm", 0, "use shared memory to speed up communication (experimental)");
}

Camera::~Camera ()
{
	if (sharedMemId >= 0)
	{
		shmdt (dataBuffer);
	}
	else
	{
		delete[] dataBuffer;
		free (focusingHeader);
	}
}

int Camera::willConnect (rts2core::NetworkAddress * in_addr)
{
	if (in_addr->getType () == DEVICE_TYPE_FW)
	{
		for (std::vector <const char *>::iterator iter = wheelDevices.begin (); iter != wheelDevices.end (); iter++)
		{
			if (in_addr->isAddress (*iter))
				return 1;
		}
	}
	if (focuserDevice && in_addr->getType () == DEVICE_TYPE_FOCUS && in_addr->isAddress (focuserDevice))
		return 1;
	return rts2core::ScriptDevice::willConnect (in_addr);
}

rts2core::DevClient *Camera::createOtherType (rts2core::Connection * conn, int other_device_type)
{
	switch (other_device_type)
	{
		case DEVICE_TYPE_FW:
			return new ClientFilterCamera (conn);
		case DEVICE_TYPE_FOCUS:
			return new ClientFocusCamera (conn);
	}
	return rts2core::ScriptDevice::createOtherType (conn, other_device_type);
}

void Camera::checkQueChanges (int fakeState)
{
	// do not check if we have queued exposures
	if (quedExpNumber->getValueInteger () > 0)
		return;
	rts2core::ScriptDevice::checkQueChanges (fakeState);
	if (queValues.empty ())
	{
		if (waitingForEmptyQue->getValueBool () == true)
		{
			waitingForEmptyQue->setValueBool (false);
			sendValueAll (waitingForEmptyQue);
			sendOkInExposure = true;
			if (exposureConn)
			{
				quedExpNumber->inc ();
				sendValueAll (quedExpNumber);
				waitingForNotBop->setValueBool (true);
				sendValueAll (waitingForNotBop);
				rts2core::connections_t::iterator iter;
				// ask all centralds for possible blocking devices
				for (iter = getCentraldConns ()->begin (); iter != getCentraldConns ()->end (); iter++)
				{
					(*iter)->queCommand (new rts2core::CommandDeviceStatusInfo (this, exposureConn));
				}
			}
		}
	}
}

void Camera::checkQueuedExposures ()
{
	if (quedExpNumber->getValueInteger () > 0)
	{
		quedExpNumber->dec ();
		camExpose (exposureConn, getStateChip (0), false);
	}
	infoAll ();
}

int Camera::killAll (bool callScriptEnds)
{
	waitingForNotBop->setValueBool (false);
	waitingForEmptyQue->setValueBool (false);
	filterMoving->setValueInteger (0);

	if (isExposing ())
		stopExposure ();
	else
	// call Camera::stopExposure tp clear states and queued exposure values
		Camera::stopExposure ();

	sendValueAll (waitingForNotBop);
	sendValueAll (waitingForEmptyQue);
	sendValueAll (filterMoving);

	return rts2core::ScriptDevice::killAll (callScriptEnds);
}

int Camera::scriptEnds ()
{
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);

	scriptExposureNum->setValueLong (0);
	sendValueAll (scriptExposureNum);

	calculateStatistics->setValueInteger (STATISTIC_YES);
	sendValueAll (calculateStatistics);

	box (-1, -1, -1, -1);
	sendValueAll (chipUsedReadout);

	// set back default binning
	binning->setValueInteger (0);
	if ((getState () & DEVICE_STATUS_MASK) != DEVICE_IDLE)
	{
		setNeedReload ();
	}
	else
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		setBinning (bin->horizontal, bin->vertical);
	}
	sendValueAll (binning);

	dataType->setValueInteger (0);
	sendValueAll (dataType);

	return rts2core::ScriptDevice::scriptEnds ();
}

int Camera::info ()
{
        if (camFilterVal)
        	camFilterVal->setValueInteger (getFilterNum ());

	std::vector <rts2core::ValueSelection *>::iterator viter;
	std::vector <const char *>::iterator niter;
	for (viter = camFilterVals.begin (), niter = wheelDevices.begin (); viter != camFilterVals.end (); viter++, niter++)
	{
		(*viter)->setValueInteger (getFilterNum (*niter));
	}
	camFocVal->setValueInteger (getFocPos ());
	return rts2core::ScriptDevice::info ();
}

int Camera::processOption (int in_opt)
{
	switch (in_opt)
	{
		case OPT_FOCUS:
			focuserDevice = optarg;
			break;
		case OPT_WHEEL:
			wheelDevices.push_back (optarg);
			break;
		case OPT_FILTER_OFFSETS:
			createFilter ();
			setFilterOffsets (optarg);
			break;
		case 'e':
			exposure->setValueCharArr (optarg);
			break;
		case 't':
			ccdRealType = optarg;
			break;
		case 'c':
			if (nightCoolTemp == NULL)
				return rts2core::ScriptDevice::processOption (in_opt);
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
		case OPT_WITHSHM:
			sharedMemId = -1;
			break;
		default:
			return rts2core::ScriptDevice::processOption (in_opt);
	}
	return 0;
}

void Camera::usage ()
{
	std::cout << "To sepcify multiple filter wheels, add multiple --wheeldev options." << std::endl
		<< "  " << getAppName () << " --wheldev W0 --wheeldev W1" << std::endl;
}

int Camera::initChips ()
{
	return 0;
}

int Camera::sendImage (char *data, size_t dataSize)
{
	if (!exposureConn)
		return -1;
	if (calculateStatistics->getValueInteger () != STATISTIC_ONLY)
	{
		startImageData (exposureConn);
		if (currentImageData == -1)
			return -1;
	}
	sendFirstLine (0, 0);
	return sendReadoutData (data, dataSize);
}

int Camera::sendReadoutData (char *data, size_t dataSize, int chan)
{
	// calculated..
	if (calculateStatistics->getValueInteger () != STATISTIC_NO)
	{
		int totPix;
		// update sum. min and max
		switch (getDataType ())
		{
			case RTS2_DATA_BYTE:
				totPix = updateStatistics ((uint8_t *) data, dataSize);
				break;
			case RTS2_DATA_SHORT:
				totPix = updateStatistics ((int16_t *) data, dataSize);
				break;
			case RTS2_DATA_LONG:
				totPix = updateStatistics ((int32_t *) data, dataSize);
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
				totPix = updateStatistics ((int8_t *) data, dataSize);
				break;
			case RTS2_DATA_USHORT:
				totPix = updateStatistics ((uint16_t *) data, dataSize);
				break;
			case RTS2_DATA_ULONG:
				totPix = updateStatistics ((uint32_t *) data, dataSize);
				break;
		}
		computedPix->setValueLong (computedPix->getValueLong () + totPix);
		pixelsSecond->setValueDouble (computedPix->getValueLong () / (getNow () - timeReadoutStart));
		average->setValueDouble (sum->getValueDouble () / computedPix->getValueLong ());

		sendValueAll (average);
		sendValueAll (max);
		sendValueAll (min);
		sendValueAll (sum);
		sendValueAll (computedPix);
		sendValueAll (pixelsSecond);
	}
	if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
		calculateDataSize -= dataSize;

	if (calculateCenter->getValueBool ())
	{
		switch (getDataType ())
		{
			case RTS2_DATA_BYTE:
				updateCenter ((uint8_t *) data, dataSize);
				break;
			case RTS2_DATA_SHORT:
				updateCenter ((int16_t *) data, dataSize);
				break;
			case RTS2_DATA_LONG:
				updateCenter ((int32_t *) data, dataSize);
				break;
			case RTS2_DATA_LONGLONG:
				updateCenter ((int64_t *) data, dataSize);
				break;
			case RTS2_DATA_FLOAT:
				updateCenter ((float *) data, dataSize);
				break;
			case RTS2_DATA_DOUBLE:
				updateCenter ((double *) data, dataSize);
				break;
			case RTS2_DATA_SBYTE:
				updateCenter ((int8_t *) data, dataSize);
				break;
			case RTS2_DATA_USHORT:
				updateCenter ((uint16_t *) data, dataSize);
				break;
			case RTS2_DATA_ULONG:
				updateCenter ((uint32_t *) data, dataSize);
				break;
		}
	}

	if (shmBuffer != NULL)
		*((unsigned long *) shmBuffer) += dataSize;

	if (exposureConn && currentImageData >= 0)
		return exposureConn->sendBinaryData (currentImageData, chan, data, dataSize);
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
	binningX->setValueInteger (1);
	binningY->setValueInteger (1);
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
	if (focuserDevice)
	{
		addConstValue ("focuser", focuserDevice);
	}

	if (wheelDevices.size () > 0)
	{
		addConstValue ("wheel", wheelDevices.front ());

		char fil = 'A';
		std::vector <const char *>::iterator iter;
		for (iter = wheelDevices.begin (); iter != wheelDevices.end (); iter++, fil++)
		{
			addConstValue ((std::string ("wheel") + fil).c_str (), *iter);
		}

                createFilter ();

		fil = 'A';
		for (iter = wheelDevices.begin (); iter != wheelDevices.end (); iter++, fil++)
		{
			rts2core::ValueSelection *filvals;
			rts2core::DoubleArray *offsetsval;
			createValue (filvals, (std::string ("FILT") + fil).c_str (), "used filter number", (wheelDevices.size () == 1) ? false : true, RTS2_VALUE_WRITABLE, CAM_EXPOSING);
			createValue (offsetsval, (std::string ("filter_offsets_") + fil).c_str (), "filter offsets", false, RTS2_VALUE_WRITABLE);

			camFilterVals.push_back (filvals);
			camFiltersOffsets.push_back (offsetsval);
		}
	}

	addConstValue ("CCD_TYPE", "camera type", ccdRealType);
	addConstValue ("CCD_SER", "camera serial number", serialNumber);

	addConstValue ("chips", 1);

	initBinnings ();
	initDataTypes ();

	// init shared memory segment
	if (sharedMemId == -1)
	{
		struct shmid_ds ds;
		sharedMemId = shmget (IPC_PRIVATE, sizeof (unsigned long) + sizeof (imghdr) + chipByteSize (), 0666);
		if (sharedMemId == -1)
		{
			logStream (MESSAGE_ERROR) << "Cannot create shared memory segment: " << strerror (errno) << sendLog;
			return -1;
		}
		dataBufferSize = chipByteSize ();
		shmBuffer = (char *) shmat (sharedMemId, NULL, 0);
		if (shmBuffer == (void *) -1)
		{
			logStream (MESSAGE_ERROR) << "Cannot attach to shared memory with key " << sharedMemId << ": " << strerror (errno) << sendLog;
			return -1;
		}
		if (shmctl (sharedMemId, IPC_STAT, &ds) < 0 || shmctl (sharedMemId, IPC_RMID, &ds) < 0)
		{
			logStream (MESSAGE_ERROR) << "Cannot perform shmctl call: " << strerror (errno) << sendLog;
			return -1;
		}
		focusingHeader = (struct imghdr*) (shmBuffer + sizeof (unsigned long));
		dataBuffer = shmBuffer + sizeof (unsigned long) + sizeof (struct imghdr);
	}
	else
	{
		focusingHeader = (struct imghdr *) malloc (sizeof (struct imghdr));
	}

	if (tempCCDHistory != NULL)
	{
		addTimer (tempCCDHistoryInterval->getValueInteger (), new rts2core::Event (EVENT_TEMP_CHECK));
	}

	return rts2core::ScriptDevice::initValues ();
}

void Camera::addFilters (char *opt)
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
						maskState (CAM_MASK_EXPOSE | CAM_MASK_FT | BOP_TEL_MOVE, CAM_NOEXPOSURE | CAM_NOFT, "exposure finished", NAN, NAN, exposureConn);

					// drop FT flag
					else
						maskState (CAM_MASK_FT, CAM_NOFT, "ft exposure chip finished", NAN, NAN, exposureConn);
					break;
				case -1:
					maskState (DEVICE_ERROR_MASK | CAM_MASK_EXPOSE | CAM_MASK_READING | BOP_TEL_MOVE, DEVICE_ERROR_HW | CAM_NOEXPOSURE | CAM_NOTREADING, "exposure finished with error", NAN, NAN, exposureConn);
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
			maskState (CAM_MASK_READING | CAM_MASK_HAS_IMAGE, CAM_NOTREADING | CAM_HAS_IMAGE, "readout ended", NAN, NAN, exposureConn);
		else
			maskState (DEVICE_ERROR_MASK | CAM_MASK_READING, DEVICE_ERROR_HW | CAM_NOTREADING, "readout ended with error", NAN, NAN, exposureConn);
	}
}

void Camera::afterReadout ()
{
	setTimeout (USEC_SEC);
}

int Camera::setValue (rts2core::Value * old_value, rts2core::Value * new_value)
{
	if (old_value == camFocVal)
	{
		return setFocuser (new_value->getValueInteger ()) == 0 ? 0 : -2;
	}
	if (old_value == camFilterVal)
	{
		int ret = setFilterNum (new_value->getValueInteger ()) == 0 ? 0 : -2;
		if (ret == 0)
			offsetForFilter (new_value->getValueInteger ());
		return ret;
	}
	int i = 0;
	for (std::vector <rts2core::ValueSelection *>::iterator iter = camFilterVals.begin (); iter != camFilterVals.end (); iter++, i++)
	{
		if (*iter == old_value)
		{
			int ret = setFilterNum (new_value->getValueInteger (), wheelDevices[i]) == 0 ? 0 : -2;
			if (ret == 0)
				offsetForFilter (new_value->getValueInteger (), i);
			return ret;
		}
	}
	if (old_value == tempSet)
	{
		return setCoolTemp (new_value->getValueFloat ()) == 0 ? 0 : -2;
	}
	if (old_value == chipUsedReadout)
	{
		rts2core::ValueRectangle *rect = (rts2core::ValueRectangle *) new_value;
		return box (rect->getXInt (), rect->getYInt (), rect->getWidthInt (), rect->getHeightInt (), rect) == 0 ? 0 : -2;
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
	if (old_value == coolingOnOff)
	{
		return switchCooling (((rts2core::ValueBool *) new_value)->getValueBool ());
	}
	if (old_value == exposure)
	{
		setExposure (new_value->getValueDouble ());
		return 0;
	}
	return rts2core::ScriptDevice::setValue (old_value, new_value);
}

void Camera::valueChanged (rts2core::Value *changed_value)
{
	if (changed_value == binning)
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		setBinning (bin->horizontal, bin->vertical);
	}
	ScriptDevice::valueChanged (changed_value);
}

void Camera::addTempCCDHistory (float temp)
{
	tempCCDHistory->addValue (temp, tempCCDHistorySize->getValueInteger ());
	tempCCDHistory->calculate ();
	sendValueAll (tempCCDHistory);
	tempCCD->setValueFloat (tempCCDHistory->getValueFloat ());
}

void Camera::deviceReady (rts2core::Connection * conn)
{
	// if that's filter wheel
	if (wheelDevices.size () > 0 && conn->getOtherDevClient ())
	{
		// find filter wheel for which device connected..
		int i = 0;
		for (std::vector <const char *>::iterator iter = wheelDevices.begin (); iter != wheelDevices.end (); iter++, i++)
		{
			if (!strcmp (conn->getName (), *iter))
			{
				// copy content of device filter variable to our list..
				rts2core::Value *val = conn->getValue ("filter");
				// it's filter and it's correct type
				if (val != NULL && val->getValueType () == RTS2_VALUE_SELECTION)
				{
					camFilterVals[i]->duplicateSelVals ((rts2core::ValueSelection *) val);
					// sends filter metainformations to all connected devices
					updateMetaInformations (camFilterVals[i]);
					if (iter == wheelDevices.begin ())
					{
						camFilterVal->duplicateSelVals ((rts2core::ValueSelection *) val);
						// sends filter metainformations to all connected devices
						updateMetaInformations (camFilterVal);
					}
				}
			}
		}
	}
}

void Camera::postEvent (rts2core::Event * event)
{
	switch (event->getType ())
	{
		case EVENT_FILTER_MOVE_END:
			if (filterMoving && filterMoving->getValueInteger () > 0)
			{
				filterMoving->dec ();
				if (filterMoving->getValueInteger () == 0)
					checkQueuedExposures ();
			}
			break;
		case EVENT_FOCUSER_END_MOVE:
			if (event->getArg () == this && focuserMoving && focuserMoving->getValueBool ())
			{
				focuserMoving->setValueBool (false);
				checkQueuedExposures ();
			}
			break;
		case EVENT_TEMP_CHECK:
			temperatureCheck ();
			addTimer (tempCCDHistoryInterval->getValueInteger (), event);
			return;
	}
	rts2core::ScriptDevice::postEvent (event);
}

int Camera::idle ()
{
	checkExposures ();
	checkReadouts ();
	return rts2core::ScriptDevice::idle ();
}

void Camera::changeMasterState (int old_state, int new_state)
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
	rts2core::ScriptDevice::changeMasterState (old_state, new_state);
}

int Camera::camStartExposure ()
{
	// check if we aren't blocked
	// we can allow this test as camStartExposure is called only after quedExpNumber was decreased
	if ((!expType || expType->getValueInteger () == 0)
		&& (
			(getDeviceBopState () & BOP_EXPOSURE)
			|| (getMasterStateFull () & BOP_EXPOSURE)
			|| (getDeviceBopState () & BOP_TRIG_EXPOSE)
			|| (getMasterStateFull () & BOP_TRIG_EXPOSE)
			|| (filterMoving && filterMoving->getValueInteger () > 0)
			|| (focuserMoving && focuserMoving->getValueBool () == true)
		))
	{
		// no conflict, as when we are called, quedExpNumber will already be decreased
		quedExpNumber->inc ();
		sendValueAll (quedExpNumber);

		if (waitingForNotBop->getValueBool () == false)
		{
			waitingForNotBop->setValueBool (true);
			sendValueAll (waitingForNotBop);
		}

		if (!((getDeviceBopState () & BOP_EXPOSURE) || (getMasterStateFull () & BOP_EXPOSURE)) && ((getDeviceBopState () & BOP_TRIG_EXPOSE) || (getMasterStateFull () & BOP_TRIG_EXPOSE)))
			maskState (BOP_WILL_EXPOSE, BOP_WILL_EXPOSE, "device plan to exposure soon", NAN, NAN, exposureConn);

		return 0;
	}

	return camStartExposureWithoutCheck ();
}

int Camera::camStartExposureWithoutCheck ()
{
	int ret;

	incExposureNumber ();

	// recalculate number of data channels
	if (dataChannels)
	{
		dataChannels->setValueInteger (0);

		// write channels and their order..
		for (size_t i = 0; i < channels->size (); i++)
		{
			if ((*channels)[i])
			{
				dataChannels->inc ();
			}
		}

		sendValueAll (dataChannels);
	}

	binningX->setValueInteger (((Binning2D *)(binning->getData ()))->horizontal);
	binningY->setValueInteger (((Binning2D *)(binning->getData ()))->vertical);

	if (getNeedReload ())
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		setBinning (bin->horizontal, bin->vertical);
		clearNeedReload ();
	}

	ret = startExposure ();
	if (ret)
		return ret;

	double now = getNow ();

	exposureEnd->setValueDouble (now + exposure->getValueDouble ());
	infoAll ();

	maskState (CAM_MASK_EXPOSE | BOP_TEL_MOVE | BOP_WILL_EXPOSE | CAM_MASK_HAS_IMAGE, CAM_EXPOSING | BOP_TEL_MOVE, "exposure started", now, exposureEnd->getValueDouble (), exposureConn);

	logStream (MESSAGE_INFO) << "starting " << TimeDiff (exposure->getValueFloat ()) << " exposure for '" << (exposureConn ? exposureConn->getName () : "null") << "'" << sendLog;

	lastFilterNum = getFilterNum ();
	// call us to check for exposures..
	long new_timeout;
	new_timeout = camWaitExpose ();
	if (new_timeout >= 0)
	{
		setTimeout (new_timeout);
	}

	// increas buffer size
	if (suggestBufferSize () > 0 && dataBufferSize < suggestBufferSize ())
	{
		delete[] dataBuffer;
		dataBufferSize = suggestBufferSize ();
		dataBuffer = new char[dataBufferSize];
	}

	// check if that comes from old request
	if (sendOkInExposure && exposureConn)
	{
		sendOkInExposure = false;
		exposureConn->sendCommandEnd (DEVDEM_OK, "executing exposure from queue");
	}

	return 0;
}

int Camera::camExpose (rts2core::Connection * conn, int chipState, bool fromQue)
{
	int ret;

	// if it is currently exposing
	// or performing other op that can block command execution
	// or there are queued values which needs to be dealed before we can start exposing
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

int Camera::camBox (rts2core::Connection * conn, int x, int y, int width, int height)
{
	int ret;
	ret = box (x, y, width, height);
	if (!ret)
		return ret;
	conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "invalid box size");
	return ret;
}

int Camera::camCenter (rts2core::Connection * conn, int in_h, int in_w)
{
	int ret;
	ret = center (in_h, in_w);
	if (ret)
		conn->sendCommandEnd (DEVDEM_E_PARAMSVAL, "cannot set box size");
	return ret;
}

int Camera::readoutStart ()
{
	int ret;
	for (int i = 0; i < (dataChannels ? dataChannels->getValueInteger () : 1); i++)
	{
		ret = sendFirstLine (i, getPhysicalChannel (i + 1));
		if (ret)
			return ret;
	}
	return ret;
}

int Camera::camReadout (rts2core::Connection * conn)
{
	// if we can do exposure, do it..
	if (quedExpNumber->getValueInteger () > 0 && exposureConn && supportFrameTransfer ())
	{
		checkQueChanges (getStateChip (0) & ~CAM_EXPOSING);
		maskState (CAM_MASK_READING | CAM_MASK_FT, CAM_READING | CAM_FT, "starting frame transfer", NAN, NAN, exposureConn);
		if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
			currentImageData = -1;
		else
			startImageData (conn);
		if (queValues.empty ())
			// remove exposure flag from state
			camExpose (exposureConn, getStateChip (0) & ~CAM_MASK_EXPOSE, true);
	}
	else
	{
		// open data connection - wait socket
		// end exposure as well..
		// do not signal BOP_TEL_MOVE down if there are exposures in que
		double now = getNow ();
		maskState (CAM_MASK_EXPOSE | CAM_MASK_READING | BOP_TEL_MOVE, CAM_NOEXPOSURE | CAM_READING, "readout started", now, now + (double) chipUsedSize () / pixelsSecond->getValueDouble (), exposureConn);
		if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)	
			currentImageData = -1;
		else
			startImageData (conn);
	}

	if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
		calculateDataSize = chipByteSize ();

	if (currentImageData != -1 || calculateStatistics->getValueInteger () == STATISTIC_ONLY)
	{
		readoutPixels = getUsedHeightBinned () * getUsedWidthBinned ();
		timeReadoutStart = getNow ();
		return readoutStart ();
	}
	maskState (DEVICE_ERROR_MASK | CAM_MASK_READING, DEVICE_ERROR_HW | CAM_NOTREADING, "readout failed", NAN, NAN, exposureConn);
	conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
	return -1;
}

int Camera::setFilterNum (int new_filter, const char *fn)
{
	int ret = -1;
	if (wheelDevices.size () > 0)
	{
		struct filterStart fs;
		if (fn == NULL)
			fs.filterName = wheelDevices[0];
		else
			fs.filterName = fn;

		fs.filter = new_filter;
		fs.arg = this;
		postEvent (new rts2core::Event (EVENT_FILTER_START_MOVE, (void *) &fs));
		// filter move will be performed
		if (fs.filter == -1)
		{
			filterMoving->inc ();
			sendValueAll (filterMoving);
			ret = 0;
		}
		else
		{
			ret = -1;
		}
	}
	return ret;
}

void Camera::offsetForFilter (int new_filter, int fn)
{
	if (!focuserDevice)
		return;
	struct focuserMove fm;
	if (fn < 0)
	{
		if ((size_t) new_filter >= camFilterOffsets->size ())
			return;
		fm.value = (*camFilterOffsets)[new_filter];
	}
	else
	{
		if ((size_t) new_filter >= camFiltersOffsets[fn]->size ())
			return;
		fm.value = (*(camFiltersOffsets[fn]))[new_filter];
	}
	fm.focuserName = focuserDevice;
	fm.conn = this;
	postEvent (new rts2core::Event (EVENT_FOCUSER_OFFSET, (void *) &fm));
	if (fm.focuserName)
		return;
	if (focuserMoving)
	{
		focuserMoving->setValueBool (true);
		sendValueAll (focuserMoving);
	}
}

int Camera::getStateChip (int chip)
{
	return (getState () & (CAM_MASK_CHIP << (chip * 4))) >> (0 * 4);
}

void Camera::setExposureMinMax (double exp_min, double exp_max)
{
	exposure->setMin (exp_min);
	exposure->setMax (exp_max);
	sendValueAll (exposure);
}

int Camera::getFilterNum (const char *fn)
{
	if (wheelDevices.size () > 0)
	{
		struct filterStart fs;
		if (fn == NULL)
			fs.filterName = wheelDevices[0];
		else
			fs.filterName = fn;
		fs.filter = -1;
		postEvent (new rts2core::Event (EVENT_FILTER_GET, (void *) &fs));
		return fs.filter;
	}
	else if (camFilterVal != NULL)
	{
		return getCamFilterNum ();
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
	fm.conn = this;
	postEvent (new rts2core::Event (EVENT_FOCUSER_SET, (void *) &fm));
	if (fm.focuserName)
		return -1;
	if (focuserMoving)
	{
		focuserMoving->setValueBool (true);
		sendValueAll (focuserMoving);
	}
	return 0;
}

int Camera::getFocPos ()
{
	if (!focuserDevice)
		return -1;
	struct focuserMove fm;
	fm.focuserName = focuserDevice;
	postEvent (new rts2core::Event (EVENT_FOCUSER_GET, (void *) &fm));
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

int Camera::commandAuthorized (rts2core::Connection * conn)
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
	return rts2core::ScriptDevice::commandAuthorized (conn);
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
	rts2core::ScriptDevice::setFullBopState (new_state);
	if (!(new_state & BOP_EXPOSURE) && quedExpNumber->getValueInteger () > 0 && waitingForNotBop->getValueBool ())
	{
		if ((new_state & BOP_TRIG_EXPOSE) && !(getState () & BOP_WILL_EXPOSE))
		{
			// this will trigger device transition from BOP_TRIG_EXPOSE to not BOP_TRIG_EXPOSE, which will trigger this code again..
			maskState (BOP_WILL_EXPOSE, BOP_WILL_EXPOSE, "device plan to exposure soon", NAN, NAN, exposureConn);
		}
		else
		{
			waitingForNotBop->setValueBool (false);
			quedExpNumber->dec ();
			camStartExposureWithoutCheck ();
		}
	}
}

void Camera::setFilterOffsets (char *opt)
{
	char *s = opt;
	char *o = s;

	for (; *o; o++)
	{
		if (*o == ':')
		{
			*o = '\0';
			camFilterOffsets->addValue (atof (s));
			s = o + 1;
		}
	}
	if (s != 0)
		camFilterOffsets->addValue (atof (s));
}
