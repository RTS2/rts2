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
#include <strings.h>
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
#include "sep/sep.h"

#define OPT_WCS_MULTI         OPT_LOCAL + 400
#define OPT_WCS_CDELT         OPT_LOCAL + 402
#define OPT_FOCUS             OPT_LOCAL + 403
#define OPT_WHEEL             OPT_LOCAL + 404
#define OPT_WITHSHM           OPT_LOCAL + 405
#define OPT_FILTER_OFFSETS    OPT_LOCAL + 406
#define OPT_OFFSETS_FILE      OPT_LOCAL + 407
#define OPT_DETSIZE           OPT_LOCAL + 408
#define OPT_CHANNELS_STARTS   OPT_LOCAL + 409
#define OPT_CHANNELS_DELTAS   OPT_LOCAL + 410
#define OPT_TRIMS_XY          OPT_LOCAL + 411
#define OPT_TRIMS_END         OPT_LOCAL + 412
#define OPT_WCS_AUXS          OPT_LOCAL + 420
#define OPT_COMMENTS          OPT_LOCAL + 421
#define OPT_HISTORIES         OPT_LOCAL + 422
#define OPT_RTS2_COOLING      OPT_LOCAL + 423

#define EVENT_TEMP_CHECK      RTS2_LOCAL_EVENT + 676

using namespace rts2camd;

FilterVal::FilterVal (Camera *master, const char *n, char fil)
{
	master->createValue (filter, (std::string ("FILT") + fil).c_str (), "used filter number", true, RTS2_VALUE_WRITABLE, CAM_EXPOSING);
	master->createValue (offsets, (std::string ("filter_offsets_") + fil).c_str (), "filter offsets", false, RTS2_VALUE_WRITABLE);
	master->createValue (moving, (std::string ("filter_moving_") + fil).c_str (), "if filter wheel is moving", false);

	name = n;
}

void Camera::initCameraChip (int in_width, int in_height, double in_pixelX, double in_pixelY)
{
	setSize (in_width, in_height, 0, 0);
	pixelX = in_pixelX;
	pixelY = in_pixelY;
}

int Camera::setBinning (int in_vert, int in_hori)
{
	// scale WCS parameters
	if (wcs_crpix1)
	{
		wcs_crpix1->setValueDouble ((default_crpix[0] - getUsedX ()) / in_hori);
		wcs_crpix2->setValueDouble ((default_crpix[1] - getUsedY ()) / in_vert);

		sendValueAll (wcs_crpix1);
		sendValueAll (wcs_crpix2);
	}
	if (wcs_cdelta1 && wcs_cdelta2)
	{
		wcs_cdelta1->setValueDouble (default_cd[0] * in_hori);
		wcs_cdelta2->setValueDouble (default_cd[1] * in_vert);

		sendValueAll (wcs_cdelta1);
		sendValueAll (wcs_cdelta2);
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
	if (wcs_crpix1)
	{
		wcs_crpix1->setValueDouble ((default_crpix[0] - _x) / binningHorizontal ());
		wcs_crpix2->setValueDouble ((default_crpix[1] - _y) / binningVertical ());
		sendValueAll (wcs_crpix1);
		sendValueAll (wcs_crpix2);
	}
	chipUsedReadout->setInts (_x, _y, _width, _height);
	if (retv)
		retv->setInts (_x, _y, _width, _height);
	sendValueAll (chipUsedReadout);

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
		return -2;				 // exposure ends
	}
								 // timeout
	return ((long int) ((exposureEnd->getValueDouble () - n) * USEC_SEC));
}

int Camera::endExposure (int ret)
{
	if (modeCount)
		memset (modeCount, 0, modeCountSize * sizeof (uint32_t));
	if (exposureConn)
	{
		logStream (MESSAGE_INFO) << "end exposure for " << exposureConn->getName () << sendLog;
		return camReadout (exposureConn);
	}
	if (getStateChip (0) & (CAM_EXPOSING | CAM_EXPOSING_NOIM | CAM_SHIFT))
	{
		switch (ret)
		{
			case -5:
				logStream (MESSAGE_INFO) << "end shifting partial exposure" << sendLog;
				return 0;
			case -4:
				logStream (MESSAGE_INFO) << "end exposure, readout was not commanded" << sendLog;
				break;
			default:
				stopExposure ();
				logStream (MESSAGE_WARNING) << "end exposure without exposure connection, state " << getStateChip (0) << sendLog;
		}
	}

	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
	maskState (CAM_MASK_EXPOSE | CAM_MASK_SHIFTING | CAM_MASK_READING | CAM_MASK_FT | BOP_TEL_MOVE | BOP_WILL_EXPOSE, CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT, "chip exposure interrupted", NAN, NAN, exposureConn);
	return 0;
}

int Camera::stopExposure ()
{
	quedExpNumber->setValueInteger (0);
	sendValueAll (quedExpNumber);
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
	// delete connection in shared data
	if (sharedData)
	{
		// unmap client from all connectons
		for (int i = 0; i < sharedMemNum; i++)
		{
			sharedData->removeClient (i, conn->getCentraldId (), false);
		}
	}
	return rts2core::ScriptDevice::deleteConnection (conn);
}

