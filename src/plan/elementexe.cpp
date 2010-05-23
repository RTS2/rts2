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

ConnExecute::ConnExecute (Execute *_masterElement, Rts2Block *_master, const char *_exec):rts2core::ConnFork (_master, _exec, true, true)
{
	masterElement = _masterElement;
}

ConnExecute::~ConnExecute ()
{
	if (masterElement != NULL)
	{
		if (masterElement->getClient () != NULL)
			masterElement->getClient ()->postEvent (new Rts2Event (EVENT_COMMAND_OK));
		masterElement->deleteExecConn ();
	}

	for (std::list <Rts2Image *>::iterator iter = images.begin (); iter != images.end (); iter++)
	{
		(*iter)->deleteImage ();
		delete *iter;
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

void ConnExecute::processLine ()
{
	char *cmd;
	char *imagename;
	char *expandPath;

	char *device;
	char *value;
	char *operat;
	char *operand;
	char *comm;


	if (paramNextString (&cmd))
		return;

	if (!strcmp (cmd, "exposure"))
	{
		if (masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new Rts2CommandExposure (getMaster (), (Rts2DevClientCamera *) masterElement->getClient (), BOP_EXPOSURE));
	}
	else if (!strcmp (cmd, "dark"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
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
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
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
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
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
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
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
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->renameImageExpand (expandPath);
			writeToProcess ((*iter)->getAbsoluteFileName ());
			delete *iter;
			images.erase (iter);
		}
	}
	else if (!strcmp (cmd, "copy"))
	{
		if (paramNextString (&imagename) || paramNextString (&expandPath))
			return;
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
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
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
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
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			((Rts2DevClientCameraExec *) masterElement->getClient ())->queImage (*iter);
			delete *iter;
			images.erase (iter);
		}
	}
	else if (!strcmp (cmd, "C"))
	{
		if (paramNextString (&device) || (comm = paramNextWholeString ()) == NULL)
			return;
		Rts2Conn *conn = getConnectionForScript (device);
		if (conn)
		{
			conn->queCommand (new Rts2Command (getMaster (), comm));
		}
	}
	else if (!strcmp (cmd, "command"))
	{
		if ((comm = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getConnection () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new Rts2Command (getMaster (), comm));
	}
	else if (!strcmp (cmd, "V"))
	{
		if (paramNextString (&device) || paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL)
			return;
		Rts2Conn *conn = getConnectionForScript (device);
		if (conn)
		{
			conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), std::string (value), *operat, std::string (operand), true));
		}
	}
	else if (!strcmp (cmd, "value"))
	{
		if (paramNextString (&value) || paramNextString (&operat) || (operand = paramNextWholeString ()) == NULL || masterElement == NULL || masterElement->getConnection () == NULL || masterElement->getClient () == NULL)
			return;
		masterElement->getConnection ()->queCommand (new Rts2CommandChangeValue (masterElement->getClient (), std::string (value), *operat, std::string (operand), true));
	}
	else if (!strcmp (cmd, "?"))
	{
		if (paramNextString (&value) || masterElement == NULL || masterElement->getConnection () == NULL)
			return;
		Rts2Value *val = masterElement->getConnection()->getValue (value);
		if (val)
		{
			writeToProcess (val->getValue ());
			return;
		}
		writeToProcess ("ERR");
	}
	else if (!strcmp (cmd, "G"))
	{
		if (paramNextString (&device) || paramNextString (&value) || master == NULL)
			return;

		Rts2Value *val = NULL;

		if (isCentraldName (device))
			val = master->getSingleCentralConn ()->getValue (value);
		else
			val = master->getValue (device, value);

		if (val)
		{
			writeToProcess (val->getValue ());
			return;
		}
		else
		{
			writeToProcess ("ERR");
		}
	}
	else if (!strcmp (cmd, "log"))
	{
		if (paramNextString (&device) || (value = paramNextWholeString ()) == NULL)
			return;
		messageType_t logLevel;
		switch (toupper (*device))
		{
			case 'E':
				logLevel = MESSAGE_ERROR;
				break;
			case 'W':
				logLevel = MESSAGE_WARNING;
				break;
			case 'I':
				logLevel = MESSAGE_INFO;
				break;
			case 'D':
				logLevel = MESSAGE_DEBUG;
				break;
			default:
				logStream (MESSAGE_ERROR) << "Unknow log level: " << *device << sendLog;
				logLevel = MESSAGE_ERROR;
				break;
		}
		logStream (logLevel) << value << sendLog;
	}
}

void ConnExecute::exposureEnd ()
{
	writeToProcess ("exposure_end");
}

int ConnExecute::processImage (Rts2Image *image)
{
	images.push_back (image);
	image->saveImage ();
	writeToProcess ((std::string ("image ") + image->getAbsoluteFileName ()).c_str ());
	return 1;
}

void ConnExecute::processErrorLine (char *errbuf)
{
	logStream (MESSAGE_ERROR) << "From script received " << getExePath () << " " << errbuf << sendLog;
}

std::list <Rts2Image *>::iterator ConnExecute::findImage (const char *path)
{
	std::list <Rts2Image *>::iterator iter;
	for (iter = images.begin (); iter != images.end (); iter++)
	{
		if (!strcmp (path, (*iter)->getAbsoluteFileName ()))
			return iter;
	}
	return iter;
}

Rts2Conn *ConnExecute::getConnectionForScript (const char *_name)
{
	if (isCentraldName (_name))
		return getMaster ()->getSingleCentralConn ();
	return getMaster ()->getOpenConnection (_name);
}

Execute::Execute (Script * _script, Rts2Block * _master, const char *_exec): Element (_script)
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

int Execute::processImage (Rts2Image *image)
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
		// connExecute will be deleted by Rts2Block holding connection
		connExecute = NULL;
		client = NULL;
		return NEXT_COMMAND_NEXT;
	}

	return NEXT_COMMAND_KEEP;
}
