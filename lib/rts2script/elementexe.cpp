/*
 * Script element for command execution.
 * Copyright (C) 2009-2010 Petr Kubanek <petr@kubanek.net>
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

#include "elementexe.h"
#include "rts2script/execcli.h"
#include "rts2script/script.h"
#include "daemon.h"

#define DEBUG_EXE

using namespace rts2script;
using namespace rts2image;

ConnExecute::ConnExecute (Execute *_masterElement, rts2core::Block *_master, const char *_exec):ConnExe (_master, _exec, true)
{
	masterElement = _masterElement;
	exposure_started = 0;
	keep_next_image = false;
	waitTargetMove = false;
}

ConnExecute::~ConnExecute ()
{
	if (masterElement != NULL)
	{
		if (masterElement->getClient () != NULL)
			masterElement->getClient ()->postEvent (new rts2core::Event (EVENT_COMMAND_OK));
		masterElement->deleteExecConn ();
	}

	for (std::list <Image *>::iterator iter = images.begin (); iter != images.end (); iter++)
	{
		logStream (MESSAGE_WARNING) << "removing image " << (*iter)->getAbsoluteFileName () << ", you probably don't want this - please make sure images are processed in script" << sendLog;
		(*iter)->deleteImage ();
		deleteImage (*iter);
	}
}

void ConnExecute::postEvent (rts2core::Event *event)
{
	switch (event->getType ())
	{
		case EVENT_MOVE_OK:
			if (waitTargetMove)
				writeToProcess ("0");
			if (masterElement && masterElement->getConnection ())
				logStream (MESSAGE_DEBUG) << masterElement->getConnection ()->getName () << " elementexe get EVENT_MOVE_OK" << sendLog;
			break;
		case EVENT_MOVE_FAILED:
			if (waitTargetMove)
			{
				writeToProcess ("! move failed");
				writeToProcess ("ERR");
			}
			if (masterElement && masterElement->getConnection ())
				logStream (MESSAGE_DEBUG) << masterElement->getConnection ()->getName () << " elementexe get EVENT_MOVE_FAILED" << sendLog;
			break;
	}
	ConnExe::postEvent (event);
}

void ConnExecute::notActive ()
{
	// waiting for image - this will not be returned
	switch (exposure_started)
	{
		case 1:
			writeToProcess ("& exposure interrupted");
			exposure_started = -5;
			break;
		case 2:
			writeToProcess ("& readout interruped");
			exposure_started = -6;
			break;
	}
	ConnExe::notActive ();
}

void ConnExecute::processCommand (char *cmd)
{
	char *imagename;
	char *expandPath;

	char *device;
	char *value;
	char *operat;
	char *operand;
	char *comm;

#ifdef DEBUG_EXE
	std::string devname;

	if (masterElement && masterElement->getConnection ())
		devname = masterElement->getConnection ()->getName ();

	logStream (MESSAGE_DEBUG) << "connexe: " << devname << " " << cmd << " " << command_buf_top << sendLog;
#endif
	// initiate timeout for external script inactivity - connection will terminate if nothing received for a long time
	if (getMaster ()->getValue (".", "exe_timeout") &&
		getMaster ()->getValue (".", "exe_timeout")->getValueDouble () > 0)
		endTime = getNow () + getMaster ()->getValue (".", "exe_timeout")->getValueDouble ();

	if (!strcasecmp (cmd, "exposure"))
	{
		if (!checkActive (true))
			return;
		if (masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new rts2core::CommandExposure (getMaster (), (rts2core::DevClientCamera *) masterElement->getClient (), BOP_EXPOSURE));
		exposure_started = 1;
	}
	else if (!strcasecmp (cmd, "exposure_wfn") || !strcasecmp (cmd, "exposure_overwrite"))
	{
		if (!checkActive (true))
			return;
		if (paramNextString (&imagename))
			return;
		if (masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		((rts2script::DevClientCameraExec *) masterElement->getClient ())->setExpandPath (imagename);
		((rts2script::DevClientCameraExec *) masterElement->getClient ())->setOverwrite (!strcasecmp (cmd, "exposure_overwrite"));
		masterElement->getConnection ()->queCommand (new rts2core::CommandExposure (getMaster (), (rts2core::DevClientCamera *) masterElement->getClient (), BOP_EXPOSURE));
		keep_next_image = true;
		exposure_started = 1;
	}
	else if (!strcasecmp (cmd, "progress"))
	{
		double start,end;
		if (paramNextDouble (&start) || paramNextDouble (&end) || !paramEnd ())
			return;
		if (masterElement == NULL || masterElement->getClient () == NULL)
			return;
		((DevClientCameraExec *) masterElement->getClient ())->scriptProgress (start, end);
	}
	else if (!strcasecmp (cmd, "radec"))
	{
		if (!checkActive ())
			return;
		struct ln_equ_posn radec;
		if (paramNextHMS (&radec.ra) || paramNextDMS (&radec.dec) || !paramEnd ())
			return;
		master->postEvent (new rts2core::Event (EVENT_CHANGE_TARGET, (void *) &radec));
	}
	else if (!strcasecmp (cmd, "newobs"))
	{
		if (!checkActive ())
			return;
		struct ln_equ_posn radec;
		if (paramNextHMS (&radec.ra) || paramNextDMS (&radec.dec) || !paramEnd ())
			return;
		master->postEvent (new rts2core::Event (EVENT_NEW_TARGET, (void *) &radec));
	}
	else if (!strcasecmp (cmd, "altaz"))
	{
		if (!checkActive (false))
			return;
		struct ln_hrz_posn hrz;
		if (paramNextDMS (&hrz.alt) || paramNextDMS (&hrz.az) || !paramEnd ())
			return;
		master->postEvent (new rts2core::Event (EVENT_CHANGE_TARGET_ALTAZ, (void *) &hrz));
	}
	else if (!strcasecmp (cmd, "newaltaz"))
	{
		if (!checkActive (false))
			return;
		struct ln_hrz_posn hrz;
		if (paramNextDMS (&hrz.alt) || paramNextDMS (&hrz.az) || !paramEnd ())
			return;
		master->postEvent (new rts2core::Event (EVENT_NEW_TARGET_ALTAZ, (void *) &hrz));
	}
	else if (!strcmp (cmd, "dark"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->toDark ();
			writeToProcess ((*iter)->getAbsoluteFileName ());
			if (masterElement != NULL && masterElement->getClient () != NULL)
				((DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			deleteImage (*iter);
			images.erase (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot move " << imagename << " to dark path, image was probably already handled (renamed,..)" << sendLog;
		}
	}
	else if (!strcmp (cmd, "flat"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->toFlat ();
			writeToProcess ((*iter)->getAbsoluteFileName ());
			if (masterElement != NULL && masterElement->getClient () != NULL)
				((DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			deleteImage (*iter);
			images.erase (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot move " << imagename << " to flat path, image was probably already handled (renamed,..)" << sendLog;
		}

	}
	else if (!strcmp (cmd, "archive"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->toArchive ();
			writeToProcess ((*iter)->getAbsoluteFileName ());
			deleteImage (*iter);
			images.erase (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot move " << imagename << " to archive path, image was probably already handled (renamed,..)" << sendLog;
		}

	}
	else if (!strcmp (cmd, "trash"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->toTrash ();
			writeToProcess ((*iter)->getAbsoluteFileName ());
			deleteImage (*iter);
			images.erase (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot move " << imagename << " to trash path, image was probably already handled (renamed,..)" << sendLog;
		}

	}
	else if (!strcmp (cmd, "rename"))
	{
		if (paramNextString (&imagename) || paramNextString (&expandPath))
			return;
		try
		{
			std::list <Image *>::iterator iter = findImage (imagename);
			if (iter != images.end ())
			{
				(*iter)->renameImageExpand (expandPath);
				writeToProcess ((*iter)->getAbsoluteFileName ());
				deleteImage (*iter);
				images.erase (iter);
			}
			else
			{
				logStream (MESSAGE_ERROR) << "cannot rename " << imagename << ", image was probably already handled (renamed,..)" << sendLog;
			}
		}
		catch (rts2core::Error &er)
		{
			writeToProcess ((std::string ("E failed ") + er.what ()).c_str ());
		}
	}
	else if (!strcmp (cmd, "move"))
	{
		if (paramNextString (&imagename) || paramNextString (&expandPath))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->renameImageExpand (expandPath);
			writeToProcess ((*iter)->getAbsoluteFileName ());
			(*iter)->deleteFromDB ();
			deleteImage (*iter);
			images.erase (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot move " << imagename << ", image was probably already handled (renamed,..)" << sendLog;
		}
	}
	else if (!strcmp (cmd, "copy"))
	{
		if (paramNextString (&imagename) || paramNextString (&expandPath))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->copyImageExpand (expandPath);
			writeToProcess ((*iter)->getAbsoluteFileName ());
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot copy " << imagename << ", image was probably already handled (renamed,..)" << sendLog;
		}
	}
	else if (!strcmp (cmd, "delete"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->deleteImage ();
			deleteImage (*iter);
			images.erase (iter);
		}
	}
	else if (!strcmp (cmd, "process"))
	{
		if (paramNextString (&imagename) || masterElement == NULL || masterElement->getClient () == NULL)
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			((DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			deleteImage (*iter);
			images.erase (iter);
		}
		else
		{
			logStream (MESSAGE_ERROR) << "cannot process " << imagename << ", image was probably already handled (renamed,..)" << sendLog;
		}
	}
	else if (!strcmp (cmd, "?"))
	{
		if (paramNextString (&value) || masterElement == NULL || masterElement->getConnection () == NULL)
			return;
		rts2core::Value *val = masterElement->getConnection()->getValue (value);
		if (val)
		{
			writeToProcess (val->getValue ());
			return;
		}
		writeToProcess ("ERR");
	}
	else if (!strcmp (cmd, "command"))
	{
		if (!checkActive (false))
			return;
		if ((comm = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getConnection () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new rts2core::Command (getMaster (), comm));
	}
	else if (!strcmp (cmd, "VT"))
	{
		if (!checkActive (false))
			return;
		if (paramNextString (&device) || paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getClient () == NULL)
			return;
		int deviceTypeNum = getDeviceType (device);
		rts2core::CommandChangeValue cmdch (masterElement->getClient ()->getMaster (), std::string (value), *operat, std::string (operand), true);
		getMaster ()->queueCommandForType (deviceTypeNum, cmdch);
	}
	else if (!strcmp (cmd, "value"))
	{
		if (paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new rts2core::CommandChangeValue (masterElement->getClient ()->getMaster (), std::string (value), *operat, std::string (operand), true));
	}
	else if (!strcmp (cmd, "device_by_type"))
	{
		if (paramNextString (&device))
			return;
		rts2core::connections_t::iterator iter = getMaster ()->getConnections ()->begin ();
		getMaster ()->getOpenConnectionType (getDeviceType (device), iter);
		if (iter != getMaster ()->getConnections ()->end ())
			writeToProcess ((*iter)->getName ());
		else
			writeToProcess ("! cannot find device with given name");
	}
	else if (!strcmp (cmd, "end_script"))
	{
		masterElement->requestEndScript ();
		notActive ();
	}
	else if (!strcmp (cmd, "end_target"))
	{
		notActive ();
		master->postEvent (new rts2core::Event (EVENT_STOP_OBSERVATION));
	}
	else if (!strcmp (cmd, "stop_target"))
	{
		notActive ();
		master->postEvent (new rts2core::Event (EVENT_STOP_TARGET));
	}
	else if (!strcmp (cmd, "wait_target_move"))
	{
		if (masterElement->getTarget ())
		{
			if (masterElement->getTarget ()->wasMoved ())
			{
				writeToProcess ("0");
			}
			else
			{
				waitTargetMove = true;
			}
		}
		else
		{
			writeToProcess ("! there isn't target to wait for");
			writeToProcess ("ERR");
		}
	}
	else if (!strcmp (cmd, "target_disable"))
	{
		if (masterElement->getTarget ())
		{
			masterElement->getTarget ()->setTargetEnabled (false);
			masterElement->getTarget ()->save (true);
		}
	}
	else if (!strcmp (cmd, "target_tempdisable"))
	{
		int ti;
		if (paramNextInteger (&ti) || masterElement->getTarget () == NULL)
			return;
		time_t now;
		time (&now);
		now += ti;
		masterElement->getTarget ()->setNextObservable (&now);
		masterElement->getTarget ()->save (true);
	}
	else if (!strcmp (cmd, "loop_disable"))
	{
		Value *val = getMaster ()->getValue (".", "auto_loop");

		if (val) {
			logStream (MESSAGE_INFO) << "disabling auto_loop for current observation due to loopdisable command" << sendLog;
			((rts2core::ValueBool *)val)->setValueBool (false);
			((rts2core::Daemon *)getMaster ())->sendValueAll (val);
		}
	}
	else if (!strcmp (cmd, "loopcount"))
	{
		std::ostringstream os;
		if (masterElement == NULL || masterElement->getScript () == NULL)
			os << "-1";
		else
			os << masterElement->getScript ()->getLoopCount ();
		writeToProcess (os.str ().c_str ());
	}
	else if (!strcmp (cmd, "run_device"))
	{
		if (masterElement == NULL || masterElement->getConnection () == NULL)
			writeToProcess ("& not active");
		else
			writeToProcess (masterElement->getConnection ()->getName ());
	}
	else if (!strcmp (cmd, "requeue"))
	{
		char *queueName;
		double delay;
		if (paramNextString (&queueName) || paramNextDouble (&delay))
			return;
		if (masterElement->getTarget ())
		{
			rts2core::connections_t::iterator iter = getMaster ()->getConnections ()->begin ();
			getMaster ()->getOpenConnectionType (DEVICE_TYPE_SELECTOR, iter);
			if (iter != getMaster ()->getConnections ()->end ())
			{
				(*iter)->queCommand (new rts2core::CommandQueueAt (getMaster (), queueName, masterElement->getTarget ()->getTargetID (), getNow () + delay, NAN));
				writeToProcess ((*iter)->getName ());
			}
			else
			{
				writeToProcess ("! cannot find any selector");
			}
		}
		else
		{
			writeToProcess ("! cannot get target");
		}
	}

	else
	{
		ConnExe::processCommand (cmd);
	}
}

void ConnExecute::connectionError (int last_data_size)
{
	rts2core::ConnFork::connectionError (last_data_size);
	// inform master to delete us..
	if (masterElement != NULL)
	{
		if (masterElement->getClient () != NULL)
			masterElement->getClient ()->postEvent (new rts2core::Event (EVENT_COMMAND_OK));
		if (masterElement)
			masterElement->deleteExecConn ();
	}
	masterElement = NULL;
}

void ConnExecute::errorReported (int current_state, int old_state)
{
	switch (exposure_started)
	{
		case 0:
			writeToProcess ("! error detected while running the script");
			break;
		case 1:
			writeToProcess ("exposure_failed");
			exposure_started = -1;
			break;
		case 2:
			writeToProcess ("! device failed");
			writeToProcess ("ERR");
			exposure_started = -2;
			break;
	}
}

void ConnExecute::exposureEnd (bool expectImage)
{
	if (exposure_started == 1)
	{
		if (expectImage)
		{
			writeToProcess ("exposure_end");
			exposure_started = 2;
		}
		else
		{
			writeToProcess ("exposure_end_noimage");
			exposure_started = 0;
		}

	}
	else
	{
		logStream (MESSAGE_WARNING) << "script received end-of-exposure without starting it. This probably signal out-of-sync communication between executor and camera" << sendLog;
	}
}

void ConnExecute::exposureFailed ()
{
	switch (exposure_started)
	{
		case 1:
			writeToProcess ("exposure_failed");
			exposure_started = -3;
			break;
		case 2:
			writeToProcess ("! exposure failed");
			writeToProcess ("ERR");
			exposure_started = -4;
			break;
		default:
			logStream (MESSAGE_WARNING) << "script received failure of exposure without starting one. This probably signal out-of-sync communication" << sendLog;
	}
}

int ConnExecute::processImage (Image *image)
{
	if (exposure_started == 2)
	{
		std::string imgn = image->getAbsoluteFileName ();
		if (keep_next_image)
		{
			keep_next_image = false;
		}
		else
		{
			images.push_back (image);
		}
		image->saveImage ();
		writeToProcess ((std::string ("image ") + imgn).c_str ());
		exposure_started = 0;
	}
	else
	{
		logStream (MESSAGE_WARNING) << "script executes method to start image processing without trigerring an exposure (" << exposure_started << ")" << sendLog;
		return -1;
	}
	return 1;
}

bool ConnExecute::knowImage (Image * image)
{
	return (std::find (images.begin (), images.end (), image) != images.end ());
}

std::list <Image *>::iterator ConnExecute::findImage (const char *path)
{
	std::list <Image *>::iterator iter;
	for (iter = images.begin (); iter != images.end (); iter++)
	{
		if (!strcmp (path, (*iter)->getAbsoluteFileName ()))
			return iter;
	}
	return iter;
}

int ConnExecute::writeToProcess (const char *msg)
{
#ifdef DEBUG_EXE
	std::string devname;

	if (masterElement && masterElement->getConnection ())
		devname = masterElement->getConnection ()->getName ();

	logStream (MESSAGE_DEBUG) << "connexe reply: " << devname << " " << msg << sendLog;
#endif

	return ConnExe::writeToProcess (msg);
}

Execute::Execute (Script * _script, rts2core::Block * _master, const char *_exec, Rts2Target *_target): Element (_script)
{
	connExecute = NULL;
	client = NULL;

	master = _master;
	exec = _exec;

	target = _target;

	endScript = false;
}

Execute::~Execute ()
{
	if (connExecute)
	{
		errno = 0;
		connExecute->nullMasterElement ();
		connExecute->endConnection ();
		deleteExecConn ();
	}
	client = NULL;
}

void Execute::errorReported (int current_state, int old_state)
{
	if (connExecute)
	{
		connExecute->errorReported (current_state, old_state);
	}
	Element::errorReported (current_state, old_state);
}

void Execute::exposureEnd (bool expectImage)
{
	if (connExecute)
	{
		connExecute->exposureEnd (expectImage);
		return;
	}
	Element::exposureEnd (expectImage);
}

void Execute::exposureFailed ()
{
	if (connExecute)
	{
		connExecute->exposureFailed ();
		return;
	}
	Element::exposureFailed ();
}

void Execute::notActive ()
{
	if (connExecute)
		connExecute->notActive ();
}

int Execute::processImage (Image *image)
{
	if (connExecute)
		return connExecute->processImage (image);

	return Element::processImage (image);
}

bool Execute::knowImage (Image *image)
{
	if (connExecute)
		return connExecute->knowImage (image);

	return Element::knowImage (image);
}

int Execute::defnextCommand (rts2core::DevClient * _client, rts2core::Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	if (connExecute == NULL)
	{
		connExecute = new ConnExecute (this, master, exec);
		int ret = connExecute->init ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot execute script control command, ending script. Script will not be executed again." << sendLog;
			return NEXT_COMMAND_STOP_TARGET;
		}
		client = _client;
		client->getMaster ()->addConnection (connExecute);
	}

	if (endScript)
	{
		connExecute->nullMasterElement ();
		connExecute = NULL;
		return NEXT_COMMAND_END_SCRIPT;
	}

	if (connExecute->getConnState () == CONN_DELETE)
	{
		connExecute->nullMasterElement ();
		// connExecute will be deleted by rts2core::Block holding connection
		connExecute = NULL;
		client = NULL;
		return NEXT_COMMAND_NEXT;
	}

	return NEXT_COMMAND_KEEP;
}
