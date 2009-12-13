/*
 * Script element for command execution.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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
#include "script.h"

using namespace rts2script;

ConnExecute::ConnExecute (Execute *_masterElement, Rts2Block *_master, const char *_exec):rts2core::ConnFork (_master, _exec, true)
{
	masterElement = _masterElement;
}

ConnExecute::~ConnExecute ()
{
	if (masterElement)
		masterElement->getClient ()->postEvent (new Rts2Event (EVENT_COMMAND_OK));

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
	masterElement->getClient ()->postEvent (new Rts2Event (EVENT_COMMAND_OK));
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

	if (paramNextString (&cmd))
		return;
	if (!strcmp (cmd, "exposure"))
	{
		masterElement->getConnection ()->queCommand (new Rts2CommandExposure (getMaster (), (Rts2DevClientCamera *) masterElement->getClient (), 0));
	}
	else if (!strcmp (cmd, "dark"))
	{
		if (paramNextString (&imagename))
			return;
		std::list <Rts2Image *>::iterator iter = findImage (imagename);
		if (iter != images.end ())
		{
			(*iter)->toDark ();
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
	else if (!strcmp (cmd, "V"))
	{
		if (paramNextString (&device) || paramNextString (&value) || paramNextString (&operat) || paramNextString (&operand))
			return;
		Rts2Conn *conn = getMaster ()->getOpenConnection (device);
		if (conn)
		{
			conn->queCommand (new Rts2CommandChangeValue (conn->getOtherDevClient (), std::string (value), *operat, std::string (operand), true));
		}
	}

	else if (!strcmp (cmd, "value"))
	{
		if (paramNextString (&value) || paramNextString (&operat) || paramNextString (&operand))
			return;
		masterElement->getConnection ()->queCommand (new Rts2CommandChangeValue (masterElement->getClient (), std::string (value), *operat, std::string (operand), true));
	}
	else if (!strcmp (cmd, "?"))
	{
		if (paramNextString (&device) || paramNextString (&value))
			return;
		Rts2Conn *conn = getMaster ()->getOpenConnection (device);
		if (conn)
		{
			Rts2Value *val = conn->getValue (value);
			if (val)
			{
				sendMsg (val->getValue ());
				return;
			}
		}
		sendMsg ("ERR");
	}
		
}

int ConnExecute::processImage (Rts2Image *image)
{
	images.push_back (image);
	image->saveImage ();
	sendMsg ((std::string ("image ") + image->getAbsoluteFileName ()).c_str ());
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
		connExecute = NULL;
	}
	client = NULL;
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
			logStream (MESSAGE_ERROR) << "Cannot execute script controll command, ending script." << sendLog;
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
