/**
 * Script for acqustion of star.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
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

#include "connimgprocess.h"
#include "elementacquire.h"

#include "../rts2fits/devclifoc.h"
#include "rts2config.h"

using namespace rts2plan;
using namespace rts2script;
using namespace rts2image;

ElementAcquire::ElementAcquire (Script * in_script, double in_precision, float in_expTime, struct ln_equ_posn *in_center_pos):Element (in_script)
{
	reqPrecision = in_precision;
	lastPrecision = rts2_nan ("f");
	expTime = in_expTime;
	processingState = NEED_IMAGE;
	Rts2Config::instance ()->getString ("imgproc", "astrometry",
		defaultImgProccess);
	obsId = -1;
	imgId = -1;

	if (in_center_pos)
	{
		center_pos.ra = in_center_pos->ra;
		center_pos.dec = in_center_pos->dec;
	}
	else
	{
		center_pos.ra = 0;
		center_pos.dec = 0;
	}
}

void ElementAcquire::postEvent (Rts2Event * event)
{
	Image *image;
	AcquireQuery *ac;
	switch (event->getType ())
	{
		case EVENT_OK_ASTROMETRY:
			image = (Image *) event->getArg ();
			if (image->getObsId () == obsId && image->getImgId () == imgId)
			{
				// we get below required errror?
				double img_prec = image->getPrecision ();
				if (isnan (img_prec))
				{
					processingState = FAILED;
					break;
				}
				if (img_prec <= reqPrecision)
				{
				#ifdef DEBUG_EXTRA
					logStream (MESSAGE_DEBUG)
						<<
						"ElementAcquire::postEvent seting PRECISION_OK on "
						<< img_prec << " <= " << reqPrecision << " obsId " << obsId <<
						" imgId " << imgId << sendLog;
				#endif
					processingState = PRECISION_OK;
				}
				else
				{
					// test if precision is better then previous one..
					if (isnan (lastPrecision) || img_prec < lastPrecision / 2)
					{
						processingState = PRECISION_BAD;
						lastPrecision = img_prec;
					}
					else
					{
						processingState = FAILED;
					}
				}
			}
			break;
		case EVENT_NOT_ASTROMETRY:
			image = (Image *) event->getArg ();
			if (image->getObsId () == obsId && image->getImgId () == imgId)
			{
				processingState = FAILED;
			}
			break;
		case EVENT_ACQUIRE_QUERY:
			ac = (AcquireQuery *) event->getArg ();
			ac->count++;
			break;
	}
	Element::postEvent (event);
}

int ElementAcquire::nextCommand (rts2core::DevClientCamera * camera, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	// this code have to coordinate efforts to reach desired target with given precission
	// based on internal state, it have to take exposure, assure that image will be processed,
	// wait till astrometry ended, based on astrometry results decide if to
	// proceed with acqusition or if to cancel acqusition
	switch (processingState)
	{
		case NEED_IMAGE:
			if (isnan (lastPrecision))
			{
				bool en = false;
				script->getMaster ()->postEvent (new Rts2Event (EVENT_QUICK_ENABLE, (void *) &en));
			}

			camera->getConnection ()->queCommand (new rts2core::CommandChangeValue (camera, "SHUTTER", '=', 0));
			// change values of the exposur
			camera->getConnection ()->queCommand (new rts2core::CommandChangeValue (camera, "exposure", '=', expTime));

			*new_command = new rts2core::CommandExposure (script->getMaster (), camera, BOP_EXPOSURE);
			getDevice (new_device);
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG) <<
				"ElementAcquire::nextCommand WAITING_IMAGE this " << this <<
				sendLog;
		#endif					 /* DEBUG_EXTRA */
			processingState = WAITING_IMAGE;
			return NEXT_COMMAND_ACQUSITION_IMAGE;
		case WAITING_IMAGE:
		case WAITING_ASTROMETRY:
			return NEXT_COMMAND_WAITING;
		case FAILED:
			return NEXT_COMMAND_PRECISION_FAILED;
		case PRECISION_OK:
		#ifdef DEBUG_EXTRA
			logStream (MESSAGE_DEBUG)
				<< "ElementAcquire::nextCommand PRECISION_OK" << sendLog;
		#endif
			return NEXT_COMMAND_PRECISION_OK;
		case PRECISION_BAD:
			processingState = NEED_IMAGE;
			// end of movemen will call nextCommand, as we should have waiting set to WAIT_MOVE
			return NEXT_COMMAND_RESYNC;
		default:
			break;
	}
	// that should not happen!
	logStream (MESSAGE_ERROR)
		<< "ElementAcquire::nextCommand unexpected processing state "
		<< (int) processingState << sendLog;
	return NEXT_COMMAND_NEXT;
}

