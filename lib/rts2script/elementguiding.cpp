/**
 * Script for guiding.
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

#include "elementguiding.h"
#include "execcli.h"
#include "configuration.h"
#include "../rts2fits/image.h"
#include "../rts2fits/devclifoc.h"

using namespace rts2script;
using namespace rts2image;

ElementGuiding::ElementGuiding (Script * in_script, float init_exposure, int in_endSignal):Element (in_script)
{
	expTime = init_exposure;
	endSignal = in_endSignal;
	processingState = NO_IMAGE;
	Configuration::instance ()->getString (in_script->getDefaultDevice (),
		"sextractor", defaultImgProccess);
	obsId = -1;
	imgId = -1;

	ra_mult = 1;
	dec_mult = 1;

	last_ra = rts2_nan ("f");
	last_dec = rts2_nan ("f");

	min_change = 0.001;
	Configuration::instance ()->getDouble ("guiding", "minchange", min_change);

	bad_change = min_change * 4;
	Configuration::instance ()->getDouble ("guiding", "badchange", bad_change);
}

ElementGuiding::~ElementGuiding (void)
{
}

void ElementGuiding::checkGuidingSign (double &last, double &mult, double act)
{
	// we are guiding in right direction, if sign changes
	if ((last < 0 && act > 0) || (last > 0 && act < 0))
	{
		last = act;
		return;
	}
	logStream (MESSAGE_DEBUG)
		<< "ElementGuiding::checkGuidingSign last: "
		<< last << " mult: " << mult << " act: " << act << " bad_change " <<
		bad_change << sendLog;
	// try to detect sign change
	if ((fabs (act) > 2 * fabs (last)) && (fabs (act) > bad_change))
	{
		mult *= -1;
		// last change was with wrong sign
		last = act * -1;
	}
	else
	{
		last = act;
	}
	logStream (MESSAGE_DEBUG)
		<< "ElementGuiding::checkGuidingSign last: "
		<< last << " mult: " << mult << " act: " << act << sendLog;
}

void ElementGuiding::postEvent (rts2core::Event * event)
{
	ConnFocus *focConn;
	Image *image;
	double star_x, star_y;
	double star_ra, star_dec, star_sep;
	float flux;
	int ret;
	switch (event->getType ())
	{
		case EVENT_GUIDING_DATA:
			focConn = (ConnFocus *) event->getArg ();
			image = focConn->getImage ();
			if (image->getObsId () == obsId && image->getImgId () == imgId)
			{
				ret = image->getBrightestOffset (star_x, star_y, flux);
				if (ret)
				{
					logStream (MESSAGE_DEBUG)
						<< "ElementGuiding::postEvent EVENT_GUIDING_DATA failed (numStars: "
						<< image->sexResultNum << ")" << sendLog;
					processingState = FAILED;
				}
				else
				{
					// guide..
					logStream (MESSAGE_DEBUG)
						<< "ElementGuiding::postEvent EVENT_GUIDING_DATA "
						<< star_x << " " << star_y << sendLog;
					ret =
						image->getOffset (star_x, star_y, star_ra, star_dec,
						star_sep);
					if (ret)
					{
						logStream (MESSAGE_DEBUG)
							<< "ElementGuiding::postEvent EVENT_GUIDING_DATA getOffset "
							<< ret << sendLog;
						processingState = NEED_IMAGE;
					}
					else
					{
						GuidingParams *pars;
						logStream (MESSAGE_DEBUG)
							<< "ElementGuiding::postEvent EVENT_GUIDING_DATA offsets ra: "
							<< star_ra << " dec: " << star_dec << sendLog;
						if (fabs (star_dec) > min_change)
						{
							checkGuidingSign (last_dec, dec_mult, star_dec);
							pars =
								new GuidingParams (DIR_NORTH, star_dec * dec_mult);
							script->getMaster ()->
								postEvent (new
								rts2core::Event (EVENT_TEL_START_GUIDING,
								(void *) pars));
							delete pars;
						}
						if (fabs (star_ra) > min_change)
						{
							checkGuidingSign (last_ra, ra_mult, star_ra);
							pars = new GuidingParams (DIR_EAST, star_ra * ra_mult);
							script->getMaster ()->
								postEvent (new
								rts2core::Event (EVENT_TEL_START_GUIDING,
								(void *) pars));
							delete pars;
						}
						processingState = NEED_IMAGE;
					}
				}
			}
			// queue us for processing
			image->saveImage ();
			script->getMaster ()-> postEvent (new rts2core::Event (EVENT_QUE_IMAGE, (void *) image));
			break;
	}
	Element::postEvent (event);
}

int ElementGuiding::nextCommand (rts2core::DevClientCamera * camera, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	int ret = endSignal;
	if (endSignal == -1)
		return NEXT_COMMAND_NEXT;
	switch (processingState)
	{
		case WAITING_IMAGE:
			*new_command = NULL;
			return NEXT_COMMAND_KEEP;
		case NO_IMAGE:
			script->getMaster ()->
				postEvent (new rts2core::Event (EVENT_SIGNAL_QUERY, (void *) &ret));
			if (ret != -1)
				return NEXT_COMMAND_NEXT;
		case NEED_IMAGE:
			// EXP_LIGHT, expTime);
			*new_command = new rts2core::CommandExposure (script->getMaster (), camera, BOP_EXPOSURE);
			getDevice (new_device);
			processingState = WAITING_IMAGE;
			return NEXT_COMMAND_KEEP;
		case FAILED:
			return NEXT_COMMAND_NEXT;
	}
	// should not happen!
	logStream (MESSAGE_DEBUG) << "ElementGuiding::nextCommand invalid state: " << (int) processingState << sendLog;
	return NEXT_COMMAND_NEXT;
}

int ElementGuiding::processImage (Image * image)
{
	int ret;
	ConnFocus *processor;
	logStream (MESSAGE_DEBUG) << "ElementGuiding::processImage state: " << (int) processingState << sendLog;
	if (processingState != WAITING_IMAGE)
	{
		logStream (MESSAGE_ERROR) << "ElementGuiding::processImage invalid processingState: " << (int) processingState << sendLog;
		processingState = FAILED;
		return -1;
	}
	obsId = image->getObsId ();
	imgId = image->getImgId ();
	logStream (MESSAGE_DEBUG) << "ElementGuiding::processImage defaultImgProccess: " << defaultImgProccess << sendLog;
	processor = new ConnFocus (script->getMaster (), image, defaultImgProccess.c_str (), EVENT_GUIDING_DATA);
	image->saveImage ();
	ret = processor->init ();
	if (ret < 0)
	{
		delete processor;
		processor = NULL;
		processingState = FAILED;
		return -1;
	}
	else
	{
		script->getMaster ()->addConnection (processor);
		processingState = NEED_IMAGE;
	}
	logStream (MESSAGE_DEBUG) << "Rts2ConnImgProcess::processImage executed processor " << ret << " processor " << processor << sendLog;
	return 1;
}

int ElementGuiding::waitForSignal (int in_sig)
{
	if (in_sig == endSignal)
	{
		endSignal = -1;
		return 1;
	}
	return 0;
}

void ElementGuiding::cancelCommands ()
{
}
