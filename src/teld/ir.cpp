#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <libnova/libnova.h>
#include <mcheck.h>

#include "status.h"
#include "telescope.h"

#include <cstdio>
#include <cstdarg>
#include <opentpl/client.h>

using namespace OpenTPL;

#define LONGITUDE -3.3847222
#define LATITUDE 37.064167
#define ALTITUDE 2896
#define LOOP_DELAY 500000

class Rts2DevTelescopeIr:public Rts2DevTelescope
{
private:
  char *ir_ip;
  int ir_port;
  Client *tplc;
  int timeout;

  struct ln_equ_posn target;

    template < typename T > int tpl_get (const char *name, T & val,
					 int *status);
    template < typename T > int tpl_set (const char *name, T val,
					 int *status);
    template < typename T > int tpl_setw (const char *name, T val,
					  int *status);

  double get_loc_sid_time ();

public:
    Rts2DevTelescopeIr (int argc, char **argv);
    virtual ~ Rts2DevTelescopeIr (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int startMove (double tar_ra, double tar_dec);
  virtual int isMoving ();
  virtual int endMove ();
  virtual int startPark ();
  virtual int isParking ();
  virtual int endPark ();
  virtual int stop ();
};

template < typename T > int
Rts2DevTelescopeIr::tpl_get (const char *name, T & val, int *status)
{
  int cstatus;

  if (!*status)
    {
      Request & r = tplc->Get (name, false);
      cstatus = r.Wait ();

      if (cstatus != TPLC_OK)
	{
	  printf (" error %i\n", cstatus);
	  *status = 1;
	}

      if (!*status)
	{
	  RequestAnswer & answr = r.GetAnswer ();

	  if (answr.begin ()->second.first == TPL_OK)
	    val = (T) answr.begin ()->second.second;
	  else
	    *status = 2;
	}
      r.Dispose ();
    }
  return *status;
}

template < typename T > int
Rts2DevTelescopeIr::tpl_set (const char *name, T val, int *status)
{
  int cstatus;

  if (!*status)
    {
      tplc->Set (name, Value (val), true);	// change to set...?
      //Request &r = tplc->Set(name, Value(val), false ); // change to set...?
      //cstatus = r.Wait();

      if (cstatus != TPLC_OK)
	{
	  printf (" error %i\n", cstatus);
	  *status = 1;
	}
      //r.Dispose();
    }
  return *status;
}

template < typename T > int
Rts2DevTelescopeIr::tpl_setw (const char *name, T val, int *status)
{
  int cstatus;

  if (!*status)
    {
      Request & r = tplc->Set (name, Value (val), false);	// change to set...?
      cstatus = r.Wait ();

      if (cstatus != TPLC_OK)
	{
	  printf (" error %i\n", cstatus);
	  *status = 1;
	}
      r.Dispose ();
    }
  return *status;
}


Rts2DevTelescopeIr::Rts2DevTelescopeIr (int argc, char **argv):Rts2DevTelescope (argc,
		  argv)
{
  ir_ip = NULL;
  ir_port = 0;
  tplc = NULL;
  addOption ('I', "ir_ip", 1, "IR TCP/IP address");
  addOption ('P', "ir_port", 1, "IR TCP/IP port number");

  strcpy (telType, "BOOTES_IR");
  strcpy (telSerialNumber, "001");
}

Rts2DevTelescopeIr::~Rts2DevTelescopeIr (void)
{

}

int
Rts2DevTelescopeIr::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      ir_ip = optarg;
      break;
    case 'P':
      ir_port = atoi (optarg);
      break;
    default:
      return Rts2DevTelescope::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevTelescopeIr::init ()
{
  int ret;
  ret = Rts2DevTelescope::init ();
  if (ret)
    return ret;

  if (!ir_ip || !ir_port)
    {
      fprintf (stderr, "Invalid port or IP address of mount controller PC\n");
      return -1;
    }

  tplc = new Client (ir_ip, ir_port, 2000);

  syslog (LOG_DEBUG,
	  "Status: ConnID = %i, connected: %s, authenticated %s, Welcome Message = %s",
	  tplc->ConnID (), (tplc->IsConnected ()? "yes" : "no"),
	  (tplc->IsAuth ()? "yes" : "no"), tplc->WelcomeMessage ().c_str ());

  // are we connected ?
  if (!tplc->IsAuth () || !tplc->IsConnected ())
    {
      syslog (LOG_ERR, "Connection to server failed");
      return -1;
    }
  return 0;
}

int
Rts2DevTelescopeIr::ready ()
{
  return !tplc->IsConnected ();
}

int
Rts2DevTelescopeIr::baseInfo ()
{
  telLongtitude = LONGITUDE;
  telLatitude = LATITUDE;
  telAltitude = ALTITUDE;
  telParkDec = 0;
  return 0;
}

double
Rts2DevTelescopeIr::get_loc_sid_time ()
{
  return 15 * ln_get_apparent_sidereal_time (ln_get_julian_from_sys ()) +
    LONGITUDE;
}

int
Rts2DevTelescopeIr::info ()
{
  double zd;
  int status = 0;

  if (!(tplc->IsAuth () && tplc->IsConnected ()))
    return -1;

  status = tpl_get ("POINTING.CURRENT.RA", telRa, &status);
  if (status)
    return -1;
  telRa *= 15.0;
  status = tpl_get ("POINTING.CURRENT.DEC", telDec, &status);
  if (status)
    return -1;

  telSiderealTime = get_loc_sid_time ();
  telLocalTime = 0;

  status = tpl_get ("ZD.REALPOS", zd, &status);
  if (status)
    return -1;

  telFlip = (zd < 0);

  telAxis[0] = nan ("f");
  telAxis[1] = nan ("f");

  return 0;
}

int
Rts2DevTelescopeIr::startMove (double ra, double dec)
{
  int status = 0;

  target.ra = ra;
  target.dec = dec;

  tpl_setw ("POINTING.TARGET.RA", ra / 15.0, &status);
  tpl_setw ("POINTING.TARGET.DEC", dec, &status);
  tpl_setw ("POINTING.TRACK", 4, &status);

  timeout = 0;
  return 0;
}

int
Rts2DevTelescopeIr::isMoving ()
{
  double diffRa, diffDec;
  ln_equ_posn curr;

  timeout++;

  // finish due to error
  if (timeout > 20000)
    {
      return -1;
    }

  info ();

  curr.ra = telRa;
  curr.dec = telDec;

  if (ln_get_angular_separation (&target, &curr) > 2)
    {
      return 1000;
    }
  // bridge 20 sec timeout
  return -2;
}

int
Rts2DevTelescopeIr::endMove ()
{
  return 0;
}

int
Rts2DevTelescopeIr::startPark ()
{
  return startMove (get_loc_sid_time () - 30, 40);
}

int
Rts2DevTelescopeIr::isParking ()
{
  return isMoving ();
}

int
Rts2DevTelescopeIr::endPark ()
{
  return 0;
}

int
Rts2DevTelescopeIr::stop ()
{
  info ();
  startMove (telRa, telDec);
  return 0;
}

int
main (int argc, char **argv)
{
  mtrace ();

  Rts2DevTelescopeIr *device = new Rts2DevTelescopeIr (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize telescope bridge - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
