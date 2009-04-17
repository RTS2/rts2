#include "soapH.h"
#include "rts2.nsmap"

#include "../utils/rts2config.h"
#include "../utils/rts2command.h"
#include "../utils/rts2connnosend.h"
#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/rts2messagedb.h"
#include "../utils/rts2block.h"
#include "rts2soapcli.h"

class Rts2SoapConn:public Rts2ConnNoSend
{
	private:
		struct soap *sp;
	protected:
		virtual int acceptConn ();
	public:
		Rts2SoapConn (int in_port, Rts2Block * in_master);
		virtual ~Rts2SoapConn (void);
		virtual int init ();
};

Rts2SoapConn::Rts2SoapConn (int in_port, Rts2Block * in_master):
Rts2ConnNoSend (in_master)
{
	sp = soap_new ();
	soap_init (sp);
	sock = soap_bind (sp, NULL, in_port, 100);
	if (sock < 0)
		soap_print_fault (sp, stderr);
	setConnTimeout (-1);
	logStream (MESSAGE_DEBUG)
		<< "Socket connection successful: master socket = " << sock << sendLog;
}


Rts2SoapConn::~Rts2SoapConn (void)
{
	soap_free (sp);
}


int
Rts2SoapConn::init ()
{
	if (sock < 0)
		return -1;
	setConnState (CONN_CONNECTING);
	return 0;
}


int
Rts2SoapConn::acceptConn ()
{
	int s;
	s = soap_accept (sp);
	if (s < 0)
	{
		soap_print_fault (sp, stderr);
		exit (-1);
	}
	soap_serve (sp);
	soap_end (sp);
	setConnState (CONN_CONNECTING);
	return 0;
}


class Rts2Soapd:public Rts2DeviceDb
{
	private:
		int soapPort;
	protected:
		virtual int processOption (int in_opt);
		virtual int reloadConfig ();
	public:
		Rts2Soapd (int in_argc, char **in_argv);
		virtual ~ Rts2Soapd (void);
		virtual int init ();

		virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
			int other_device_type);

		virtual void message (Rts2Message & msg);
};

Rts2Soapd::Rts2Soapd (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_SOAP, "SOAP")
{
	soapPort = -1;
	addOption ('o', "soap_port", 1,
		"soap port, if not specified, taken from config file (observatory, soap)");
}


Rts2Soapd::~Rts2Soapd (void)
{
}


int
Rts2Soapd::processOption (int in_opt)
{
	switch (in_opt)
	{
		case 'o':
			soapPort = atoi (optarg);
			break;
		default:
			return Rts2DeviceDb::processOption (in_opt);
	}
	return 0;
}


int
Rts2Soapd::reloadConfig ()
{
	return Rts2DeviceDb::reloadConfig ();
}


int
Rts2Soapd::init ()
{
	int ret;
	ret = Rts2DeviceDb::init ();
	if (ret)
		return ret;

	Rts2Config *config = Rts2Config::instance ();

	if (soapPort == -1)
		config->getInteger ("observatory", "soap", soapPort);

	if (soapPort == -1)
	{
		std::
			cerr <<
			"Rts2Soapd::init soap port was not specified, cannot start soapd." <<
			std::endl;
		return -1;
	}

	setMessageMask (MESSAGE_MASK_ALL);

	Rts2SoapConn *s_conn = new Rts2SoapConn (soapPort, this);
	ret = s_conn->init ();
	if (ret)
		return -1;
	addConnection (s_conn);

	return 0;
}


Rts2DevClient *
Rts2Soapd::createOtherType (Rts2Conn * conn, int other_device_type)
{
	Rts2DevClient *cli;
	switch (other_device_type)
	{
		case DEVICE_TYPE_MOUNT:
			cli = new Rts2DevClientTelescopeSoap (conn);
			break;
		case DEVICE_TYPE_EXECUTOR:
			cli = new Rts2DevClientExecutorSoap (conn);
			break;
		case DEVICE_TYPE_DOME:
			cli = new Rts2DevClientDomeSoap (conn);
			break;
		case DEVICE_TYPE_CCD:
			cli = new Rts2DevClientCameraSoap (conn);
			break;
		case DEVICE_TYPE_FOCUS:
			cli = new Rts2DevClientFocusSoap (conn);
			break;
		case DEVICE_TYPE_FW:
			cli = new Rts2DevClientFilterSoap (conn);
			break;
		case DEVICE_TYPE_IMGPROC:
			cli = new Rts2DevClientImgprocSoap (conn);
			break;
		default:
			cli = Rts2DeviceDb::createOtherType (conn, other_device_type);
	}
	return cli;
}