int Camera::endReadout ()
{
	// that will do anything only if the end was not marked
	updateReadoutSpeed (readoutPixels);

	double n = getNow ();

	double tr = readoutTime->getValueDouble ();
	double tt = n - timeTransferStart;

	transferTime->setValueDouble (tt);
	sendValueAll (transferTime);

	double transferSecond = readoutPixels / tt;

	logStream (MESSAGE_INFO) << "readout " <<  readoutPixels << " pixels in " << TimeDiff (tr)
		<< " (" << std::setiosflags (std::ios_base::fixed) << pixelsSecond->getValueDouble () << " pixels per second, transfered with " << transferSecond << " pixels per second)" << sendLog;

	clearReadout ();
	if (currentImageTransfer == SHARED && exposureConn)
	{
		if (currentImageData >= 0)
		{
			exposureConn->endSharedData (currentImageData, true);
			currentImageData = -1;
		}
	}
	if (quedExpNumber->getValueInteger () > 0 && exposureConn)
	{
		// do not report that we start exposure
		camExpose (exposureConn, getStateChip(0) & CAM_MASK_EXPOSE, true, lastCareBlock);
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
	if (currentImageTransfer == FITS)
		return;

	int chnTot = dataChannels ? dataChannels->getValueInteger () : 1;
	size_t chansize[chnTot];
	int i;
	for (i = 0; i < chnTot; i++)
		chansize[i] = lastExposureChipByteSize () + sizeof (imghdr);

	if (sharedData)
	{
		int segments[chnTot];
		sharedData->clearChan2Seg ();
		// map channels
		for (i = 0; i < chnTot; i++)
		{
			segments[i] = sharedData->addClient (chansize[i], i, conn->getCentraldId ());
			if (segments[i] < 0)
				break;
		}
		// if there aren't segments left, send over TCP
		if (i < chnTot)
		{
			// clear allocation we did with addClient above
			for (int j = 0; j < i; j++)
			{
				sharedData->removeClient (segments[j], conn->getCentraldId ());
			}
			sharedData->clearChan2Seg ();
			logStream (MESSAGE_WARNING) << "starting binary connection instead of shared data connection" << sendLog;
			currentImageData = conn->startBinaryData (dataType->getValueInteger (), chnTot, chansize);
			currentImageTransfer = TCPIP;
		}
		else
		{
			currentImageData = conn->startSharedData (sharedData, chnTot, segments);
			currentImageTransfer = SHARED;
		}
	}
	else
	{
		currentImageData = conn->startBinaryData (dataType->getValueInteger (), chnTot, chansize);
		currentImageTransfer = TCPIP;
	}
	exposureConn = conn;
}

int Camera::sendFirstLine (int chan, int pchan)
{
	struct imghdr *header = NULL;

	fhd->channel = htons (pchan);

	sum->setValueDouble (0);
	average->setValueDouble (0);
	max->setValueDouble (-LONG_MAX);
	min->setValueDouble (LONG_MAX);
	computedPix->setValueLong (0);

	switch (currentImageTransfer)
	{
		case SHARED:
			header = (struct imghdr*) (sharedData->getChannelData (chan));
			memcpy (header, fhd, sizeof(imghdr));
			sharedData->dataWritten (chan, sizeof (imghdr));
			break;
		case TCPIP:
			if (exposureConn)
				return exposureConn->sendBinaryData (currentImageData, chan, (char *) fhd, sizeof (imghdr));
			break;
		case FITS:
			break;
	}
	return 0;
}

bool Camera::supportFrameTransfer ()
{
	return false;
}

Camera::Camera (int in_argc, char **in_argv, rounding_t binning_rounding):rts2core::ScriptDevice (in_argc, in_argv, DEVICE_TYPE_CCD, "C0")
{
	binningRounding = binning_rounding;
	expType = NULL;

	dataChannels = NULL;
	channels = NULL;

	rts2ControlCooling = NULL;

	tempAir = NULL;
	tempCCD = NULL;
	tempCCDHistory = NULL;
	tempCCDHistoryInterval = NULL;
	tempCCDHistorySize = NULL;
	tempSet = NULL;
	nightCoolTemp = NULL;
	coolingOnOff = NULL;

	detsize = NULL;
	chan1offset = NULL;
	chan2offset = NULL;

	chan1delta = NULL;
	chan2delta = NULL;
	acquireTime = NULL;

	shiftstoreLines = NULL;

	timeReadoutStart = NAN;
	timeTransferStart = NAN;

	multi_wcs = '\0';

	wcs_ctype1 = wcs_ctype2 = NULL;
	wcs_crpix1 = wcs_crpix2 = NULL;
	default_crpix[0] = default_crpix[1] = 0;

	wcs_cdelta1 = wcs_cdelta2 = wcs_crota = NULL;
	default_cd[0] = default_cd[1] = 1;
	default_cd[2] = 0;

	pixelX = NAN;
	pixelY = NAN;

	nAcc = 1;

	dataBuffers = NULL;
	dataWritten = NULL;

	lastExposurePixels = 0;
	lastExposureBytes = 0;

	histories = 0;
	comments = 0;

	sharedData = NULL;
	sharedMemNum = -1;

	currentImageData = -1;
	currentImageTransfer = TCPIP;

	filterOffsetFile = NULL;

	realTimeDataTransferCount = -1;

	lastCareBlock = true;

	createValue (ccdRealType, "CCD_TYPE", "camera type", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (serialNumber, "CCD_SER", "camera serial number", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);
	createValue (ccdChipType, "CCD_CHIP", "camera chip type", true, RTS2_VALUE_WRITABLE | RTS2_VALUE_AUTOSAVE);

	createValue (imageType, "IMAGETYP", "IRAF based image type", true, RTS2_VALUE_WRITABLE);
	imageType->addSelVal ("object");
	imageType->addSelVal ("dark");
	imageType->addSelVal ("zero");
	imageType->addSelVal ("flat");

	createValue (objectName, "OBJECT", "target object name", true, RTS2_VALUE_WRITABLE);

	createValue (calculateStatistics, "calculate_stat", "if statistics values should be calculated", false, RTS2_VALUE_WRITABLE);
	calculateStatistics->addSelVal ("yes");
	calculateStatistics->addSelVal ("without mode");
	calculateStatistics->addSelVal ("only statistics");
	calculateStatistics->addSelVal ("no");
	calculateStatistics->setValueInteger (STATISTIC_YES);

	createValue (average, "average", "image average", false);
	createValue (max, "max", "maximum pixel value", false);
	createValue (min, "min", "minimal pixel value", false);
	createValue (sum, "sum", "sum of pixels readed out", false);
	createValue (image_mode, "image_mode", "mode (most often pixel value)", false);

	// mode histogram
	modeCount = NULL;
	modeCountSize = 0;

	createValue (computedPix, "computed", "number of pixels so far computed", false);

	createValue (calculateCenter, "center_cal", "calculate center box statistics", false, RTS2_VALUE_WRITABLE | RTS2_DT_ONOFF);
	calculateCenter->setValueBool (false);

	createValue (centerBox, "center_box", "calculate center box coordinates", false, RTS2_VALUE_INTEGER | RTS2_VALUE_WRITABLE);
	centerBox->setInts (-1, -1, -1, -1);

	createValue (sepFind, "sep_cal", "find stars with SEP", false, RTS2_VALUE_WRITABLE);
	createValue (sepX, "sep_X", "X positions of stars", false);
	createValue (sepY, "sep_Y", "Y positions of stars", false);
	createValue (sepFluxes, "sep_fluxes", "star fluxes", false);

	sepFind->setValueBool (false);

	createValue (slitPosX, "slitposx", "[pixels] slit position along dithering axis", true, RTS2_VALUE_WRITABLE);
	slitPosX->setValueDouble (-1);
	createValue (slitPosY, "slitposy", "[pixels] slit position along dithering axis", true, RTS2_VALUE_WRITABLE);
	slitPosY->setValueDouble (-1);

	createValue (centerCutLevel, "center_cut_level", "[ADU] lower limit for ADU value to be considered for pixels", false, RTS2_VALUE_WRITABLE);
	centerCutLevel->setValueDouble (0);

	createValue (sumsX, "sums_Xs", "sums along X axis", false, RTS2_VALUE_DOUBLE);
	createValue (sumsY, "sums_Ys", "sums along Y axis", false, RTS2_VALUE_DOUBLE);

	createValue (centerX, "center_X", "center pixel in X", false);
	createValue (centerY, "center_Y", "center pixel in Y", false);

	createValue (centerMax, "center_max", "maximal ADU value", false);
	createValue (centerSums, "center_sums", "number of measurements to sum for statistics", false, RTS2_VALUE_WRITABLE);
	centerSums->setValueInteger (5);
	createValue (centerStat, "center_stat", "center statistics", false);

	createValue (centerAvg, "center_avg", "average of pixels above threshold", false);
	createValue (centerAvgStat, "center_avg_stat", "statistics of average of pixels above threshold", false);

	createValue (quedExpNumber, "que_exp_num", "number of exposures in que", false, RTS2_VALUE_WRITABLE, 0);
	quedExpNumber->setValueInteger (0);

	createValue (exposureNumber, "exposure_num", "number of exposures camera took from driver restart", false, RTS2_VALUE_WRITABLE, 0);
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

	createValue (lastImagePath, "last_image", "path to the last image", false, RTS2_VALUE_WRITABLE);

	sendOkInExposure = false;

	createValue (pixelsSecond, "pixels_second", "[pixels/second] average readout speed", false, RTS2_DT_KMG);
	createValue (readoutTime, "readout_time", "[s] data readout time", false, RTS2_DT_TIMEINTERVAL);
	createValue (transferTime, "transfer_time", "[s] data transfer time, including overhead", false, RTS2_DT_TIMEINTERVAL);

	createValue (camFocVal, "focpos", "position of focuser", false, RTS2_VALUE_WRITABLE, CAM_EXPOSING);

	camFilterVal = NULL;
	camFilterOffsets = NULL;
	useFilterOffsets = NULL;
	focuserDevice = NULL;

	exposureConn = NULL;

	createValue (focuserMoving, "foc_moving", "if focuser is moving", false);
	focuserMoving->setValueBool (false);

	createValue (needReload, "need_reload", "camera needs to be reloaded after finishing the exposure", false);
	needReload->setValueBool (false);

	// other options..
	addOption (OPT_RTS2_COOLING, "no-autocooling", 0, "when set, RTS2 did not switch cooling off at the end of night");
	addOption (OPT_COMMENTS, "add-comments", 1, "add given number of comment fields");
	addOption (OPT_HISTORIES, "add-history", 1, "add given number of history fields");
	addOption (OPT_FOCUS, "focdev", 1, "name of focuser device, which will be granted to do exposures without priority");
	addOption (OPT_WHEEL, "wheeldev", 1, "name of device which is used as filter wheel; - for internal wheel device");
	addOption (OPT_FILTER_OFFSETS, "filter-offsets", 1, "camera filter offsets, separated with :");
	addOption (OPT_OFFSETS_FILE, "offsets-file", 1, "configuration file for camera filter offsets. Filter names are separated with space from filter offsets");
	addOption ('e', NULL, 1, "default exposure");
	addOption (OPT_WCS_AUXS, "wcs-aux", 1, "suffixes of auxiliary WCS values, separated with :");
	addOption (OPT_WCS_CDELT, "wcs", 1, "WCS CD matrix (CRPIX1:CRPIX2:CDELT1:CDELT2:CROTA in default, unbinned configuration)");
	addOption (OPT_WCS_MULTI, "wcs-multi", 1, "letter for multiple WCS (A-Z)");
	addOption (OPT_WITHSHM, "with-shm", 2, "use given numbers of segments of shared memory");

	// detector sizes, channel starting points and offsets
	addOption (OPT_DETSIZE, "detsize", 1, "detector size - X:Y:W:H");
	addOption (OPT_TRIMS_XY, "trimstart", 1, "trimmed (good data) XY section on unbinned chip - x1:y1,..");
	addOption (OPT_TRIMS_END, "trimend", 1, "trimmed (good data) XY ends on unbinned chip - x1:y1,..");
	addOption (OPT_CHANNELS_STARTS, "chanstarts", 1, "channel starts - X1:Y1,:..");
	addOption (OPT_CHANNELS_DELTAS, "chandeltas", 1, "channel deltas - DX1:DY1,..");
}

Camera::~Camera ()
{
	delete sharedData;
	delete fhd;

	delete[] dataBuffers;
	delete[] dataWritten;

	delete[] modeCount;
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
			for (std::list <FilterVal>::iterator iter = camFilterVals.begin (); iter != camFilterVals.end (); iter++)
			{
				if (!strcmp (iter->name, conn->getName ()))
					return new ClientFilterCamera (conn, &(*iter));
			}
			break;
		case DEVICE_TYPE_FOCUS:
			if (!strcmp (focuserDevice, conn->getName ()))
				return new ClientFocusCamera (conn);
	}
	return rts2core::ScriptDevice::createOtherType (conn, other_device_type);
}

void Camera::checkQueChanges (rts2_status_t fakeState)
{
	// don't check values changes if there are qued exposures and exposure is still running
	if (quedExpNumber->getValueInteger () > 0 && (fakeState & (CAM_EXPOSING | CAM_READING)))
		return;
	// do not check if we have queued exposures
	fakeState |= (getDeviceBopState () & BOP_TRIG_EXPOSE);
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
		camExpose (exposureConn, getStateChip (0), false, lastCareBlock);
	}
	infoAll ();
}

