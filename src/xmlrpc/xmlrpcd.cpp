/* 
 * XML-RPC daemon.
 * Copyright (C) 2007-2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2007 Stanislav Vitek <standa@iaa.es>
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

#include "xmlrpcd.h"

#ifdef HAVE_PGSQL
#include "../../lib/rts2db/messagedb.h"
#else
#include "rts2config.h"
#include "device.h"
#endif /* HAVE_PGSQL */

#if defined(HAVE_LIBJPEG) && HAVE_LIBJPEG == 1
#include <Magick++.h>
using namespace Magick;
#endif // HAVE_LIBJPEG

#include "r2x.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define OPT_STATE_CHANGE        OPT_LOCAL + 76
#define OPT_NO_EMAILS           OPT_LOCAL + 77

using namespace XmlRpc;

/**
 * @file
 * XML-RPC access to RTS2.
 *
 * @defgroup XMLRPC XML-RPC
 */

using namespace rts2xmlrpc;

void XmlDevInterface::stateChanged (Rts2ServerState * state)
{
	(getMaster ())->stateChangedEvent (getConnection (), state);
}

void XmlDevInterface::valueChanged (rts2core::Value * value)
{
	changedTimes[value] = getMaster()->getNow ();
	(getMaster ())->valueChangedEvent (getConnection (), value);
}

double XmlDevInterface::getValueChangedTime (rts2core::Value *value)
{
	std::map <rts2core::Value *, double>::iterator iter = changedTimes.find (value);
	if (iter == changedTimes.end ())
		return rts2_nan ("f");
	return iter->second;
}

rts2image::Image *XmlDevCameraClient::createImage (const struct timeval *expStart)
{
	delete lastImage;
	lastImage = new rts2image::Image ("/tmp/xmlrpcd_%c.fits", getExposureNumber (), expStart, connection);
	lastImage->keepImage ();
	return lastImage;
}

rts2image::imageProceRes XmlDevCameraClient::processImage (rts2image::Image * image)
{
	return rts2image::IMAGE_KEEP_COPY;
}

int XmlRpcd::idle ()
{
	for (std::list <AsyncAPI *>::iterator iter = deleteAsync.begin (); iter != deleteAsync.end (); iter++)
	{
		delete *iter;
	}
	deleteAsync.clear ();
#ifdef HAVE_PGSQL
	return Rts2DeviceDb::idle ();
#else
	return rts2core::Device::idle ();
#endif
}

#ifndef HAVE_PGSQL
int XmlRpcd::willConnect (Rts2Address *_addr)
{
	if (_addr->getType () < getDeviceType ()
		|| (_addr->getType () == getDeviceType ()
		&& strcmp (_addr->getName (), getDeviceName ()) < 0))
		return 1;
	return 0;
}
#endif

int XmlRpcd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'p':
			rpcPort = atoi (optarg);
			break;
		case OPT_STATE_CHANGE:
			stateChangeFile = optarg;
			break;
		case OPT_NO_EMAILS:
			send_emails->setValueBool (false);
			break;
#ifdef HAVE_PGSQL
		default:
			return Rts2DeviceDb::processOption (in_opt);
#else
		case OPT_CONFIG:
			config_file = optarg;
			break;
		default:
			return rts2core::Device::processOption (in_opt);
#endif
	}
	return 0;
}

