/* 
 * XML-RPC daemon.
 * Copyright (C) 2010-2011 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#ifndef __RTS2_XMLRPCD__
#define __RTS2_XMLRPCD__

#include "config.h"
#include <deque>

#ifdef HAVE_PGSQL
#include "../../lib/rts2db/rts2devicedb.h"
#else
#include "rts2config.h"
#include "device.h"
#endif /* HAVE_PGSQL */

#include "directory.h"
#include "events.h"
#include "httpreq.h"
#include "session.h"
#include "xmlrpc++/XmlRpc.h"

#include "xmlapi.h"

#include "augerreq.h"
#include "nightreq.h"
#include "obsreq.h"
#include "targetreq.h"
#include "imgpreview.h"
#include "devicesreq.h"
#include "planreq.h"
#include "graphreq.h"
#include "switchstatereq.h"
#include "libjavascript.h"
#include "libcss.h"
#include "api.h"
#include "images.h"
#include "../../lib/rts2/connnotify.h"
#include "../../lib/rts2script/execcli.h"
#include "../../lib/rts2script/scriptinterface.h"
#include "../../lib/rts2script/scripttarget.h"

#define OPT_STATE_CHANGE            OPT_LOCAL + 76

#define EVENT_XMLRPC_VALUE_TIMER    RTS2_LOCAL_EVENT + 850
#define EVENT_XMLRPC_BB             RTS2_LOCAL_EVENT + 851

using namespace XmlRpc;

/**
 * @file
 * XML-RPC access to RTS2.
 *
 * @defgroup XMLRPC XML-RPC
 */
namespace rts2xmlrpc
{

class Directory;

/**
 * Support class/interface for operations needed by XmlDevClient and XmlDevClientCamera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlDevInterface
{
	public:
		void stateChanged (Rts2ServerState * state);

		void valueChanged (rts2core::Value * value);

		double getValueChangedTime (rts2core::Value *value);

	protected:
		virtual XmlRpcd *getMaster () = 0;
		virtual Rts2Conn *getConnection () = 0;

	private:
		// value change times
		std::map <rts2core::Value *, double> changedTimes;
};

/**
 * XML-RPC client class. Provides functions for XML-RPCd to react on state
 * and value changes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class XmlDevClient:public rts2core::Rts2DevClient, XmlDevInterface
{
	public:
		XmlDevClient (Rts2Conn *conn):rts2core::Rts2DevClient (conn), XmlDevInterface () {}

		virtual void stateChanged (Rts2ServerState * state)
		{
			XmlDevInterface::stateChanged (state);
			rts2core::Rts2DevClient::stateChanged (state);
		}

		virtual void valueChanged (rts2core::Value * value)
		{
			XmlDevInterface::valueChanged (value);
			rts2core::Rts2DevClient::valueChanged (value);
		}

	protected:
		virtual XmlRpcd *getMaster ()
		{
			return (XmlRpcd *) rts2core::Rts2DevClient::getMaster ();
		}

		virtual Rts2Conn *getConnection () { return rts2core::Rts2DevClient::getConnection (); }
};

/**
 * Device client for Camera, which is used inside XMLRPCD. Writes images to
 * disk file, provides access to images.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlDevCameraClient:public rts2script::DevClientCameraExec, rts2script::ScriptInterface, XmlDevInterface
{
	public:
		XmlDevCameraClient (Rts2Conn *conn);

		virtual ~XmlDevCameraClient ()
		{
		}

		virtual void stateChanged (Rts2ServerState * state)
		{
			XmlDevInterface::stateChanged (state);
			rts2image::DevClientCameraImage::stateChanged (state);
		}

		virtual void valueChanged (rts2core::Value * value)
		{
			XmlDevInterface::valueChanged (value);
			rts2image::DevClientCameraImage::valueChanged (value);
		}
		
		virtual rts2image::Image *createImage (const struct timeval *expStart);

		/**
		 * Return default image expansion subpath.
		 *
		 * @return default image expansion subpath
		 */
		const char * getDefaultFilename () { return fexpand.c_str (); }

		/**
		 * Return if there is running script on the device.
		 */
		bool isScriptRunning ();

		/**
		 * Execute script. If script is running, first kill it.
		 */
		void executeScript (const char *scriptbuf, bool killScripts = false);

		/**
		 * Set expansion for the next file. Throws error if there is an expansion
		 * filled in, which was not yet used. This probably signal two consequtive
		 * calls to this method, without camera going to EXPOSE state.
		 */
		void setNextExpand (const char *fe);

		int findScript (std::string in_deviceName, std::string & buf) { buf = currentscript; return 0; }

	protected:
		virtual rts2image::imageProceRes processImage (rts2image::Image * image);

		virtual XmlRpcd *getMaster ()
		{
			return (XmlRpcd *) rts2image::DevClientCameraImage::getMaster ();
		}

		virtual Rts2Conn *getConnection () { return rts2image::DevClientCameraImage::getConnection (); }

	private:
		// path for storing XMLRPC produced images
		std::string path;
		
		// expansion for images
		std::string fexpand;

		// expand path for next filename
		std::string nexpand;

		/**
		 * Last image location.
		 */
		rts2core::ValueString *lastFilename;

		/**
		 * Call scriptends before new script is started.
		 */
		rts2core::ValueBool *callScriptEnds;

		std::string currentscript;

		rts2script::ScriptTarget currentTarget;