int Camera::killAll (bool callScriptEnds)
{
	timeReadoutStart = NAN;

	waitingForNotBop->setValueBool (false);
	waitingForEmptyQue->setValueBool (false);

	for (std::list <FilterVal>::iterator iter = camFilterVals.begin (); iter != camFilterVals.end (); iter++)
	{
		iter->moving->setValueBool (false);
		sendValueAll (iter->moving);
	}

	if (exposureConn && currentImageData >= 0)
	{
		// end actual data connections
		switch (currentImageTransfer)
		{
			case SHARED:
				exposureConn->endSharedData (currentImageData, false);
				currentImageTransfer = TCPIP;
				break;
			case TCPIP:
				exposureConn->endBinaryData (currentImageData);
				break;
			case FITS:
				currentImageTransfer = TCPIP;
		}
		currentImageData = -1;
	}

	stopExposure ();

	sendValueAll (waitingForNotBop);
	sendValueAll (waitingForEmptyQue);

	centerStat->clearStat ();
	sendValueAll (centerStat);

	centerAvgStat->clearStat ();
	sendValueAll (centerAvgStat);

	maskState (CAM_MASK_EXPOSE | CAM_MASK_SHIFTING | CAM_MASK_READING | CAM_MASK_FT | BOP_TEL_MOVE | BOP_WILL_EXPOSE | DEVICE_ERROR_KILL, CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT | DEVICE_ERROR_KILL, "exposure interrupted", NAN, NAN, exposureConn);

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
		needReload->setValueBool (true);
	}
	else
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		setBinning (bin->horizontal, bin->vertical);
	}
	sendValueAll (binning);

	dataType->setValueInteger (0);
	sendValueAll (dataType);

	// Clear PRIORITY CHANGED state
	maskState (DEVICE_ERROR_KILL, 0, "cleared PRIORITY CHANGED state");

	return rts2core::ScriptDevice::scriptEnds ();
}

int Camera::info ()
{
        if (camFilterVal)
        	camFilterVal->setValueInteger (getFilterNum ());

	std::list <FilterVal>::iterator viter;
	std::vector <const char *>::iterator niter;
	for (viter = camFilterVals.begin (), niter = wheelDevices.begin (); viter != camFilterVals.end (); viter++, niter++)
	{
		viter->filter->setValueInteger (getFilterNum (*niter));
	}
	camFocVal->setValueInteger (getFocPos ());
	return rts2core::ScriptDevice::info ();
}

void fillPairs (rts2core::DoubleArray *a1, rts2core::DoubleArray *a2, const char *opt)
{
	std::vector <std::string> chans = SplitStr (optarg, ",");
	for (std::vector <std::string>::iterator iter = chans.begin (); iter != chans.end (); iter++)
	{
		std::vector <std::string> xy = SplitStr (iter->c_str (), ":");
		if (xy.size () != 2)
			throw rts2core::Error ("invalid channel pair: " + *iter);
		a1->addValue (atof (xy[0].c_str ()));
		a2->addValue (atof (xy[1].c_str ()));
	}
}