int XmlRpcd::init ()
{
	int ret;
#ifdef HAVE_PGSQL
	ret = Rts2DeviceDb::init ();
#else
	ret = rts2core::Device::init ();
#endif
	if (ret)
		return ret;

	ret = notifyConn->init ();
	if (ret)
		return ret;

	addConnection (notifyConn);

	if (printDebug ())
		XmlRpc::setVerbosity (5);

	XmlRpcServer::bindAndListen (rpcPort);
	XmlRpcServer::enableIntrospection (true);

	// try states..
	if (stateChangeFile != NULL)
	{
		try
		{
		  	reloadEventsFile ();
		}
		catch (XmlError ex)
		{
			logStream (MESSAGE_ERROR) << ex << sendLog;
			return -1;
		}
	}

	for (std::vector <DirectoryMapping>::iterator iter = events.dirs.begin (); iter != events.dirs.end (); iter++)
		directories.push_back (new Directory (iter->getTo (), iter->getPath (), "", this));
	
	if (events.docroot.length () > 0)
		XmlRpcServer::setDefaultGetRequest (new Directory (NULL, events.docroot.c_str (), "index.html", NULL));
	if (events.defchan != INT_MAX)
		defchan = events.defchan;
	else
		defchan = 0;

	setMessageMask (MESSAGE_MASK_ALL);

	if (events.bbServers.size () != 0)
		addTimer (30, new Rts2Event (EVENT_XMLRPC_BB));

#ifndef HAVE_PGSQL
	ret = Rts2Config::instance ()->loadFile (config_file);
	if (ret)
		return ret;
#endif
	// get page prefix
	Rts2Config::instance ()->getString ("xmlrpcd", "page_prefix", page_prefix, "");

	// auth_localhost
	auth_localhost = Rts2Config::instance ()->getBoolean ("xmlrpcd", "auth_localhost", auth_localhost);

#ifdef HAVE_LIBJPEG
	Magick::InitializeMagick (".");
#endif /* HAVE_LIBJPEG */
	return ret;
}

void XmlRpcd::addSelectSocks ()
{
#ifdef HAVE_PGSQL
	Rts2DeviceDb::addSelectSocks ();
#else
	rts2core::Device::addSelectSocks ();
#endif
	XmlRpcServer::addToFd (&read_set, &write_set, &exp_set);
}

void XmlRpcd::selectSuccess ()
{
#ifdef HAVE_PGSQL
	Rts2DeviceDb::selectSuccess ();
#else
	rts2core::Device::selectSuccess ();
#endif
	XmlRpcServer::checkFd (&read_set, &write_set, &exp_set);
}

void XmlRpcd::signaledHUP ()
{
#ifdef HAVE_PGSQL
	Rts2DeviceDb::selectSuccess ();
#else
	rts2core::Device::selectSuccess ();
#endif
	reloadEventsFile ();
}

void XmlRpcd::connectionRemoved (Rts2Conn *conn)
{
	for (std::list <AsyncAPI *>::iterator iter = asyncAPIs.begin (); iter != asyncAPIs.end ();)
	{
		if ((*iter)->isForConnection (conn))
			iter = asyncAPIs.erase (iter);
		else
			iter++;
	}
#ifdef HAVE_PGSQL
	Rts2DeviceDb::connectionRemoved (conn);
#else
	rts2core::Device::connectionRemoved (conn);
#endif
}

void XmlRpcd::asyncFinished (XmlRpcServerConnection *source)
{
	for (std::list <AsyncAPI *>::iterator iter = asyncAPIs.begin (); iter != asyncAPIs.end ();)
	{
		if ((*iter)->isForSource (source))
		{
			deleteAsync.push_back (*iter);
			iter = asyncAPIs.erase (iter);
		}
		else
		{
			iter++;
		}
	}
	XmlRpcServer::asyncFinished (source);
}

void XmlRpcd::removeConnection (XmlRpcServerConnection *source)
{
	for (std::list <AsyncAPI *>::iterator iter = asyncAPIs.begin (); iter != asyncAPIs.end ();)
	{
		if ((*iter)->isForSource (source))
		{
			deleteAsync.push_back (*iter);
			iter = asyncAPIs.erase (iter);
		}
		else
			iter++;
	}
	XmlRpcServer::removeConnection (source);
}

#ifdef HAVE_PGSQL
XmlRpcd::XmlRpcd (int argc, char **argv): Rts2DeviceDb (argc, argv, DEVICE_TYPE_XMLRPC, "XMLRPC")
#else
XmlRpcd::XmlRpcd (int argc, char **argv): rts2core::Device (argc, argv, DEVICE_TYPE_XMLRPC, "XMLRPC")
#endif
,XmlRpcServer (),
  events(this),
