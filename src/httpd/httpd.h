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

#include "rts2-config.h"
#include <deque>

#ifdef RTS2_HAVE_PGSQL
#include "rts2db/devicedb.h"
#include "rts2db/plan.h"
#include "rts2json/addtargetreq.h"
#include "bbapi.h"
#else
#include "configuration.h"
#include "device.h"
#endif /* RTS2_HAVE_PGSQL */

#include "userlogins.h"
#include "graphreq.h"
#include "rts2json/directory.h"
#include "events.h"
#include "rts2json/httpreq.h"
#include "rts2json/images.h"
#include "rts2json/libcss.h"
#include "rts2json/libjavascript.h"
#include "rts2json/jsonvalue.h"
#include "rts2json/targetreq.h"
#include "rts2json/obsreq.h"
#include "rts2json/imgpreview.h"
#include "rts2json/nightreq.h"
#include "session.h"
#include "xmlrpc++/XmlRpc.h"

#include "xmlapi.h"

#include "augerreq.h"
#include "devicesreq.h"
#include "planreq.h"
#include "switchstatereq.h"
#include "api.h"

#include "connnotify.h"
#include "rts2script/execcli.h"
#include "rts2script/scriptinterface.h"
#include "rts2script/scripttarget.h"

#define OPT_STATE_CHANGE            OPT_LOCAL + 76

#define EVENT_XMLRPC_VALUE_TIMER    RTS2_LOCAL_EVENT + 850
#define EVENT_XMLRPC_BB             RTS2_LOCAL_EVENT + 851
#define EVENT_TERMINATE_TEST        RTS2_LOCAL_EVENT + 852

using namespace XmlRpc;

/**
 * @file
 * XML-RPC access to RTS2.
 *
 * @defgroup XMLRPC XML-RPC
 */
namespace rts2xmlrpc
{

/**
 * Support class/interface for operations needed by XmlDevClient and XmlDevClientCamera.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class XmlDevInterface:public rts2json::DevInterface
{
	public:
		XmlDevInterface ():changedTimes () {}
		void stateChanged (rts2core::ServerState * state);

		void valueChanged (rts2core::Value * value);

	protected:
		virtual HttpD *getMaster () = 0;
		virtual rts2core::Connection *getConnection () = 0;

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
class XmlDevClient:public rts2image::DevClientWriteImage, XmlDevInterface
{
	public:
		XmlDevClient (rts2core::Connection *conn):rts2image::DevClientWriteImage (conn), XmlDevInterface () {}

		virtual void stateChanged (rts2core::ServerState * state)
		{
			XmlDevInterface::stateChanged (state);
			rts2image::DevClientWriteImage::stateChanged (state);
		}

		virtual void valueChanged (rts2core::Value * value)
		{
			XmlDevInterface::valueChanged (value);
			rts2image::DevClientWriteImage::valueChanged (value);
		}

	protected:
		virtual HttpD *getMaster ()
		{
			return (HttpD *) rts2core::DevClient::getMaster ();
		}

		virtual rts2core::Connection *getConnection () { return rts2core::DevClient::getConnection (); }
};

/**
 * XML-RPC client class. Provides functions for XML-RPCd to react on state
 * and value changes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class XmlDevTelescopeClient:public rts2image::DevClientTelescopeImage, XmlDevInterface
{
	public:
		XmlDevTelescopeClient (rts2core::Connection *conn):rts2image::DevClientTelescopeImage (conn), XmlDevInterface () {}

		virtual void stateChanged (rts2core::ServerState * state)
		{
			XmlDevInterface::stateChanged (state);
			rts2image::DevClientTelescopeImage::stateChanged (state);
		}

		virtual void valueChanged (rts2core::Value * value)
		{
			XmlDevInterface::valueChanged (value);
			rts2image::DevClientTelescopeImage::valueChanged (value);
		}

	protected:
		virtual HttpD *getMaster ()
		{
			return (HttpD *) rts2image::DevClientTelescopeImage::getMaster ();
		}

		virtual rts2core::Connection *getConnection () { return rts2image::DevClientTelescopeImage::getConnection (); }
};

/**
 * XML-RPC client class. Provides functions for XML-RPCd to react on state
 * and value changes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup XMLRPC
 */
class XmlDevFocusClient:public rts2image::DevClientFocusImage, XmlDevInterface
{
	public:
		XmlDevFocusClient (rts2core::Connection *conn):rts2image::DevClientFocusImage (conn), XmlDevInterface () {}

		virtual void stateChanged (rts2core::ServerState * state)
		{
			XmlDevInterface::stateChanged (state);
			rts2image::DevClientFocusImage::stateChanged (state);
		}

		virtual void valueChanged (rts2core::Value * value)
		{
			XmlDevInterface::valueChanged (value);
			rts2image::DevClientFocusImage::valueChanged (value);
		}

	protected:
		virtual HttpD *getMaster ()
		{
			return (HttpD *) rts2image::DevClientFocusImage::getMaster ();
		}

		virtual rts2core::Connection *getConnection () { return rts2image::DevClientFocusImage::getConnection (); }
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
		XmlDevCameraClient (rts2core::Connection *conn);