int ElementAcquire::processImage (Image * image)
{
	int ret;
	ConnImgProcess *processor;

	if (processingState != WAITING_IMAGE || !image->getIsAcquiring ())
	{
		logStream (MESSAGE_ERROR)
			<< "ElementAcquire::processImage invalid processingState: "
			<< (int) processingState << " isAcquiring: " << image->
			getIsAcquiring () << " this " << this << sendLog;
		return -1;
	}
	obsId = image->getObsId ();
	imgId = image->getImgId ();
	processor = new ConnImgProcess (script->getMaster (), defaultImgProccess.c_str (), image->getFileName (), Rts2Config::instance ()->getAstrometryTimeout ());
	// save image before processing..
	image->saveImage ();
	ret = processor->init ();
	if (ret < 0)
	{
		delete processor;
		processor = NULL;
		processingState = FAILED;
	}
	else
	{
		script->getMaster ()->addConnection (processor);
		processingState = WAITING_ASTROMETRY;
		script->getMaster ()->postEvent (new Rts2Event (EVENT_ACQUIRE_WAIT));
	}
	return 0;
}

void ElementAcquire::cancelCommands ()
{
	processingState = FAILED;
}

double ElementAcquire::getExpectedDuration ()
{
	// conservative estimate: 3 images, 5 minutes to run astrometry on them
	return (expTime + script->getFullReadoutTime ()) * 3 + 300;
}

ElementAcquireStar::ElementAcquireStar (Script * in_script, int in_maxRetries, double in_precision, float in_expTime, double in_spiral_scale_ra, double in_spiral_scale_dec, struct ln_equ_posn *in_center_pos):ElementAcquire (in_script, in_precision, in_expTime, in_center_pos)
{
	maxRetries = in_maxRetries;
	retries = 0;
	spiral_scale_ra = in_spiral_scale_ra;
	spiral_scale_dec = in_spiral_scale_dec;
	minFlux = 5000;

	Rts2Config::instance ()->getString (in_script->getDefaultDevice (),
		"sextractor", defaultImgProccess);

	Rts2Config::instance ()->getDeviceMinFlux (in_script->getDefaultDevice (),
		minFlux);
	spiral = new Rts2Spiral ();
}

ElementAcquireStar::~ElementAcquireStar (void)
{
	delete spiral;
}