int Camera::processOption (int in_opt)
{
	std::vector <std::string> params;
	switch (in_opt)
	{
		case OPT_HISTORIES:
			histories = atoi (optarg);
			break;
		case OPT_COMMENTS:
			comments = atoi (optarg);
			break;
		case OPT_RTS2_COOLING:
			if (rts2ControlCooling != NULL)
				rts2ControlCooling->setValueBool (false);
			break;
		case OPT_FOCUS:
			focuserDevice = optarg;
			break;
		case OPT_WHEEL:
			if (!strcmp (optarg, "-"))
				wheelDevices.push_back (NULL);
			else
				wheelDevices.push_back (optarg);
			break;
		case OPT_FILTER_OFFSETS:
			createFilter ();
			setFilterOffsets (optarg);
			break;
		case OPT_OFFSETS_FILE:
			if (filterOffsetFile)
			{
				std::cerr << "the program does not accept two filter offsets files" << std::endl;
				return -1;
			}
			createValue (filterOffsetFile, "offset_file", "filter offset file", false, RTS2_VALUE_WRITABLE);
			setFilterOffsetFile (optarg);
			break;
		case 'e':
			exposure->setValueCharArr (optarg);
			break;
		case 'c':
			if (nightCoolTemp == NULL)
				return rts2core::ScriptDevice::processOption (in_opt);
			nightCoolTemp->setValueCharArr (optarg);
			break;
		case OPT_WCS_MULTI:
			if (strlen (optarg) != 1 || optarg[0] < 'A' || optarg[0] > 'Z')
			{
				std::cerr << "invalid multiple WCS - only letters are allowed" << std::endl;
				return -1;
			}
			multi_wcs = optarg[0];
			break;
		case OPT_WCS_CDELT:
			// create WCS parameters..
			if (wcs_cdelta1 == NULL)
			{
				createValue (wcs_ctype1, multiWCS ("CTYPE1", multi_wcs), "WCS transformation type", false, RTS2_DT_WCS_MASK);
				createValue (wcs_ctype2, multiWCS ("CTYPE2", multi_wcs), "WCS transformation type", false, RTS2_DT_WCS_MASK);
				wcs_ctype1->setValueCharArr ("RA---TAN");
				wcs_ctype2->setValueCharArr ("DEC--TAN");
				createValue (wcs_crpix1, multiWCS ("CRPIX1", multi_wcs), "WCS x reference pixel", false, RTS2_VALUE_WRITABLE | RTS2_DT_WCS_CRPIX1);
				createValue (wcs_crpix2, multiWCS ("CRPIX2", multi_wcs), "WCS y reference pixel", false, RTS2_VALUE_WRITABLE | RTS2_DT_WCS_CRPIX2);

				createValue (wcs_cdelta1, multiWCS ("CDELT1", multi_wcs), "[deg] WCS delta along 1st axis", false, RTS2_VALUE_WRITABLE | RTS2_DT_WCS_CDELT1);
				createValue (wcs_cdelta2, multiWCS ("CDELT2", multi_wcs), "[deg] WCS delta along 2nd axis", false, RTS2_VALUE_WRITABLE | RTS2_DT_WCS_CDELT2);
				createValue (wcs_crota, multiWCS ("CROTA2", multi_wcs), "[deg] WCS rotation", false, RTS2_VALUE_WRITABLE | RTS2_DT_WCS_ROTANG);

				createValue (wcs_aux, "WCSAUX", "suffixes of auxiliary WCS to add", false, RTS2_VALUE_WRITABLE);
			}
			params = SplitStr (optarg, ":");
			if (params.size () != 5)
			{
				std::cerr << "cannot parse --wcs parameter " << optarg << std::endl;
				return -1;
			}

			int i;

			for (i = 0; i < 2; i++)
				default_crpix[i] = atof (params[i].c_str ());
			wcs_crpix1->setValueDouble (default_crpix[0]);
			wcs_crpix2->setValueDouble (default_crpix[1]);

			for (i = 2; i < 5; i++)
				default_cd[i - 2] = atof (params[i].c_str ());

			wcs_cdelta1->setValueDouble (default_cd[0]);
			wcs_cdelta2->setValueDouble (default_cd[1]);
			wcs_crota->setValueDouble (default_cd[2]);
			break;
		case OPT_WCS_AUXS:
			{
				if (wcs_aux == NULL)
				{
					std::cerr << "Cannot specify auxiliary WCS without camera WCSs values! Please change order of options so --wcs-aux is aafter --wcs." << std::endl;
					return -1;
				}
				std::vector <std::string> cwcs = SplitStr (std::string (optarg), ":");
				wcs_aux->setValueArray (cwcs);
			}
			break;
		case OPT_WITHSHM:
			// autoscale
			if (optarg == NULL)
				sharedMemNum = 0;
			else
				sharedMemNum = atoi (optarg);
			break;

		case OPT_DETSIZE:
			{
				createValue (detsize, "DETSIZE", "[x w y h] total detector size", false, RTS2_VALUE_FLOAT);
				if (detsize->setValueCharArr (optarg))
				{
					std::cerr << "cannot parse --detsize argument: " << optarg << ", exiting." << std::endl;
					return -1;
				}
			}
			break;
		case OPT_TRIMS_XY:
			createValue (trimx, "TRIM_X1", "[X11 X12 X13 ..] channel X trimmed data start", false);
			createValue (trimy, "TRIM_Y1", "[Y11 Y12 Y13 ..] channel Y trimmed data start", false);
			fillPairs (trimx, trimy, optarg);
			break;
		case OPT_TRIMS_END:
			createValue (trimx2, "TRIM_X2", "[X22 X22 X23 ..] channel X trimmed data end", false);
			createValue (trimy2, "TRIM_Y2", "[Y21 Y22 Y23 ..] channel Y trimmed data end", false);
			fillPairs (trimx2, trimy2, optarg);
			break;
		case OPT_CHANNELS_STARTS:
			createValue (chan1offset, "CHAN1_OFFSETS", "[X1 X2 X3 ..] channels X offsets", false);
			createValue (chan2offset, "CHAN2_OFFSETS", "[Y1 Y2 Y3 ..] channels Y offsets", false);
			fillPairs (chan1offset, chan2offset, optarg);
			break;
		case OPT_CHANNELS_DELTAS:
			createValue (chan1delta, "CHAN1_DELTA", "[X1 X2 X3 ..] channels X deltas", false);
			createValue (chan2delta, "CHAN2_DELTA", "[Y1 Y2 Y3 ..] channels Y deltas", false);
			fillPairs (chan1delta, chan2delta, optarg);
			break;
		default:
			return rts2core::ScriptDevice::processOption (in_opt);
	}
	return 0;
}

void Camera::usage ()
{
	std::cout << "Add multiple --wheeldev options to specify multiple filter wheels:" << std::endl
		<< "  " << getAppName () << " --wheldev W0 --wheeldev W1" << std::endl;
}

int Camera::initChips ()
{
	return 0;
}

void Camera::changeAxisDirections (bool x_orig, bool y_orig)
{
	if (wcs_cdelta1 == NULL || wcs_cdelta2 == NULL)
		return;

	wcs_cdelta1->setValueDouble ((x_orig ? 1 : -1) * default_cd[0] * binningHorizontal ());
	wcs_cdelta2->setValueDouble ((y_orig ? 1 : -1) * default_cd[1] * binningVertical ());

	sendValueAll (wcs_cdelta1);
	sendValueAll (wcs_cdelta2);
}

int Camera::sendImage (char *data, size_t dataSize)
{
	if (!exposureConn)
		return -1;
	if (calculateStatistics->getValueInteger () != STATISTIC_ONLY)
	{
		if (realTimeDataTransferCount != 0)
			startImageData (exposureConn);
		if (realTimeDataTransferCount >= 0)
			realTimeDataTransferCount++;
		if (currentImageData == -1)
			return -1;
	}
	sendFirstLine (0, 0);
	return sendReadoutData (data, dataSize);
}

