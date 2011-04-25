/*
 * Script element.
 * Copyright (C) 2005-2008 Petr Kubanek <petr@kubanek.net>
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

#include <iterator>

#include "script.h"

#include "../utils/rts2config.h"
#include "../utils/utilsfunc.h"
#include "../writers/image.h"
#include "../writers/devclifoc.h"

using namespace rts2script;
using namespace rts2image;

Element::Element (Script * _script)
{
	script = _script;
	startPos = script->getParsedStartPos ();
	timerclear (&idleTimeout);
	timerclear (&nextIdle);
}

Element::~Element ()
{
}

void Element::getDevice (char new_device[DEVICE_NAME_SIZE])
{
	script->getDefaultDevice (new_device);
}

int Element::processImage (Image * image)
{
	return -1;
}

int Element::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	return NEXT_COMMAND_NEXT;
}

int Element::nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	return defnextCommand (camera, new_command, new_device);
}

int Element::nextCommand (Rts2DevClientPhot * phot, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	return defnextCommand (phot, new_command, new_device);
}

int Element::getStartPos ()
{
	return startPos;
}

int Element::getLen ()
{
	return len;
}

int Element::idleCall ()
{
	if (!timerisset (&idleTimeout))
		return NEXT_COMMAND_KEEP;
	struct timeval tv;
	gettimeofday (&tv, NULL);
	if (timercmp (&tv, &nextIdle, >))
	{
		int ret = idle ();
		timeradd (&tv, &idleTimeout, &nextIdle);
		return ret;
	}
	return NEXT_COMMAND_KEEP;
}

void Element::setIdleTimeout (double sec)
{
	idleTimeout.tv_sec = (long int) floor (sec);
	idleTimeout.tv_usec =
		(long int) ((sec - (double) nextIdle.tv_sec) * USEC_SEC);
	// and set nextIdle appropriatly
	gettimeofday (&nextIdle, NULL);
	timeradd (&idleTimeout, &nextIdle, &nextIdle);
}

int Element::idle ()
{
	return NEXT_COMMAND_KEEP;
}

ElementExpose::ElementExpose (Script * _script, float in_expTime):Element (_script)
{
	expTime = in_expTime;
	callProgress = first;
}

int ElementExpose::nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	getDevice (new_device);
	if (callProgress == first)
	{
		callProgress = SHUTTER;
		if (camera->getConnection ()->getValue ("SHUTTER") != NULL && camera->getConnection ()->getValueInteger ("SHUTTER") != 0)
		{
			*new_command = new Rts2CommandChangeValue (camera, "SHUTTER", '=', 0);
			(*new_command)->setBopMask (BOP_TEL_MOVE);
			return NEXT_COMMAND_KEEP;
		}
	}
	// change values of the exposure
	if (callProgress == SHUTTER && camera->getConnection ()->getValue ("exposure") && camera->getConnection ()->getValueDouble ("exposure") != expTime)
	{
		callProgress = EXPOSURE;
		*new_command = new Rts2CommandChangeValue (camera, "exposure", '=', expTime);
		(*new_command)->setBopMask (BOP_TEL_MOVE);
		return NEXT_COMMAND_KEEP;
	}
	*new_command = new Rts2CommandExposure (script->getMaster (), camera, BOP_EXPOSURE);
	return 0;
}

ElementDark::ElementDark (Script * _script, float in_expTime):Element (_script)
{
	expTime = in_expTime;
	callProgress = first;
}

int ElementDark::nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	getDevice (new_device);
	if (callProgress == first)
	{
		callProgress = SHUTTER;
		if (camera->getConnection ()->getValue ("SHUTTER") != NULL && camera->getConnection ()->getValueInteger ("SHUTTER") != 1)
		{
			*new_command = new Rts2CommandChangeValue (camera, "SHUTTER", '=', 1);
			(*new_command)->setBopMask (BOP_TEL_MOVE);
			return NEXT_COMMAND_KEEP;
		}
	}
	// change values of the exposure
	if (callProgress == SHUTTER && camera->getConnection ()->getValue ("exposure") && camera->getConnection ()->getValueDouble ("exposure") != expTime)
	{
		callProgress = EXPOSURE;
		*new_command = new Rts2CommandChangeValue (camera, "exposure", '=', expTime);
		(*new_command)->setBopMask (BOP_TEL_MOVE);
		return NEXT_COMMAND_KEEP;
	}
	*new_command = new Rts2CommandExposure (script->getMaster (), camera, 0);
	return 0;
}

ElementBox::ElementBox (Script * _script, int in_x, int in_y, int in_w, int in_h):Element (_script)
{
	x = in_x;
	y = in_y;
	w = in_w;
	h = in_h;
}

int ElementBox::nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	*new_command = new Rts2CommandBox (camera, x, y, w, h);
	getDevice (new_device);
	return 0;
}

ElementCenter::ElementCenter (Script * _script, int in_w, int in_h):Element (_script)
{
	w = in_w;
	h = in_h;
}

int ElementCenter::nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	*new_command = new Rts2CommandCenter (camera, w, h);
	getDevice (new_device);
	return 0;
}

ElementChange::ElementChange (Script * _script, char new_device[DEVICE_NAME_SIZE], double in_ra, double in_dec):Element (_script)
{
	deviceName = new char[strlen (new_device) + 1];
	strcpy (deviceName, new_device);
	setChangeRaDec (in_ra, in_dec);
}

int ElementChange::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	*new_command = new Rts2CommandChange (script->getMaster (), ra, dec);
	strcpy (new_device, deviceName);
	return 0;
}

ElementWait::ElementWait (Script * _script):Element (_script)
{
}

int ElementWait::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	return NEXT_COMMAND_CHECK_WAIT;
}

ElementWaitAcquire::ElementWaitAcquire (Script * _script, int in_tar_id):Element (_script)
{
	tar_id = in_tar_id;
}

int ElementWaitAcquire::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	AcquireQuery ac = AcquireQuery (tar_id);
	// detect is somebody plans to run A command..
	script->getMaster ()->postEvent (new Rts2Event (EVENT_ACQUIRE_QUERY, (void *) &ac));
	//#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG)
		<< "ElementWaitAcquire::defnextCommand " << ac.count
		<< " (" << script->getDefaultDevice () << ") " << tar_id << sendLog;
	//#endif
	if (ac.count)
		return NEXT_COMMAND_WAIT_ACQUSITION;
	return NEXT_COMMAND_NEXT;
}

ElementPhotometer::ElementPhotometer (Script * _script, int in_filter, float in_exposure, int in_count):Element (_script)
{
	filter = in_filter;
	exposure = in_exposure;
	count = in_count;
}

int ElementPhotometer::nextCommand (Rts2DevClientPhot * phot, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	*new_command = new Rts2CommandIntegrate (phot, filter, exposure, count);
	getDevice (new_device);
	return 0;
}

ElementSendSignal::ElementSendSignal (Script * _script, int in_sig):Element (_script)
{
	sig = in_sig;
	askedFor = false;
}

ElementSendSignal::~ElementSendSignal ()
{
	if (askedFor)
		script->getMaster ()->
			postEvent (new Rts2Event (EVENT_SIGNAL, (void *) &sig));
}

void ElementSendSignal::postEvent (Rts2Event * event)
{
	switch (event->getType ())
	{
		case EVENT_SIGNAL_QUERY:
			if (*(int *) event->getArg () == sig)
			{
				askedFor = true;
				*(int *) event->getArg () = -1;
			}
			break;
	}
	Element::postEvent (event);
}

int ElementSendSignal::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	// when some else script will wait reach point when it has to wait for
	// this signal, it will not wait as it will ask before enetring wait
	// if some script will send required signal
	script->getMaster ()->postEvent (new Rts2Event (EVENT_SIGNAL, (void *) &sig));
	askedFor = false;
	return NEXT_COMMAND_NEXT;
}

ElementWaitSignal::ElementWaitSignal (Script * _script, int in_sig):Element (_script)
{
	sig = in_sig;
}

int ElementWaitSignal::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	int ret;

	// not valid signal..
	if (sig == -1)
		return NEXT_COMMAND_NEXT;

	// nobody will send us a signal..end script
	ret = sig;
	script->getMaster ()->postEvent (new Rts2Event (EVENT_SIGNAL_QUERY, (void *) &ret));
	#ifdef DEBUG_EXTRA
	logStream (MESSAGE_DEBUG) << "ElementWaitSignal::defnextCommand " << ret << " (" << script->getDefaultDevice () << ")" << sendLog;
	#endif
	if (ret != -1)
		return NEXT_COMMAND_NEXT;
	return NEXT_COMMAND_WAIT_SIGNAL;
}

int ElementWaitSignal::waitForSignal (int in_sig)
{
	if (sig == in_sig)
	{
		sig = -1;
		return 1;
	}
	return 0;
}

ElementChangeValue::ElementChangeValue (Script * _script, const char *new_device, const char *chng_str):Element (_script)
{
	deviceName = new char[strlen (new_device) + 1];
	strcpy (deviceName, new_device);
	std::string chng_s = std::string (chng_str);
	op = '\0';
	rawString = false;
	int op_end = 0;
	int i;
	std::string::iterator iter;
	for (iter = chng_s.begin (), i = 0; iter != chng_s.end (); iter++, i++)
	{
		char ch = *iter;
		if (!op && (ch == '+' || ch == '-' || ch == '='))
		{
			valName = chng_s.substr (0, i);
			op = ch;
			if ((ch == '+' || ch == '-') && (*(iter + 1) == '='))
			{
				iter++;
				i++;
			}
			op_end = i;
			break;
		}
	}
	if (op == '\0')
		return;
	operands = rts2operands::OperandsSet();
	operands.parse (chng_s.substr (op_end + 1));
}

ElementChangeValue::~ElementChangeValue (void)
{
	delete[]deviceName;
}

void ElementChangeValue::getDevice (char new_device[DEVICE_NAME_SIZE])
{
	strcpy (new_device, deviceName);
}

int ElementChangeValue::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	if (op == '\0' || operands.size () == 0)
		return -1;
	// handle while exposing part..
	std::ostringstream _os;
	_os << operands;
	if (operands.size () > 1)
		rawString = true;
	if (valName[0] == '!')
	{
		*new_command = new Rts2CommandChangeValue (client, valName.substr (1), op, _os.str(), rawString);
		(*new_command)->setBopMask (BOP_TEL_MOVE | BOP_WHILE_STATE);
	}
	else
	{
		*new_command = new Rts2CommandChangeValue (client, valName, op, _os.str(), rawString);
		(*new_command)->setBopMask (BOP_TEL_MOVE);
	}
	getDevice (new_device);
	return 0;
}

std::string ElementChangeValue::getOperands ()
{
	std::ostringstream os;
	os << operands;
	return os.str ();
}

ElementComment::ElementComment (Script * _script, const char *in_comment, int in_cnum):Element (_script)
{
	comment = new char[strlen (in_comment) + 1];
	cnum = in_cnum;
	strcpy (comment, in_comment);
}

ElementComment::~ElementComment (void)
{
	delete []comment;
}

int ElementComment::defnextCommand (Rts2DevClient * client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	client->getConnection ()->queCommand (new Rts2CommandChangeValue (client, "COMM_NUM", '=', cnum));
	// script comment value
	*new_command = new Rts2CommandChangeValue (client, "SCR_COMM", '=', std::string (comment));
	getDevice (new_device);
	return 0;
}