void
Rts2Soapd::message (Rts2Message & msg)
{
	if (msg.isNotDebug ())
	{
		Rts2MessageDB msgDB (msg);
		msgDB.insertDB ();
	}
}


Rts2Soapd *soapd;

int
rts2__getEqu (struct soap *in_soap, rts2__getEquResponse & res)
{
	res.radec = soap_new_rts2__radec (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_TEL_GETEQU, (void *) &res));
	return SOAP_OK;
}


int
rts2__getAlt (struct soap *in_soap, rts2__getAltResponse & res)
{
	res.altaz = soap_new_rts2__altaz (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_TEL_GETALT, (void *) &res));
	return SOAP_OK;
}


int
rts2__getTelescope (struct soap *in_soap, std::string name,
rts2__getTelescopeResponse & res)
{
	res.tel = soap_new_rts2__telescope (in_soap, 1);
	res.tel->target = soap_new_rts2__radec (in_soap, 1);
	res.tel->mount = soap_new_rts2__radec (in_soap, 1);
	res.tel->astrometry = soap_new_rts2__radec (in_soap, 1);
	res.tel->err = soap_new_rts2__radec (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_TEL_GET, (void *) &res));
	return SOAP_OK;
}


int
rts2__getExec (struct soap *in_soap, rts2__getExecResponse & res)
{
	struct soapExecGetst gets;
	res.current = soap_new_rts2__target (in_soap, 1);
	res.next = soap_new_rts2__target (in_soap, 1);
	res.obsid = 0;
	nullTarget (res.current);
	nullTarget (res.next);
	gets.res = &res;
	gets.in_soap = in_soap;
	soapd->postEvent (new Rts2Event (EVENT_SOAP_EXEC_GETST, (void *) &gets));
	return SOAP_OK;
}


int
rts2__getTarget (struct soap *in_soap, unsigned int id,
rts2__getTargetResponse & res)
{
	res.target = soap_new_rts2__target (in_soap, 1);
	fillTarget (id, in_soap, res.target);
	return SOAP_OK;
}


int
rts2__setNext (struct soap *in_soap, unsigned int id,
rts2__setNextResponse & res)
{
	struct soapExecNext setN;
	res.target = soap_new_rts2__target (in_soap, 1);
	nullTarget (res.target);

	setN.next = id;
	setN.res = &res;
	setN.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_EXEC_SET_NEXT, (void *) &setN));
	return SOAP_OK;
}


int
rts2__setNow (struct soap *in_soap, unsigned int id,
rts2__setNowResponse & res)
{
	struct soapExecNow setN;
	res.target = soap_new_rts2__target (in_soap, 1);
	nullTarget (res.target);

	setN.now = id;
	setN.res = &res;
	setN.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_EXEC_SET_NOW, (void *) &setN));
	return SOAP_OK;
}


int
rts2__getDome (struct soap *in_soap, rts2__getDomeResponse & res)
{
	res.dome = soap_new_rts2__dome (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_DOME_GETST, (void *) &res));
	return SOAP_OK;
}


int
rts2__getFocus (struct soap *in_soap, rts2__getFocusResponse & res)
{
	res.focus = soap_new_rts2__focus (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_FOC_GET, (void *) &res));
	return SOAP_OK;
}


int
rts2__setFocSet (struct soap *in_soap, unsigned int pos,
rts2__setFocSetResponse & res)
{
	struct soapFocusSet setF;
	res.focus = soap_new_rts2__focus (in_soap, 1);

	setF.pos = pos;
	setF.res = &res;
	setF.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_FOC_SET, (void *) &setF));
	return SOAP_OK;
}


int
rts2__setFocStep (struct soap *in_soap, unsigned int step,
rts2__setFocStepResponse & res)
{
	struct soapFocusStep stepF;
	res.focus = soap_new_rts2__focus (in_soap, 1);

	stepF.step = step;
	stepF.res = &res;
	stepF.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_FOC_STEP, (void *) &stepF));
	return SOAP_OK;
}


int
rts2__getFilter (struct soap *in_soap, rts2__getFilterResponse & res)
{
	res.filter = soap_new_rts2__filter (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_FW_GET, (void *) &res));
	return SOAP_OK;
}


int
rts2__setFilter (struct soap *in_soap, unsigned int id,
rts2__setFilterResponse & res)
{
	struct soapFilterSet setF;
	res.filter = soap_new_rts2__filter (in_soap, 1);

	setF.filter = id;
	setF.res = &res;
	setF.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_FW_SET, (void *) &setF));
	return SOAP_OK;
}