int Camera::sendReadoutData (char *data, size_t dataSize, int chan)
{
	std::cerr << "Camera::sendReadoutData " << dataSize << " chan " << chan << " exposureConn " << exposureConn << std::endl;
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
		average->setValueDouble (sum->getValueDouble () / computedPix->getValueLong ());

		// find maximal mode value
		if (modeCount != NULL)
		{
			uint32_t modeNum = 0;
			for (unsigned int i = 0; i < modeCountSize; i++)
			{
				if (modeCount[i] > modeNum)
				{
					image_mode->setValueInteger (i);
					modeNum = modeCount[i];
				}
			}
			sendValueAll (image_mode);
		}

		sendValueAll (average);
		sendValueAll (max);
		sendValueAll (min);
		sendValueAll (sum);
		sendValueAll (computedPix);
	}
	else
	{
		computedPix->setValueLong (computedPix->getValueLong () + dataSize / usedPixelByteSize ());
		sendValueAll (computedPix);
	}

	// will update only if some data still need to be transfered
	updateReadoutSpeed (computedPix->getValueLong ());

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

	if (currentImageTransfer == SHARED)
		sharedData->dataWritten (chan, dataSize);

	dataWritten[chan] += dataSize;

	if (exposureConn && currentImageTransfer == TCPIP)
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


const int Camera::maxPixelByteSize ()
{
	int ms = 0;
	for (std::vector < rts2core::SelVal >::iterator iter = dataType->selBegin (); iter != dataType->selEnd (); iter++)
	{
		if (((DataType *) iter->data)->type == RTS2_DATA_ULONG && ms < 4)
		{
			ms = 4;
			continue;
		}
		int ps = ((DataType *) iter->data)->type / 8;
		if (ps > ms)
			ms = ps;
	}
	return ms;
}

int Camera::initValues ()
{
	for (int i = 0; i < histories; i++)
	{
		rts2core::ValueString *val;
		std::ostringstream os;
		os << "hist_" << i;
		createValue (val, os.str ().c_str (), "history", true, RTS2_VALUE_WRITABLE | RTS2_DT_HISTORY);
	}
	for (int i = 0; i < comments; i++)
	{
		rts2core::ValueString *val;
		std::ostringstream os;
		os << "comm_" << i;
		createValue (val, os.str ().c_str (), "comment", true, RTS2_VALUE_WRITABLE | RTS2_DT_COMMENT);
	}
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
			camFilterVals.push_back (FilterVal (this, *iter, fil));
		}
	}

	addConstValue ("chips", 1);

	initBinnings ();
	initDataTypes ();

	// init shared memory segment
	if (sharedMemNum >= 0)
	{
		size_t dataBufferSize = getWidth () * getHeight () * maxPixelByteSize ();
		// autoscale shared memory
		if (sharedMemNum == 0)
		{
#ifdef __linux__
			// read limit
			std::ifstream ifs ("/proc/sys/kernel/shmmax");
			long long shmax;
			ifs >> shmax;
			ifs.close ();
			if (ifs.fail ())
			{
				logStream (MESSAGE_ERROR) << "cannot read maximal shared memory size from /proc/sys/kernel/shmmax" << sendLog;
				sharedMemNum = 10;
			}
			else
			{
				double d = sizeof (struct rts2core::SharedDataHeader);
				d *= d;
				size_t a = sizeof (struct rts2core::SharedDataSegment) + dataBufferSize;
				d += 4 * a * shmax;
				d = (sqrt (d) - sizeof (struct rts2core::SharedDataHeader)) / (2 * a);
				if (d > 1000)
				{
					sharedMemNum = 1000;
				}
				else if (d < 0)
				{
					logStream (MESSAGE_ERROR) << "invalid root of quadratic equation for maximal shared memory size " << d << sendLog;
					return -1;
				}
				else
				{
					sharedMemNum = d;
					if (sharedMemNum == 0)
					{
						logStream (MESSAGE_ERROR) << "shared memory maximal size is insuficient for a single image. Please increase the memory limit - see man shmget for details" << sendLog;
						return -1;
					}
				}
			}
#else
			logStream (MESSAGE_ERROR) << "unknow operating system, uses 10 for number of shared memory segments" << sendLog;
			sharedMemNum = 10;
#endif
		}
		sharedData = new rts2core::DataSharedWrite ();
		if (sharedData->create (sharedMemNum, dataBufferSize) == NULL)
		{
			return -1;
		}
		logStream (MESSAGE_DEBUG) << "creating shared memory with " << sharedMemNum << " segments" << sendLog;
	}
	fhd = new struct imghdr;
	focusingHeader = NULL;

	if (tempCCDHistory != NULL)
	{
		addTimer (tempCCDHistoryInterval->getValueInteger (), new rts2core::Event (EVENT_TEMP_CHECK));
	}

	dataBuffers = new char*[getNumChannels ()];
	memset (dataBuffers, 0, getNumChannels () * sizeof (char*));

	dataWritten = new size_t[getNumChannels ()];
	memset (dataWritten, 0, getNumChannels () * sizeof (size_t));

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
	addFilterOffsets ();
}

void Camera::addFilterOffsets ()
{
	if (filterOffsets.size () == 0)
		return;
	camFilterOffsets->clear ();
	for (std::vector < rts2core::SelVal >::iterator iter = camFilterVal->selBegin (); iter != camFilterVal->selEnd (); iter++)
	{
		std::map <std::string, double>::iterator fo = filterOffsets.find (iter->name);
		if (fo != filterOffsets.end ())
		{
			camFilterOffsets->addValue (fo->second);
		}
		else
		{
			camFilterOffsets->addValue (0);
		}
	}
	sendValueAll (camFilterOffsets);
}