		template < typename T > void createOrReplaceValue (T * &val, Rts2Conn *conn, int32_t expectedType, const char *suffix, const char *description, bool writeToFits = true, int32_t valueFlags = 0, int queCondition = 0)
		{
			std::string vn = std::string (conn->getName ()) + suffix;
			rts2core::Value *v = conn->getValue (vn.c_str ());

			if (v)
			{
				if (v->getValueType () == expectedType)
					val = (T *) v;
				else
					throw rts2core::Error (std::string ("cannot create ") + suffix + ", value already exists with different type");		
			}
			else
			{
				((rts2core::Daemon *) conn->getMaster ())->createValue (val, vn.c_str (), description, false);
			}
		}
};

/**
 * XML-RPC daemon class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
#ifdef HAVE_PGSQL
class XmlRpcd:public Rts2DeviceDb, XmlRpc::XmlRpcServer
#else
class XmlRpcd:public rts2core::Device, XmlRpc::XmlRpcServer
#endif
{
	public:
		XmlRpcd (int argc, char **argv);
		virtual ~XmlRpcd ();

		virtual rts2core::Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);

		void stateChangedEvent (Rts2Conn *conn, Rts2ServerState *new_state);

		void valueChangedEvent (Rts2Conn *conn, rts2core::Value *new_value);

		virtual void message (Rts2Message & msg);

		/**
		 * Create new session for given user.
		 *
		 * @param _username  Name of the user.
		 * @param _timeout   Timeout in seconds for session validity.
		 *
		 * @return String with session ID.
		 */
		std::string addSession (std::string _username, time_t _timeout);


		/**
		 * Returns true if session with a given sessionId exists.
		 *
		 * @param sessionId  Session ID.
		 *
		 * @return True if session with a given session ID exists.
		 */
		bool existsSession (std::string sessionId);

		/**
		 * If XmlRpcd is allowed to send emails.
		 */
		bool sendEmails () { return send_emails->getValueBool (); }

		virtual void postEvent (Rts2Event *event);

		/**
		 * Returns messages buffer.
		 */
		std::deque <Rts2Message> & getMessages () { return messages; }

		/**
		 * Return prefix for generated pages - usefull for pages behind proxy.
		 */
		const char* getPagePrefix () { return page_prefix.c_str (); }

		/**
		 * Default channel for display.
		 */
		int defchan; 

		/**
		 * If requests from localhost should be authorized.
		 */
		bool authorizeLocalhost () { return auth_localhost; }

		/**
		 * Returns true, if given path is marked as being public - accessible to all.
		 */
		bool isPublic (struct sockaddr_in *saddr, const std::string &path);

		/**
		 * Return default image label.
		 */
		const char *getDefaultImageLabel ();

		rts2core::ConnNotify * getNotifyConnection () { return notifyConn; }

		/**
		 * Register asynchronous API call.
		 */
		void registerAPI (AsyncAPI *a) { asyncAPIs.push_back (a); }

	protected:
		virtual int idle ();
#ifndef HAVE_PGSQL
		virtual int willConnect (Rts2Address * _addr);
#endif
		virtual int processOption (int in_opt);
		virtual int init ();
		virtual void addSelectSocks ();
		virtual void selectSuccess ();

		virtual void signaledHUP ();

		virtual void connectionRemoved (Rts2Conn *coon);

		virtual void asyncFinished (XmlRpcServerConnection *source);

		virtual void removeConnection (XmlRpcServerConnection *source);
	private:
		int rpcPort;
		const char *stateChangeFile;
		const char *defLabel;
		std::map <std::string, Session*> sessions;

		std::deque <Rts2Message> messages;

		std::vector <Directory *> directories;
		std::list <AsyncAPI *> asyncAPIs;
		std::list <AsyncAPI *> deleteAsync;

		Events events;

		rts2core::ValueBool *send_emails;

#ifndef HAVE_PGSQL
		const char *config_file;
#endif

		bool auth_localhost;

		std::string page_prefix;

		void sendBB ();

		rts2core::ValueInteger *bbCadency;

		void reloadEventsFile ();

		// pages
		Login login;
		DeviceCount deviceCount;
		ListDevices listDevices;
		DeviceByType deviceByType;
		DeviceType deviceType;
		DeviceCommand deviceCommand;
		MasterState masterState;
		MasterStateIs masterStateIs;
		DeviceState deviceState;
		ListValues listValues;
		ListValuesDevice listValuesDevice;
		ListPrettyValuesDevice listPrettValuesDecice;
		GetValue _getValue;
		SetValue setValue;
		SetValueByType setValueByType;
		IncValue incValue;
		GetMessages _getMessages;

#ifdef HAVE_PGSQL
		ListTargets listTargets;
		ListTargetsByType listTargetsByType;
		TargetInfo targetInfo;
		TargetAltitude targetAltitude;
		ListTargetObservations listTargetObservations;
		ListMonthObservations listMonthObservations;
		ListImages listImages;
		TicketInfo ticketInfo;
		RecordsValues recordsValues;
		Records records;
		RecordsAverage recordsAverage;
		UserLogin userLogin;
#endif // HAVE_PGSQL

#ifdef HAVE_LIBJPEG
		JpegImageRequest jpegRequest;
		JpegPreview jpegPreview;
		DownloadRequest downloadRequest;
		CurrentPosition current;
#ifdef HAVE_PGSQL
		Graph graph;
		AltAzTarget altAzTarget;
#endif // HAVE_PGSQL
		ImageReq imageReq;
#endif /* HAVE_LIBJPEG */
		FitsImageRequest fitsRequest;
		LibJavaScript javaScriptRequests;
		LibCSS cssRequests;
		API api;
#ifdef HAVE_PGSQL
		Auger auger;
		Night night;
		Observation observation;
		Targets targets;
		AddTarget addTarget;
		Plan plan;
#endif // HAVE_PGSQL
		SwitchState switchState;
		Devices devices;

		rts2core::ConnNotify *notifyConn;
};

};

#endif /* __RTS2_XMLRPCD__ */
