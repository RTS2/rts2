#include "soapH.h"
#include "rts2.nsmap"

#include "../utils/rts2config.h"
#include "../utilsdb/rts2devicedb.h"
#include "rts2soapcli.h"

#include <signal.h>

class Rts2SoapConn:public Rts2Conn
{
private:
  struct soap soap;
protected:
    virtual int acceptConn ();
public:
    Rts2SoapConn (int in_port, Rts2Block * in_master);
  virtual int init ();
};

Rts2SoapConn::Rts2SoapConn (int in_port, Rts2Block * in_master):
Rts2Conn (in_master)
{
  soap_init (&soap);
  sock = soap_bind (&soap, NULL, in_port, 100);
  if (sock < 0)
    soap_print_fault (&soap, stderr);
  setConnTimeout (-1);
  fprintf (stderr, "Socket connection successful: master socket = %d\n",
	   sock);
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
  s = soap_accept (&soap);
  fprintf (stderr, "Socket connection successful: slave socket = %d\n", s);
  if (s < 0)
    {
      soap_print_fault (&soap, stderr);
      exit (-1);
    }
  soap_serve (&soap);
  soap_end (&soap);
  setConnState (CONN_CONNECTING);
  return 0;
}

class Rts2Soapd:public Rts2DeviceDb
{
private:
  int soapPort;
  int servedRequest;
protected:
    virtual int processOption (int in_opt);
public:
    Rts2Soapd (int in_argc, char **in_argv);
    virtual ~ Rts2Soapd (void);
  virtual int init ();

  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual int ready ()
  {
    return 0;
  }

  virtual int baseInfo ()
  {
    return 0;
  }

  virtual int sendInfo (Rts2Conn * conn);
};

Rts2Soapd::Rts2Soapd (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_SOAP, "SOAP")
{
  soapPort = -1;
  servedRequest = 0;
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
    return -1;

  Rts2SoapConn *s_conn = new Rts2SoapConn (soapPort, this);
  ret - s_conn->init ();
  if (ret)
    return -1;
  ret = addConnection (s_conn);
  if (ret)
    return -1;

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
    default:
      cli = Rts2DeviceDb::createOtherType (conn, other_device_type);
    }
  return cli;
}

int
Rts2Soapd::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("serverd", servedRequest);
  return 0;
}

Rts2Soapd *soapd;

int
ns1__getEqu (struct soap *in_soap, ns1__getEquResponse & res)
{
  res.radec = soap_new_ns1__radec (in_soap, 1);
  soapd->postEvent (new Rts2Event (EVENT_SOAP_TEL_GETEQU, (void *) &res));
  return SOAP_OK;
}

int
ns1__getTelescope (struct soap *in_soap, char *name,
		   ns1__getTelescopeResponse & res)
{
  soapd->postEvent (new Rts2Event (EVENT_SOAP_TEL_GET, (void *) &res));
  return SOAP_OK;
}

int
ns1__getExec (struct soap *in_soap, ns1__getExecResponse & res)
{
  soapd->postEvent (new Rts2Event (EVENT_SOAP_EXEC_GETST, (void *) &res));
  return SOAP_OK;
}

int
ns1__getDome (struct soap *in_soap, ns1__getDomeResponse & res)
{
  soapd->postEvent (new Rts2Event (EVENT_SOAP_DOME_GETST, (void *) &res));
  return SOAP_OK;
}

void
killSignal (int sig)
{
  delete soapd;
  exit (0);
}

int
main (int argc, char **argv)
{
  int ret;
  soapd = new Rts2Soapd (argc, argv);
  signal (SIGTERM, killSignal);
  signal (SIGINT, killSignal);
  ret = soapd->init ();
  if (ret)
    return ret;
  ret = soapd->run ();
  delete soapd;
  return ret;
}