		virtual ~XmlDevCameraClient ();

		virtual void deleteConnection (Connection *_conn);

		virtual void stateChanged (rts2core::ServerState * state)
		{
			XmlDevInterface::stateChanged (state);
			rts2script::DevClientCameraExec::stateChanged (state);
		}

		virtual void valueChanged (rts2core::Value * value)
		{
			XmlDevInterface::valueChanged (value);
			rts2script::DevClientCameraExec::valueChanged (value);
		}

		virtual rts2image::Image *createImage (const struct timeval *expStart);

		virtual void scriptProgress (double start, double end);

		/**
		 * Set flag indicating if before script should be called script_ends.
		 *
		 * @param nv  new call script end flag value
		 */
		void setCallScriptEnds (bool nv);

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
		 * Kill script on device.
		 */
		void killScript ();

		/**
		 * Set expansion for the next file. Throws error if there is an expansion
		 * filled in, which was not yet used. This probably signal two consequtive
		 * calls to this method, without camera going to EXPOSE state.
		 *
		 * @param fe              next filename expand string
		 */
		virtual void setExpandPath (const char *fe);

		/**
		 * Set expansion for duration of script. This is not reset by next exposure, but
		 * is reset by setExpandPath call.
		 *
		 * @param fe               next filename expand string
		 */
		void setScriptExpand (const char *fe);

		int findScript (std::string in_deviceName, std::string & buf) { buf = currentscript; return 0; }

		virtual void newDataConn (int data_conn);
		virtual void dataReceived (DataAbstractRead *data);
		virtual void fullDataReceived (int data_conn, DataChannels *data);
		virtual void exposureFailed (int status);

		rts2image::Image *getPreviousImage () { return previmage; }

		rts2core::ValueString *templateFile;

		int setTemplateFilename (const char *fn);

	protected:
		virtual void postEvent (Event *event);

		virtual rts2image::imageProceRes processImage (rts2image::Image * image);

		virtual HttpD *getMaster ()
		{
			return (HttpD *) rts2script::DevClientCameraExec::getMaster ();
		}

		virtual rts2core::Connection *getConnection () { return rts2script::DevClientCameraExec::getConnection (); }

	private:
		// path for storing XMLRPC produced images
		std::string path;

		// default expansion for images
		std::string fexpand;

		// expand path for next filename
		std::string nexpand;

		std::string screxpand;

		/**
		 * Last image location.
		 */
		rts2core::ValueString *lastFilename;

		/**
		 * Call scriptends before new script is started.
		 */
		rts2core::ValueBool *callScriptEnds;

		rts2core::ValueTime *scriptStart;
		rts2core::ValueTime *scriptEnd;

		rts2core::ValueInteger *exposureWritten;

		/**
		 * True/false if script is running.
		 */
		rts2core::ValueBool *scriptRunning;

		rts2core::ValueBool *writeMetaData;

		rts2image::Image *previmage;

		std::string currentscript;

		rts2script::ScriptTarget currentTarget;

		rts2core::Command *scriptKillCommand;