// construct all handling events..
  login (this),
  deviceCount (this),
  listDevices (this),
  deviceByType (this),
  deviceType (this),
  deviceCommand (this),
  masterState (this),
  masterStateIs (this),
  deviceState (this),
  listValues (this),
  listValuesDevice (this),
  listPrettValuesDecice (this),
  _getValue (this),
  setValue (this),
  setValueByType (this),
  incValue (this),
  _getMessages (this),
#ifdef HAVE_PGSQL
  listTargets (this),
  listTargetsByType (this),
  targetInfo (this),
  targetAltitude (this),
  listTargetObservations (this),
  listMonthObservations (this),
  listImages (this),
  ticketInfo (this),
  recordsValues (this),
  records (this),
  recordsAverage (this),
  userLogin (this),
#endif // HAVE_PGSQL

  // web requeusts
#ifdef HAVE_LIBJPEG
  jpegRequest ("/jpeg", this),
  jpegPreview ("/preview", "/", this),
  downloadRequest ("/download", this),
  current ("/current", this),
#ifdef HAVE_PGSQL
  graph ("/graph", this),
  altAzTarget ("/altaz", this),
#endif // HAVE_PGSQL
  imageReq ("/images", this),
#endif /* HAVE_LIBJPEG */
  fitsRequest ("/fits", this),
  javaScriptRequests ("/js", this),
  cssRequests ("/css", this),
  api ("/api", this),
#ifdef HAVE_PGSQL
  auger ("/auger", this),
  night ("/nights", this),
  observation ("/observations", this),
  targets ("/targets", this),
  addTarget ("/addtarget", this),
  plan ("/plan", this),
#endif // HAVE_PGSQL
  switchState ("/switchstate", this),
  devices ("/devices", this)
{
	rpcPort = 8889;
	stateChangeFile = NULL;
	defLabel = "%Y-%m-%d %H:%M:%S @OBJECT";

	auth_localhost = true;

	notifyConn = new rts2core::ConnNotify (this);

	createValue (send_emails, "send_email", "if XML-RPC is allowed to send emails", false, RTS2_VALUE_WRITABLE);
	send_emails->setValueBool (true);

	createValue (bbCadency, "bb_cadency", "cadency (in seconds) of upstream BB messages", false, RTS2_VALUE_WRITABLE);
	bbCadency->setValueInteger (60);

#ifndef HAVE_PGSQL
	config_file = NULL;

	addOption (OPT_CONFIG, "config", 1, "configuration file");
#endif
	addOption ('p', NULL, 1, "XML-RPC port. Default to 8889");
	addOption (OPT_STATE_CHANGE, "event-file", 1, "event changes file, list commands which are executed on state change");
	addOption (OPT_NO_EMAILS, "no-emails", 0, "do not send emails");
	XmlRpc::setVerbosity (0);
}


XmlRpcd::~XmlRpcd ()
{
	for (std::vector <Directory *>::iterator id = directories.begin (); id != directories.end (); id++)
		delete *id;

	for (std::map <std::string, Session *>::iterator iter = sessions.begin (); iter != sessions.end (); iter++)
	{
		delete (*iter).second;
	}
	sessions.clear ();
#ifdef HAVE_LIBJPEG
	MagickLib::DestroyMagick ();
#endif /* HAVE_LIBJPEG */
}

rts2core::Rts2DevClient * XmlRpcd::createOtherType (Rts2Conn * conn, int other_device_type)
{
	if (other_device_type == DEVICE_TYPE_CCD)
		return new XmlDevCameraClient (conn);
	return new XmlDevClient (conn);
}