int
rts2__getImgproc (struct soap *in_soap, rts2__getImgprocResponse & res)
{
	res.imgproc = soap_new_rts2__imgproc (in_soap, 1);
	soapd->postEvent (new Rts2Event (EVENT_SOAP_IMG_GET, (void *) &res));
	return SOAP_OK;
}


int
rts2__getCamera (struct soap *in_soap, rts2__getCameraResponse & res)
{
	soapCameraGet cam_get;

	cam_get.res = &res;
	cam_get.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_CAMD_GET, (void *) &cam_get));
	return SOAP_OK;
}


int
rts2__getCameras (struct soap *in_soap, rts2__getCamerasResponse & res)
{
	soapCamerasGet cams_get;
	res.cameras = soap_new_rts2__cameras (in_soap, 1);
	res.cameras->camera =
		#ifdef WITH_FAST
		*soap_new_std__vectorTemplateOfPointerTorts2__camera (in_soap, 1);
	#else
	soap_new_std__vectorTemplateOfPointerTorts2__camera (in_soap, 1);
	#endif

	cams_get.res = &res;
	cams_get.in_soap = in_soap;

	soapd->postEvent (new Rts2Event (EVENT_SOAP_CAMS_GET, (void *) &cams_get));
	return SOAP_OK;
}


int
rts2__getCentrald (struct soap *in_soap, rts2__getCentraldResponse & res)
{
	int state = soapd->getMasterState ();
	if (state == SERVERD_SOFT_OFF || state == SERVERD_HARD_OFF)
	{
		res.system = rts2__system__OFF;
		res.daytime = rts2__daytime__DAY;
		return SOAP_OK;
	}
	if ((state & SERVERD_STANDBY_MASK) == SERVERD_STANDBY)
	{
		res.system = rts2__system__STANDBY;
	}
	else
	{
		res.system = rts2__system__ON;
	}
	switch (state & SERVERD_STATUS_MASK)
	{
		case SERVERD_DAY:
			res.daytime = rts2__daytime__DAY;
			break;
		case SERVERD_EVENING:
			res.daytime = rts2__daytime__EVENING;
			break;
		case SERVERD_DUSK:
			res.daytime = rts2__daytime__DUSK;
			break;
		case SERVERD_NIGHT:
			res.daytime = rts2__daytime__NIGHT;
			break;
		case SERVERD_DAWN:
			res.daytime = rts2__daytime__DAWN;
			break;
		case SERVERD_MORNING:
			res.daytime = rts2__daytime__MORNING;
			break;
	}
	return SOAP_OK;
}


int
rts2__getDeviceList (struct soap *in_soap, rts2__getDeviceListResponse & res)
{
	res.devices = soap_new_rts2__devices (in_soap, 1);
	res.devices->device =
	#ifdef WITH_FAST
	*soap_new_std__vectorTemplateOfPointerTorts2__device (in_soap, 1);
	#else
	soap_new_std__vectorTemplateOfPointerTorts2__device (in_soap, 1);
	#endif

	connections_t::iterator iter;

	for (iter = soapd->getConnections ()->begin (); iter != soapd->getConnections ()->end (); iter++)
	{
		Rts2Conn *conn = *iter;
		const char *name = conn->getName ();
		rts2__device *dev = soap_new_rts2__device (in_soap, 1);
		dev->name = name;
		#ifdef WITH_FAST
		res.devices->device.push_back (dev);
		#else
		res.devices->device->push_back (dev);
		#endif
	}

	return SOAP_OK;
}


int
rts2__setCentrald (struct soap *in_soap, enum rts2__system system, struct rts2__setCentraldResponse &res)
{
	const char *cmd;
	switch (system)
	{
		case rts2__system__OFF:
			cmd = "off";
			break;
		case rts2__system__STANDBY:
			cmd = "standby";
			break;
		case rts2__system__ON:
			cmd = "on";
			break;
		default:
			return SOAP_NO_METHOD;
	}
	connections_t::iterator iter;
	for (iter = soapd->getCentraldConns ()->begin (); iter != soapd->getCentraldConns ()->end (); iter++)
	{
		(*iter)->queCommand (new Rts2Command (soapd, cmd));
	}
	return SOAP_OK;
}


int
main (int argc, char **argv)
{
	int ret;
	soapd = new Rts2Soapd (argc, argv);
	ret = soapd->run ();
	delete soapd;
	return ret;
}