		template < typename T > void createOrReplaceValue (T * &val, rts2core::Connection *conn, int32_t expectedType, const char *suffix, const char *description, bool writeToFits = true, int32_t valueFlags = 0, int queCondition = 0)
		{
			std::string vn = std::string (conn->getName ()) + suffix;
			rts2core::Value *v = ((rts2core::Daemon *) conn->getMaster ())->getOwnValue (vn.c_str ());

			if (v)
			{
				if (v->getValueType () == expectedType)
					val = (T *) v;
				else
					throw rts2core::Error (std::string ("cannot create ") + suffix + ", value already exists with different type");
			}
			else
			{
				((rts2core::Daemon *) conn->getMaster ())->createValue (val, vn.c_str (), description, false, valueFlags, queCondition);
				((rts2core::Daemon *) conn->getMaster ())->updateMetaInformations (val);
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
#ifdef RTS2_HAVE_PGSQL
class HttpD:public rts2db::DeviceDb, XmlRpc::XmlRpcServer, rts2json::HTTPServer
#else
class HttpD:public rts2core::Device, XmlRpc::XmlRpcServer, rts2json::HTTPServer
#endif
{
	public:
		HttpD (int argc, char **argv);
		virtual ~HttpD ();

		virtual rts2core::DevClient *createOtherType (rts2core::Connection * conn, int other_device_type);

		virtual void deviceReady (rts2core::Connection *conn);

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		void stateChangedEvent (rts2core::Connection *conn, rts2core::ServerState *new_state);

		void valueChangedEvent (rts2core::Connection *conn, rts2core::Value *new_value);

		virtual void addPollSocks ();
		virtual void pollSuccess ();

		virtual void message (Message & msg);

		virtual int commandAuthorized (rts2core::Connection * conn);

		virtual void fileModified (struct inotify_event *event);

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
		 * If HttpD is allowed to send emails.
		 */
		bool sendEmails () { return send_emails->getValueBool (); }

		/**
		 * Returns messages buffer.
		 */
		std::deque <Message> & getMessages () { return messages; }

		virtual const char* getPagePrefix () { return page_prefix.c_str (); }

		virtual bool getDebug ();

		virtual void sendValueAll (rts2core::Value * value);

		virtual rts2db::CamList *getCameras ();

		virtual rts2core::connections_t *getConnections ();

		virtual void getOpenConnectionType (int deviceType, rts2core::connections_t::iterator &current);

		virtual bool isPublic (struct sockaddr_in *saddr, const std::string &path);

		virtual bool existsSession (std::string sessionId);

		rts2core::ConnNotify * getNotifyConnection () { return notifyConn; }

		void scriptProgress (double start, double end);

		virtual bool verifyDBUser (std::string username, std::string pass, rts2core::UserPermissions *userPermissions = NULL);

		/**
		 *
		 * @param v_name   value name
		 * @param oper     operator
		 * @param operand
		 *
		 * @throw rts2core::Error
		 */
		void doOpValue (const char *v_name, char oper, const char *operand);

		void clientNewDataConn (Connection *conn, int data_conn);
		void clientDataReceived (Connection *conn, DataAbstractRead *data);
		void clientFullDataReceived (Connection *conn, DataChannels *data);
		void clientExposureFailed (Connection *conn, int status);

		virtual void addExecutedPage () { numRequests->inc (); }

		/**
		 * Called when BB information were succesfully transmitted.
		 */
		void bbSend (double t);

#ifdef RTS2_HAVE_PGSQL
		void confirmSchedule (rts2db::Plan &plan);
#endif

	protected:
		virtual int info ();

		virtual int idle ();
#ifndef RTS2_HAVE_PGSQL
		virtual int willConnect (NetworkAddress * _addr);
#endif
		virtual int processOption (int in_opt);
		virtual int init ();

		virtual void signaledHUP ();

		virtual void connectionRemoved (rts2core::Connection *coon);

		virtual void asyncFinished (XmlRpcServerConnection *source);

		virtual void removeConnection (XmlRpcServerConnection *source);
	private:
		int rpcPort;
		const char *stateChangeFile;
		const char *defLabel;
		std::map <std::string, Session*> sessions;

		std::deque <Message> messages;

		std::list <XmlDevCameraClient *> camClis;

		std::vector <rts2json::Directory *> directories;

		Events events;

		rts2core::ValueInteger *numRequests;
		rts2core::ValueBool *send_emails;
		rts2core::ValueInteger *bbCadency;
		rts2core::ValueInteger *bbQueueSize;
		rts2core::ValueSelection *bbSelectorQueue;

		rts2core::ValueDouble *previewQuantiles;
		rts2core::ValueInteger *previewQuality;
		rts2core::ValueInteger *previewSize;
		rts2core::ValueInteger *previewPageSize;
		rts2core::ValueInteger *previewColorVariant;
		rts2core::ValueInteger *previewDefaultChannel;
		rts2core::ValueString *previewImageLabel;

		const char *bbQueueName;

		rts2core::ValueTime *bbLastSuccess;

		rts2core::ValueInteger *messageBufferSize;

#ifndef RTS2_HAVE_PGSQL
		const char *config_file;
#endif
		// user - login fields
		rts2core::UserLogins userLogins;

		std::string page_prefix;

		void sendBB ();
		void updateObservation (int obs_id, int plan_id);

		void reloadEventsFile ();

		int startTestScript ();

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
		SetValue _setValue;
		SetValueByType setValueByType;
		IncValue incValue;
		GetMessages _getMessages;

#ifdef RTS2_HAVE_PGSQL
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
#endif // RTS2_HAVE_PGSQL

#ifdef RTS2_HAVE_LIBJPEG
		rts2json::JpegImageRequest jpegRequest;
		rts2json::JpegPreview jpegPreview;
		rts2json::DownloadRequest downloadRequest;
		CurrentPosition current;
#ifdef RTS2_HAVE_PGSQL
		Graph graph;
		AltAzTarget altAzTarget;
#endif // RTS2_HAVE_PGSQL
		rts2json::ImageReq imageReq;
#endif /* RTS2_HAVE_LIBJPEG */
		rts2json::FitsImageRequest fitsRequest;
		rts2json::LibJavaScript javaScriptRequests;
		rts2json::LibCSS cssRequests;
		API api;
#ifdef RTS2_HAVE_PGSQL
		BBAPI bbapi;
		Auger auger;
		rts2json::Night night;
		rts2json::Observation observation;
		rts2json::Targets targets;
		rts2json::AddTarget addTarget;
		Plan plan;
#endif // RTS2_HAVE_PGSQL
		SwitchState switchState;
		Devices devices;

		rts2core::ConnNotify *notifyConn;

		std::list <const char *> testScripts;
		bool debugTestscript;

#ifdef RTS2_SSL
		std::string sslCert;
		std::string sslKey;
#endif
};


#ifndef RTS2_HAVE_PGSQL
bool verifyUser (std::string username, std::string pass, rts2core::UserPermissions *userPermissions = NULL);
#endif

};


#endif /* __RTS2_XMLRPCD__ */