void XmlRpcd::stateChangedEvent (Rts2Conn * conn, Rts2ServerState * new_state)
{
	double now = getNow ();
	// look if there is some state change command entry, which match us..
	for (StateCommands::iterator iter = events.stateCommands.begin (); iter != events.stateCommands.end (); iter++)
	{
		StateChange *sc = (*iter);
		if (sc->isForDevice (conn->getName (), conn->getOtherType ()) && sc->executeOnStateChange (new_state->getOldValue (), new_state->getValue ()))
		{
			sc->run (this, conn, now);
		}
	}
}

void XmlRpcd::valueChangedEvent (Rts2Conn * conn, rts2core::Value * new_value)
{
	double now = getNow ();
	// look if there is some state change command entry, which match us..
	for (ValueCommands::iterator iter = events.valueCommands.begin (); iter != events.valueCommands.end (); iter++)
	{
		ValueChange *vc = (*iter);
		if (vc->isForValue (conn->getName (), new_value->getName (), now))
		 
		{
			try
			{
				vc->run (new_value, now);
				vc->runSuccessfully (now);
			}
			catch (rts2core::Error err)
			{
				logStream (MESSAGE_ERROR) << err << sendLog;
			}
		}
	}
}

void XmlRpcd::message (Rts2Message & msg)
{
// log message to DB, if database is present
#ifdef HAVE_PGSQL
	if (msg.isNotDebug ())
	{
		rts2db::MessageDB msgDB (msg);
		msgDB.insertDB ();
	}
#endif
	// look if there is some state change command entry, which match us..
	for (MessageCommands::iterator iter = events.messageCommands.begin (); iter != events.messageCommands.end (); iter++)
	{
		MessageEvent *me = (*iter);
		if (me->isForMessage (&msg))
		{
			try
			{
				me->run (&msg);
			}
			catch (rts2core::Error err)
			{
				logStream (MESSAGE_ERROR) << err << sendLog;
			}
		}
	}
	
	while (messages.size () > 42) // messagesBufferSize ())
	{
		messages.pop_front ();
	}

	messages.push_back (msg);
}

std::string XmlRpcd::addSession (std::string _username, time_t _timeout)
{
	Session *s = new Session (_username, time(NULL) + _timeout);
	sessions[s->getSessionId()] = s;
	return s->getSessionId ();
}

bool XmlRpcd::existsSession (std::string sessionId)
{
	std::map <std::string, Session*>::iterator iter = sessions.find (sessionId);
	if (iter == sessions.end ())
	{
		return false;
	}
	return true;
}

void XmlRpcd::postEvent (Rts2Event *event)
{
	switch (event->getType ())
	{
		case EVENT_XMLRPC_BB:
			sendBB ();
			addTimer (bbCadency->getValueInteger (), event);
			return;
	}
#ifdef HAVE_PGSQL
	Rts2DeviceDb::postEvent (event);
#else
	rts2core::Device::postEvent (event);
#endif
}

bool XmlRpcd::isPublic (struct sockaddr_in *saddr, const std::string &path)
{
	if (authorizeLocalhost () || ntohl (saddr->sin_addr.s_addr) != INADDR_LOOPBACK)
		return events.isPublic (path);
	return true;
}

const char *XmlRpcd::getDefaultImageLabel ()
{
	return defLabel;
}

void XmlRpcd::sendBB ()
{
	// construct data..
	XmlRpcValue data;

	connections_t::iterator iter;

	for (iter = getConnections ()->begin (); iter != getConnections ()->end (); iter++)
	{
		XmlRpcValue connData;
		connectionValuesToXmlRpc (*iter, connData, false);
		data[(*iter)->getName ()] = connData;
	}

	events.bbServers.sendUpdate (&data);
}

void XmlRpcd::reloadEventsFile ()
{
	if (stateChangeFile != NULL)
	{
		events.load (stateChangeFile);
	  	defLabel = events.getDefaultImageLabel ();
		if (defLabel == NULL)
			defLabel = "%Y-%m-%d %H:%M:%S @OBJECT";
	}
}

int main (int argc, char **argv)
{
	XmlRpcd device (argc, argv);
	return device.run ();
}
