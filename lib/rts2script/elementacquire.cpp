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

#include "rts2script/elementacquire.h"
#include "rts2script/connimgprocess.h"
#include "rts2fits/devclifoc.h"
#include "configuration.h"

using namespace rts2plan;
using namespace rts2script;
using namespace rts2image;

ElementAcquire::ElementAcquire (Script * in_script, double in_precision, float in_expTime, struct ln_equ_posn *in_center_pos):Element (in_script)
{
	reqPrecision = in_precision;
	lastPrecision = NAN;
	expTime = in_expTime;
	processingState = NEED_IMAGE;
	Configuration::instance ()->getString ("imgproc", "astrometry",
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

void ElementAcquire::postEvent (rts2core::Event * event)
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
				if (std::isnan (img_prec))
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
					if (std::isnan (lastPrecision) || img_prec < lastPrecision / 2)
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
			if (std::isnan (lastPrecision))
			{
				bool en = false;
				script->getMaster ()->postEvent (new rts2core::Event (EVENT_QUICK_ENABLE, (void *) &en));
			}

			camera->getConnection ()->queCommand (new rts2core::CommandChangeValue (camera->getMaster (), "SHUTTER", '=', 0));
			// change values of the exposur
			camera->getConnection ()->queCommand (new rts2core::CommandChangeValue (camera->getMaster (), "exposure", '=', expTime));

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
	processor = new ConnImgProcess (script->getMaster (), defaultImgProccess.c_str (), image->getFileName (), Configuration::instance ()->getAstrometryTimeout ());
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
		script->getMaster ()->postEvent (new rts2core::Event (EVENT_ACQUIRE_WAIT));
	}
	return 0;
}

void ElementAcquire::cancelCommands ()
{
	processingState = FAILED;
}

double ElementAcquire::getExpectedDuration (int runnum)
{
	if (runnum == 0)
		// conservative estimate: 2 images, 2 minutes to run astrometry on them
		return (expTime + script->getFullReadoutTime ()) * 3 + 120;
	return 0;
}
