#include "soaprts2Proxy.h"
#include "rts2.nsmap"

#include "../utils/rts2app.h"

#include <iostream>

class Rts2TestSoap:public Rts2App
{
  char *server;
protected:
    virtual int processOption (int in_opt);
public:
    Rts2TestSoap (int in_argc, char **in_argv);
  virtual int run ();
};

Rts2TestSoap::Rts2TestSoap (int in_argc, char **in_argv):
Rts2App (in_argc, in_argv)
{
  server = "http://localhost";
  addOption ('s', "server", 1, "soap server (default http://localhost");
}

int
Rts2TestSoap::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 's':
      server = optarg;
      break;
    default:
      return Rts2App::processOption (in_opt);
    }
  return 0;
}

int
Rts2TestSoap::run ()
{
  struct soap soap;
  struct ns1__getEquResponse res;
  struct ns1__getTelescopeResponse resTel;
  int ret;

  soap_init (&soap);

  ret = soap_call_ns1__getEqu (&soap, server, "", res);
  if (ret != SOAP_OK)
    {
      std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
      return ret;
    }
  std::cout << res.radec->ra << " " << res.radec->dec << std::endl;

  ret = soap_call_ns1__getTelescope (&soap, server, "", "T0", resTel);
  if (ret != SOAP_OK)
    {
      std::cerr << "Cannot connect to SOAP server: " << ret << std::endl;
      return ret;
    }
  std::cout
    << resTel.tel->target->ra << " " << resTel.tel->target->dec << std::endl
    << resTel.tel->mount->ra << " " << resTel.tel->mount->dec << std::endl
    << resTel.tel->astrometry->ra << " " << resTel.tel->astrometry->
    dec << std::endl << resTel.tel->err->ra << " " << resTel.tel->err->
    dec << std::endl;

  return ret;
}


Rts2TestSoap *app;

int
main (int argc, char **argv)
{
  int ret;
  app = new Rts2TestSoap (argc, argv);
  ret = app->init ();
  if (ret)
    return ret;
  ret = app->run ();
  delete app;
  return ret;
}
