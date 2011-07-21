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
#include "rts2execcli.h"
#include "script.h"

using namespace rts2script;
using namespace rts2image;

ConnExecute::ConnExecute (Execute *_masterElement, rts2core::Block *_master, const char *_exec):ConnExe (_master, _exec, true)
{
	masterElement = _masterElement;
	exposure_started = false;
}

ConnExecute::~ConnExecute ()
{
	if (masterElement != NULL)
	{
		if (masterElement->getClient () != NULL)
			masterElement->getClient ()->postEvent (new Rts2Event (EVENT_COMMAND_OK));
		masterElement->deleteExecConn ();
	}

	for (std::list <Image *>::iterator iter = images.begin (); iter != images.end (); iter++)
	{
		(*iter)->deleteImage ();
		delete *iter;
	}
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
	
	if (!strcmp (cmd, "exposure"))
	{
		if (masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new Rts2CommandExposure (getMaster (), (Rts2DevClientCamera *) masterElement->getClient (), BOP_EXPOSURE));
		exposure_started = true;
	}
	else if (!strcasecmp (cmd, "radec"))
	{
		struct ln_equ_posn radec;
		if (paramNextHMS (&radec.ra) || paramNextDMS (&radec.dec) || !paramEnd ())
			return;
		master->postEvent (new Rts2Event (EVENT_CHANGE_TARGET, (void *) &radec));
	}
	else if (!strcasecmp (cmd, "newobs"))
	{
		struct ln_equ_posn radec;
		if (paramNextHMS (&radec.ra) || paramNextDMS (&radec.dec) || !paramEnd ())
			return;
		master->postEvent (new Rts2Event (EVENT_NEW_TARGET, (void *) &radec));
	}
	else if (!strcasecmp (cmd, "altaz"))
	{
		struct ln_hrz_posn hrz;
		if (paramNextDMS (&hrz.alt) || paramNextDMS (&hrz.az) || !paramEnd ())
			return;
		master->postEvent (new Rts2Event (EVENT_CHANGE_TARGET_ALTAZ, (void *) &hrz));
	}
	else if (!strcasecmp (cmd, "newaltaz"))
	{
		struct ln_hrz_posn hrz;
		if (paramNextDMS (&hrz.alt) || paramNextDMS (&hrz.az) || !paramEnd ())
			return;
		master->postEvent (new Rts2Event (EVENT_NEW_TARGET_ALTAZ, (void *) &hrz));
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
			((Rts2DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			delete *iter;
			images.erase (iter);
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
			((Rts2DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			delete *iter;
			images.erase (iter);
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
			delete *iter;
			images.erase (iter);
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
			delete *iter;
			images.erase (iter);
		}
	}
	else if (!strcmp (cmd, "rename"))
	{
		if (paramNextString (&imagename) || paramNextString (&expandPath))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->renameImageExpand (expandPath);
			writeToProcess ((*iter)->getAbsoluteFileName ());
			delete *iter;
			images.erase (iter);
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
			delete *iter;
			images.erase (iter);
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
	}
	else if (!strcmp (cmd, "delete"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->deleteImage ();
			delete *iter;
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
			((Rts2DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			delete *iter;
			images.erase (iter);
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
		if ((comm = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getConnection () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new Rts2Command (getMaster (), comm));
	}
	else if (!strcmp (cmd, "VT"))
	{
		if (paramNextString (&device) || paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL)
			return;
		int deviceTypeNum = getDeviceType (device);
		Rts2CommandChangeValue cmdch (masterElement->getClient (), std::string (value), *operat, std::string (operand), true);
		getMaster ()->queueCommandForType (deviceTypeNum, cmdch);
	}
	else if (!strcmp (cmd, "value"))
	{
		if (paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new Rts2CommandChangeValue (masterElement->getClient (), std::string (value), *operat, std::string (operand), true));
	}
	else if (!strcmp (cmd, "loopcount"))
	{
		std::ostringstream os;
		os << masterElement->getScript ()->getLoopCount ();
		writeToProcess (os.str ().c_str ());
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
			masterElement->getClient ()->postEvent (new Rts2Event (EVENT_COMMAND_OK));
		masterElement->deleteExecConn ();
	}
	masterElement = NULL;
}

void ConnExecute::exposureEnd ()
{
	if (exposure_started)  
		writeToProcess ("exposure_end");
	else
		logStream (MESSAGE_WARNING) << "script received end-of-exposure without starting it. This probably signal out-of-sync communication between executor and camera" << sendLog;
}

int ConnExecute::processImage (Image *image)
{
	if (exposure_started)
	{
		images.push_back (image);
		image->saveImage ();
		writeToProcess ((std::string ("image ") + image->getAbsoluteFileName ()).c_str ());
		exposure_started = false;
	}
	else
	{
		logStream (MESSAGE_WARNING) << "script executes method to start image processing without trigerring an exposure" << sendLog;
		return -1;
	} 
	return 1;
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

Execute::Execute (Script * _script, rts2core::Block * _master, const char *_exec): Element (_script)
{
	connExecute = NULL;
	client = NULL;

	master = _master;
	exec = _exec;
}

Execute::~Execute ()
{
	if (connExecute)
	{
		connExecute->nullMasterElement ();
		connExecute->endConnection ();
		deleteExecConn ();
	}
	client = NULL;
}

void Execute::exposureEnd ()
{
	if (connExecute)
	{
		connExecute->exposureEnd ();
		return;
	}
	Element::exposureEnd ();
}

int Execute::processImage (Image *image)
{
	if (connExecute)
		return connExecute->processImage (image);

	return Element::processImage (image);
}

int Execute::defnextCommand (Rts2DevClient * _client, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE])
{
	if (connExecute == NULL)
	{
		connExecute = new ConnExecute (this, master, exec);
		int ret = connExecute->init ();
		if (ret)
		{
			logStream (MESSAGE_ERROR) << "Cannot execute script control command, ending script." << sendLog;
			return NEXT_COMMAND_END_SCRIPT;
		}
		client = _client;
		client->getMaster ()->addConnection (connExecute);
	}

	if (connExecute->getConnState () == CONN_DELETE)
	{
		// connExecute will be deleted by rts2core::Block holding connection
		connExecute = NULL;
		client = NULL;
		return NEXT_COMMAND_NEXT;
	}

	return NEXT_COMMAND_KEEP;
}