void Camera::checkExposures ()
{
	long ret;
	if ((getStateChip (0) & (CAM_EXPOSING | CAM_EXPOSING_NOIM)))
	{
		// try to end exposure
		ret = isExposing ();
		if (ret >= 0)
		{
			setTimeout (ret);
		}
		else
		{
			int expNum;
			switch (ret)
			{
				case -5:
					endExposure (ret);
					maskState (CAM_MASK_EXPOSE | CAM_MASK_FT | BOP_TEL_MOVE, CAM_NOEXPOSURE | CAM_NOFT, "exposure finished", NAN, NAN, exposureConn);
					break;
				case -4:
					exposureConn = NULL;
					endExposure (ret);
					maskState (CAM_MASK_EXPOSE | CAM_MASK_FT | BOP_TEL_MOVE, CAM_NOEXPOSURE | CAM_NOFT, "exposure finished", NAN, NAN, exposureConn);
					if (quedExpNumber->getValueInteger () > 0)
						camExpose (exposureConn, getStateChip (0) & ~CAM_MASK_EXPOSE, true, lastCareBlock);
					break;
				case -3:
					exposureConn = NULL;
					endExposure (ret);
					break;
				case -2:
					// remember exposure number
					expNum = getExposureNumber ();
					endExposure (ret);
					// if new exposure does not start during endExposure (camReadout) call, drop exposure state
					if (expNum == getExposureNumber ())
						maskState (CAM_MASK_EXPOSE | CAM_MASK_FT | BOP_TEL_MOVE, CAM_NOEXPOSURE | CAM_NOFT, "exposure finished", NAN, NAN, exposureConn);

					// drop FT flag
					else
						maskState (CAM_MASK_FT, CAM_NOFT, "ft exposure chip finished", NAN, NAN, exposureConn);
					break;
				case -1:
					maskState (DEVICE_ERROR_MASK | CAM_MASK_EXPOSE | CAM_MASK_SHIFTING | CAM_MASK_READING | BOP_TEL_MOVE, DEVICE_ERROR_HW | CAM_NOEXPOSURE | CAM_NOTREADING, "exposure finished with error", NAN, NAN, exposureConn);
					stopExposure ();
					if (quedExpNumber->getValueInteger () > 0)
					{
						logStream (MESSAGE_DEBUG) << "starting new exposure after camera failure" << sendLog;
						camExpose (exposureConn, getStateChip (0) & ~CAM_MASK_EXPOSE, true, lastCareBlock);
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
			maskState (CAM_MASK_SHIFTING | CAM_MASK_READING | CAM_MASK_HAS_IMAGE, CAM_NOTREADING | CAM_HAS_IMAGE, "readout ended", NAN, NAN, exposureConn);
		else
			maskState (DEVICE_ERROR_MASK | CAM_MASK_SHIFTING | CAM_MASK_READING, DEVICE_ERROR_HW | CAM_NOTREADING, "readout ended with error", NAN, NAN, exposureConn);
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
		int ret;
		if (wheelDevices.size () > 0)
			ret = setFilterNum (new_value->getValueInteger (), wheelDevices[0]) == 0 ? 0 : -2;
		else
			ret = setFilterNum (new_value->getValueInteger ()) == 0 ? 0 : -2;
		if (ret == 0)
			offsetForFilter (new_value->getValueInteger (), camFilterVals.end ());
		return ret;
	}
	int i = 0;
	for (std::list <FilterVal>::iterator iter = camFilterVals.begin (); iter != camFilterVals.end (); iter++, i++)
	{
		if (iter->filter == old_value)
		{
			int ret = setFilterNum (new_value->getValueInteger (), wheelDevices[i]) == 0 ? 0 : -2;
			if (ret == 0)
				offsetForFilter (new_value->getValueInteger (), iter);
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
	if (old_value == coolingOnOff)
	{
		return switchCooling (((rts2core::ValueBool *) new_value)->getValueBool ()) == 0 ? 0 : -2;
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
	sendValueAll (tempCCD);
}

void Camera::deviceReady (rts2core::Connection * conn)
{
	// if that's filter wheel
	if (wheelDevices.size () > 0 && conn->getOtherDevClient ())
	{
		// find filter wheel for which device connected..
		std::list <FilterVal>::iterator fiter;
		std::vector <const char *>::iterator iter;
		for (fiter = camFilterVals.begin (), iter = wheelDevices.begin (); fiter != camFilterVals.end () && iter != wheelDevices.end (); fiter++, iter++)
		{
			if (!strcmp (conn->getName (), *iter))
			{
				// copy content of device filter variable to our list..
				rts2core::Value *val = conn->getValue ("filter");
				// it's filter and it's correct type
				if (val != NULL && val->getValueType () == RTS2_VALUE_SELECTION)
				{
					fiter->filter->duplicateSelVals ((rts2core::ValueSelection *) val);
					// sends filter metainformations to all connected devices
					updateMetaInformations (fiter->filter);
					if (iter == wheelDevices.begin ())
					{
						camFilterVal->duplicateSelVals ((rts2core::ValueSelection *) val);
						addFilterOffsets ();
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
			((FilterVal *) (event->getArg ()))->moving->setValueBool (false);
			if (!filterMoving ())
				checkQueuedExposures ();
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

void Camera::changeMasterState (rts2_status_t old_state, rts2_status_t new_state)
{
	switch (new_state & SERVERD_STATUS_MASK)
	{
		case SERVERD_EVENING:
		case SERVERD_DUSK:
		case SERVERD_NIGHT:
		case SERVERD_DAWN:
		case SERVERD_MORNING:
			{
				switch (new_state & SERVERD_ONOFF_MASK)
				{
					case SERVERD_ON:
					case SERVERD_STANDBY:
						beforeNight ();
						break;
					// otherwise, don't forgot to un-cool CCDs
					default:
						afterNight ();
				}
				break;
			}
		default:
			afterNight ();
	}
	rts2core::ScriptDevice::changeMasterState (old_state, new_state);
}

void Camera::findSepStars (uint16_t *data)
{
	if (sepFind->getValueBool () == false)
		return;

	sepX->clear ();
	sepY->clear ();
	sepFluxes->clear ();

	sep_image im = {data, NULL, NULL, SEP_TUINT16, 0, 0, getUsedWidthBinned (), getUsedHeightBinned (), 0.0, SEP_NOISE_NONE, 1.0, 0.0};
	sep_bkg *bkg = NULL;
	int status = sep_background (&im, 64, 64, 3, 3, 0.0, &bkg);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "SEP: unable to estimate background:" << status << sendLog;
		return;
	}

	uint16_t *imback = (uint16_t *) malloc (getUsedWidthBinned () * getUsedHeightBinned () * sizeof (uint16_t));
	status = sep_bkg_array (bkg, imback, SEP_TUINT16);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "SEP: cannot construct background array:" << status << sendLog;
		return;
	}

	status = sep_bkg_subarray (bkg, im.data, im.dtype);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "SEP: cannot subtract background:" << status << sendLog;
		return;
	}

	float conv[] = {1,2,1, 2,4,2, 1,2,1};
	sep_catalog *catalog = NULL;

	im.noise = &(bkg->globalrms);  /* set image noise level */

	status = sep_extract(&im, 1.5*bkg->globalrms, SEP_THRESH_REL, 5, conv, 3, 3, SEP_FILTER_CONV, 32, 0.005, 1, 1.0, &catalog);
	if (status)
	{
		logStream (MESSAGE_ERROR) << "SEP: cannot extract sources:" << status << sendLog;
		return;
	}

	/* aperture photometry */
	double *flux, *fluxerr, *fluxt, *fluxerrt, *area, *areat;
	short *flag, *flagt;

	im.noise = &(bkg->globalrms);  /* set image noise level */
	im.ndtype = SEP_TUINT16;
	fluxt = flux = (double *)malloc(catalog->nobj * sizeof(double));
	fluxerrt = fluxerr = (double *)malloc(catalog->nobj * sizeof(double));
	areat = area = (double *)malloc(catalog->nobj * sizeof(double));
	flagt = flag = (short *)malloc(catalog->nobj * sizeof(short));
	for (int i=0; i<catalog->nobj; i++, fluxt++, fluxerrt++, flagt++, areat++)
		sep_sum_circle(&im, catalog->x[i], catalog->y[i], 5.0, 5, 0, fluxt, fluxerrt, areat, flagt);


}

int Camera::camStartExposure (bool careBlock)
{
	// check if we aren't blocked
	// we can allow this test as camStartExposure is called only after quedExpNumber was decreased
	bool fm = filterMoving ();

	if (careBlock == true
		&& (!expType || expType->getValueInteger () == 0)
		&& (
			(getDeviceBopState () & BOP_EXPOSURE)
			|| (getMasterStateFull () & BOP_EXPOSURE)
			|| (getDeviceBopState () & BOP_TRIG_EXPOSE)
			|| (getMasterStateFull () & BOP_TRIG_EXPOSE)
			|| fm
			|| (focuserMoving && focuserMoving->getValueBool ())
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

		logStream (MESSAGE_DEBUG) << "should ask for exposure block " << (getDeviceBopState () & BOP_EXPOSURE) << " " << (getMasterStateFull () & BOP_EXPOSURE) << " " << (getDeviceBopState () & BOP_TRIG_EXPOSE) << " " << (getMasterStateFull () & BOP_TRIG_EXPOSE) << " " << fm << " " << focuserMoving->getValueBool () << sendLog;

		if (!((getDeviceBopState () & BOP_EXPOSURE) || (getMasterStateFull () & BOP_EXPOSURE)) && ((getDeviceBopState () & BOP_TRIG_EXPOSE) || (getMasterStateFull () & BOP_TRIG_EXPOSE)))
			maskState (BOP_WILL_EXPOSE, BOP_WILL_EXPOSE, "device plan to exposure soon", NAN, NAN, exposureConn);

		return 0;
	}

	if (expType && expType->getValueInteger () != 0 && ((getDeviceBopState () & BOP_TRIG_EXPOSE) || (getMasterStateFull () & BOP_TRIG_EXPOSE)))
		maskState (BOP_WILL_EXPOSE, BOP_WILL_EXPOSE, "device plan to exposure soon", NAN, NAN, exposureConn);

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

	if (needReload->getValueBool ())
	{
		Binning2D *bin = (Binning2D *) binning->getData ();
		setBinning (bin->horizontal, bin->vertical);
		needReload->setValueBool (false);
	}

	// Fill the image header while we have all the actual values used to start exposure
	fhd->data_type = htons (getDataType ());
	fhd->naxes = 2;
	fhd->sizes[0] = htonl (chipUsedReadout->getWidthInt () / binningHorizontal ());
	fhd->sizes[1] = htonl (chipUsedReadout->getHeightInt () / binningVertical ());
	fhd->binnings[0] = htons (binningVertical ());
	fhd->binnings[1] = htons (binningHorizontal ());
	fhd->x = htons (chipUsedReadout->getXInt ());
	fhd->y = htons (chipUsedReadout->getYInt ());
	fhd->filter = htons (getFilterNum ());

	// Cache frame size in pixels and bytes so that the readout may use these actual values, and not
	// the ones at readout start (as it may be after current script end + reset of parameters)
	lastExposurePixels = getUsedHeightBinned () * getUsedWidthBinned ();
	lastExposureBytes = chipByteSize ();

	if (expType)
	{
		fhd->shutter = htons (expType->getValueInteger ());
		imageType->setValueInteger (expType->getValueInteger () == 0 ? 0 : (exposure->getValueFloat () > 0 ? 1 : 2));
		sendValueAll (imageType);
	}
	else
		fhd->shutter = 0;

	sepX->clear ();
	sepY->clear ();
	sepFluxes->clear ();

	maskState (BOP_TEL_MOVE, BOP_TEL_MOVE, "exposure is about to start", NAN, NAN, exposureConn);

	ret = startExposure ();
	if (!(ret == 0 || ret == 1))
	{
		maskState (BOP_TEL_MOVE, 0, "exposure did not start", NAN, NAN, exposureConn);
		return ret;
	}

	double now = getNow ();

	exposureEnd->setValueDouble (now + ( acquireTime ? acquireTime->getValueDouble () : exposure->getValueDouble () ));

	infoAll ();

	maskState (CAM_MASK_EXPOSE | BOP_TEL_MOVE | BOP_WILL_EXPOSE | CAM_MASK_HAS_IMAGE, (ret == 0 ? (CAM_EXPOSING) : (CAM_EXPOSING | CAM_EXPOSING_NOIM)) | BOP_TEL_MOVE, "exposure started", now, exposureEnd->getValueDouble (), exposureConn);
	if (ret == 0)
		logStream (MESSAGE_INFO) << "starting " << TimeDiff (exposure->getValueFloat ()) << (ret == 1 ? "no image " : "") << " exposure for '" << (exposureConn ? exposureConn->getName () : "null") << "'" << sendLog;
	else
		logStream (MESSAGE_INFO) << "starting " << TimeDiff (exposure->getValueFloat ()) << " exposure cycle (without image) for '" << (exposureConn ? exposureConn->getName () : "null") << "'" << sendLog;

	lastFilterNum = getFilterNum ();
	// call us to check for exposures..
	long new_timeout;
	new_timeout = isExposing ();
	if (new_timeout >= 0)
	{
		setTimeout (new_timeout);
	}

	// check if that comes from old request
	if (sendOkInExposure && exposureConn)
	{
		sendOkInExposure = false;
		exposureConn->sendCommandEnd (DEVDEM_OK, "executing exposure from queue");
	}

	return 0;
}

int Camera::camExpose (rts2core::Connection * conn, int chipState, bool fromQue, bool careBlock)
{
	int ret;

	// if it is currently exposing
	// or performing other op that can block command execution
	// or there are queued values which needs to be dealed before we can start exposing
	if ((chipState & CAM_EXPOSING)
	  	|| (chipState & CAM_EXPOSING_NOIM)
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

	ret = camStartExposure (careBlock);
	if (ret)
	{
		conn->sendCommandEnd (DEVDEM_E_HW, "cannot exposure on chip");
	}

	return ret;
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
	timeTransferStart = getNow ();
	// if we can do exposure, do it..
	if (quedExpNumber->getValueInteger () > 0 && exposureConn && supportFrameTransfer ())
	{
		checkQueChanges (getStateChip (0) & ~ (CAM_EXPOSING | CAM_EXPOSING_NOIM));
		maskState (CAM_MASK_READING | CAM_MASK_FT, CAM_READING | CAM_FT, "starting frame transfer", NAN, NAN, exposureConn);
		if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
			currentImageData = -1;
		else if (realTimeDataTransferCount < 0)
			startImageData (conn);
		if (queValues.empty ())
			// remove exposure flag from state
			camExpose (exposureConn, getStateChip (0) & ~CAM_MASK_EXPOSE, true, lastCareBlock);
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
		else if (realTimeDataTransferCount < 0)
			startImageData (conn);
	}

	if (calculateStatistics->getValueInteger () == STATISTIC_ONLY)
		calculateDataSize = lastExposureChipByteSize ();

	memset (dataWritten, 0, getNumChannels () * sizeof (size_t));

	if (realTimeDataTransferCount >= 0)
	{
		realTimeDataTransferCount++;
		return 0;
	}

	if (currentImageData != -1 || currentImageTransfer == FITS || calculateStatistics->getValueInteger () == STATISTIC_ONLY)
	{
		readoutPixels = lastExposurePixels;
		if (std::isnan (timeReadoutStart))
			timeReadoutStart = getNow ();
		return readoutStart ();
	}

	maskState (DEVICE_ERROR_MASK | CAM_MASK_SHIFTING | CAM_MASK_READING, DEVICE_ERROR_HW | CAM_NOTREADING, "readout failed", NAN, NAN, exposureConn);
	conn->sendCommandEnd (DEVDEM_E_HW, "cannot read chip");
	return -1;
}

int Camera::shiftStoreStart (rts2core::Connection *conn, float exptime)
{
	shiftstoreLines->clear ();
	sendValueAll (shiftstoreLines);
	maskState (DEVICE_ERROR_MASK | CAM_MASK_EXPOSE | CAM_MASK_SHIFTING, CAM_EXPOSING | CAM_SHIFT, "starting shift-store", NAN, NAN, conn);
	return 0;
}

int Camera::shiftStoreShift (rts2core::Connection *conn, int shift, float exptime)
{
	shiftstoreLines->addValue (shift);
	sendValueAll (shiftstoreLines);
	maskState (DEVICE_ERROR_MASK | CAM_MASK_EXPOSE, CAM_EXPOSING, "shifting shift-store", NAN, NAN, conn);
	return 0;
}

int Camera::shiftStoreEnd (rts2core::Connection *conn, int shift, float exptime)
{
	shiftstoreLines->addValue (shift);
	sendValueAll (shiftstoreLines);
	exposureConn = conn;
	maskState (DEVICE_ERROR_MASK | CAM_MASK_EXPOSE | CAM_MASK_READING, CAM_READING, "end shift-store", NAN, NAN, conn);
	return 0;
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
		if (fs.filter == -1)
		// filter move will be performed
		{
			ret = 0;
		}
		else
		{
			ret = -1;
		}
	}
	return ret;
}

void Camera::offsetForFilter (int new_filter, std::list <FilterVal>::iterator fvi)
{
	if (!focuserDevice || useFilterOffsets->getValueBool () == false)
		return;
	struct focuserMove fm;
	if (fvi == camFilterVals.end ())
	{
		if ((size_t) new_filter >= camFilterOffsets->size ())
			return;
		fm.value = (*camFilterOffsets)[new_filter];
	}
	else
	{
		if ((size_t) new_filter >= fvi->offsets->size ())
			return;
		fm.value = (*(fvi->offsets))[new_filter];
	}
	fm.focuserName = focuserDevice;
	fm.conn = this;
	postEvent (new rts2core::Event (EVENT_FOCUSER_FILTEROFFSET, (void *) &fm));
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
		conn->sendMsg ("expose - start exposition");
		conn->sendMsg ("stopexpo <chip> - stop exposition on given chip");
		conn->sendMsg ("stopread <chip> - stop reading given chip");
		conn->sendMsg ("exit - exit from connection");
		conn->sendMsg ("help - print, what you are reading just now");
		return 0;
	}
	else if (conn->isCommand (COMMAND_CCD_EXPOSURE))
	{
		if (!conn->paramEnd ())
			return -2;
		lastCareBlock = true;
		return camExpose (conn, getStateChip (0), false, true);
	}
	else if (conn->isCommand (COMMAND_CCD_EXPOSURE_NO_CHECKS))
	{
		if (!conn->paramEnd ())
			return -2;
		lastCareBlock = false;
		return camExpose (conn, getStateChip (0), false, false);
	}
	else if (conn->isCommand (COMMAND_CCD_SHIFTSTORE))
	{
		// not supported
		if (shiftstoreLines == NULL)
			return -2;
		char *kind;
		int shift;
		float exptime;
		if (conn->paramNextString (&kind))
			return -2;
		if (strcmp (kind, "start") == 0)
		{
			if (conn->paramNextFloat (&exptime) || !conn->paramEnd ())
				return -2;
			return shiftStoreStart (conn, exptime);
		}
		else if (strcmp (kind, "shift") == 0)
		{
			if (conn->paramNextInteger (&shift) || conn->paramNextFloat (&exptime) || !conn->paramEnd ())
				return -2;
			return shiftStoreShift (conn, shift, exptime);
		}
		else if (strcmp (kind, "end") == 0)
		{
			if (conn->paramNextInteger (&shift) || conn->paramNextFloat (&exptime) || !conn->paramEnd ())
				return -2;
			return shiftStoreEnd (conn, shift, exptime);
		}
		return -2;
	}
	else if (conn->isCommand ("stopexpo"))
	{
		if (!conn->paramEnd ())
			return -2;
		int ret = stopExposure ();
		if (ret)
		{
			maskState (CAM_MASK_EXPOSE | CAM_MASK_SHIFTING | CAM_MASK_READING | CAM_MASK_FT | BOP_TEL_MOVE | BOP_WILL_EXPOSE | DEVICE_ERROR_KILL, CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT | DEVICE_ERROR_KILL, "chip exposure interrupted", NAN, NAN, exposureConn);
		}
		else
		{
			maskState (CAM_MASK_EXPOSE | CAM_MASK_SHIFTING | CAM_MASK_READING | CAM_MASK_FT | BOP_TEL_MOVE | BOP_WILL_EXPOSE | DEVICE_ERROR_KILL | DEVICE_ERROR_HW, CAM_NOEXPOSURE | CAM_NOTREADING | CAM_NOFT | DEVICE_ERROR_KILL | DEVICE_ERROR_HW, "chip exposure interrupted with error", NAN, NAN, exposureConn);
		}
		return 0;
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
	else if (conn->isCommand ("clear_center"))
	{
		if (!conn->paramEnd ())
			return -2;
		centerStat->clearStat ();
		sendValueAll (centerStat);

		centerAvgStat->clearStat ();
		sendValueAll (centerAvgStat);
		return 0;
	}
	else if (conn->isCommand (COMMAND_FITS_STAT))
	{
		double p_average, p_min, p_max, p_sum, p_mode;
		if (conn->paramNextDouble (&p_average)
			|| conn->paramNextDouble (&p_min)
			|| conn->paramNextDouble (&p_max)
			|| conn->paramNextDouble (&p_sum)
			|| conn->paramNextDouble (&p_mode)
			|| !conn->paramEnd ())
			return -2;
		average->setValueDouble (p_average);
		min->setValueDouble (p_min);
		max->setValueDouble (p_max);
		sum->setValueDouble (p_sum);
		image_mode->setValueDouble (p_mode);

		sendValueAll (average);
		sendValueAll (min);
		sendValueAll (max);
		sendValueAll (sum);
		sendValueAll (image_mode);
		return 0;
	}
	return rts2core::ScriptDevice::commandAuthorized (conn);
}

int Camera::maskQueValueBopState (rts2_status_t new_state, int valueQueCondition)
{
	if (valueQueCondition & CAM_EXPOSING)
		new_state |= BOP_EXPOSURE;
	if (valueQueCondition & CAM_READING)
		new_state |= BOP_READOUT;
	return new_state;
}

void Camera::setFullBopState (rts2_status_t new_state)
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
			// do not start if already exposing
			if ((getStateChip (0) & CAM_EXPOSING) || (!supportFrameTransfer () && (getStateChip (0) & CAM_READING)))
			{
				logStream (MESSAGE_DEBUG) << "ignore state change, as camera is exposing (state " << getStateChip (0) << sendLog;
				return;
			}
			waitingForNotBop->setValueBool (false);
			quedExpNumber->dec ();
			camStartExposureWithoutCheck ();
		}
	}
}

char* Camera::getDataBuffer (int chan)
{
	if (currentImageTransfer == SHARED)
		return ((char *) sharedData->getChannelData (chan)) + sizeof (imghdr);
	// if dataBuffesr is null, allocate it
	if (dataBuffers[chan] == NULL && suggestBufferSize () > 0)
		dataBuffers[chan] = new char[getHeight () * getWidth () * maxPixelByteSize ()];
	return dataBuffers[chan];
}

char* Camera::getDataTop (int chan)
{
	return getDataBuffer (chan) + dataWritten[chan];
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

bool Camera::filterMoving ()
{
	for (std::list <FilterVal>::iterator iter = camFilterVals.begin (); iter != camFilterVals.end (); iter++)
	{
		if (iter->moving->getValueBool ())
			return true;
	}
	return false;
}

void Camera::setFilterOffsetFile (const char *filename)
{
	filterOffsets.clear ();
	std::ifstream ifs (filename);
	int ln = 1;
	while (!ifs.fail ())
	{
		std::string line;
		std::getline (ifs, line);
		if (ifs.fail ())
			break;
		std::string fn;
		double fo;
		if (line[0] == '#')
			continue;
		std::istringstream iifs (line);
		iifs >> fn >> fo;
		if (iifs.fail ())
		{
			std::ostringstream erros;
			erros << "cannot parse filter offsets file " << filename << " on line " << ln;
			throw rts2core::Error (erros.str ());
		}
		std::map <std::string, double>::iterator fit = filterOffsets.find (fn);
		if (fit != filterOffsets.end ())
			throw rts2core::Error (std::string ("multiple lines for filter ") + fn);
		filterOffsets[fn] = fo;
		ln++;
	}

	if (!ifs.eof ())
	{
		throw rts2core::Error (std::string ("cannot parse filter file ") + strerror (errno));
	}


	filterOffsetFile->setValueCharArr (filename);
}