void ElementAcquireStar::postEvent (Rts2Event * event)
{
	ConnFocus *focConn;
	Image *image;
	struct ln_equ_posn offset;
	int ret;
	short next_x, next_y;
	switch (event->getType ())
	{
		case EVENT_STAR_DATA:
			focConn = (ConnFocus *) event->getArg ();
			image = focConn->getImage ();
			// that was our image
			if (image->getObsId () == obsId && image->getImgId () == imgId)
			{
				ret = getSource (image, offset.ra, offset.dec);
				switch (ret)
				{
					case -1:
						logStream (MESSAGE_DEBUG)
							<<
							"ElementAcquireStar::postEvent EVENT_STAR_DATA failed (numStars: "
							<< image->sexResultNum << ")" << sendLog;
						retries++;
						if (retries >= maxRetries)
						{
							processingState = FAILED;
							break;
						}
						processingState = PRECISION_BAD;
						// try some offset..
						spiral->getNextStep (next_x, next_y);
						// change spiral RA, which are in planar deg, to sphere deg
						if (fabs (center_pos.dec) < 89)
						{
							offset.ra =
								(spiral_scale_ra * next_x) /
								cos (ln_deg_to_rad (center_pos.dec));
						}
						offset.dec = spiral_scale_dec * next_y;
						script->getMaster ()->
							postEvent (new
							Rts2Event (EVENT_ADD_FIXED_OFFSET,
							(void *) &offset));
						break;
					case 0:
						logStream (MESSAGE_DEBUG)
							<< "ElementAcquireStar::offsets ra: "
							<< offset.ra << " dec: " << offset.dec << " OK" << sendLog;
						processingState = PRECISION_OK;
						break;
					case 1:
						logStream (MESSAGE_DEBUG)
							<< "ElementAcquireStar::offsets ra: "
							<< offset.ra << " dec: " << offset.
							dec << " failed" << sendLog;
						retries++;
						if (retries >= maxRetries)
						{
							processingState = FAILED;
							break;
						}
						processingState = PRECISION_BAD;
						script->getMaster ()->
							postEvent (new
							Rts2Event (EVENT_ADD_FIXED_OFFSET,
							(void *) &offset));
						break;
				}
				image->toAcquisition ();
			}
			break;
	}
	ElementAcquire::postEvent (event);
}

int ElementAcquireStar::processImage (Image * image)
{
	int ret;
	ConnFocus *processor;
	if (processingState != WAITING_IMAGE)
	{
		logStream (MESSAGE_ERROR)
			<<
			"ElementAcquireStar::processImage invalid processingState: "
			<< (int) processingState << sendLog;
		processingState = FAILED;
		return -1;
	}
	obsId = image->getObsId ();
	imgId = image->getImgId ();
	processor = new ConnFocus (script->getMaster (), image, defaultImgProccess.c_str (), EVENT_STAR_DATA);
	// save image before processing
	image->saveImage ();
	ret = processor->init ();
	if (ret < 0)
	{
		delete processor;
		processor = NULL;
		processingState = FAILED;
		return 0;
	}
	else
	{
		script->getMaster ()->addConnection (processor);
		processingState = WAITING_ASTROMETRY;
		script->getMaster ()->postEvent (new Rts2Event (EVENT_ACQUIRE_WAIT));
		return 1;
	}
}

int ElementAcquireStar::getSource (Image * image, double &ra_offset, double &dec_offset)
{
	int ret;
	double off_x, off_y;
	double sep;
	float flux;
	ret = image->getBrightestOffset (off_x, off_y, flux);
	if (ret)
		return -1;
	if (!isnan (minFlux) && flux < minFlux)
	{
		logStream (MESSAGE_INFO) << "Source too week: flux " << flux <<
			" minFlux " << minFlux << sendLog;
		return -1;
	}
	ret = image->getOffset (off_x, off_y, ra_offset, dec_offset, sep);
	if (ret)
		return -1;
	if (sep < reqPrecision)
		return 0;
	return 1;
}

ElementAcquireHam::ElementAcquireHam (Script * in_script, int in_maxRetries, float in_expTime, struct ln_equ_posn * in_center_pos):ElementAcquireStar (in_script, in_maxRetries, 0.05, in_expTime, 0.7, 0.3, in_center_pos)
{
}

ElementAcquireHam::~ElementAcquireHam (void)
{
}

int ElementAcquireHam::getSource (Image * image, double &ra_off, double &dec_off)
{
	double ham_x, ham_y;
	double sep;
	int ret;
	ret = image->getHam (ham_x, ham_y);
	if (ret)
		return -1;				 // continue HAM search..
	// change fixed offset
	logStream (MESSAGE_DEBUG)
		<< "ElementAcquireHam::getSource " << ham_x << " " << ham_y <<
		sendLog;
	ret = image->getOffset (ham_x, ham_y, ra_off, dec_off, sep);
	if (ret)
		return -1;
	logStream (MESSAGE_DEBUG)
		<< "ElementAcquireHam::offsets ra: " << ra_off << " dec: " <<
		dec_off << sendLog;
	if (sep < reqPrecision)
		return 0;
	return 1;
}
