#include "rts2soapcli.h"
#include "../utilsdb/target.h"

Rts2DevClientTelescopeSoap::Rts2DevClientTelescopeSoap (Rts2Conn * in_connection):Rts2DevClientTelescope
  (in_connection)
{
}

void
Rts2DevClientTelescopeSoap::postEvent (Rts2Event * event)
{
  struct rts2__getEquResponse *resEqu;
  struct rts2__getAltResponse *resAlt;
  struct rts2__getTelescopeResponse *resTel;
  struct ln_hrz_posn altaz;
  struct ln_equ_posn radec;

  getEqu (&radec);
  getAltAz (&altaz);

  switch (event->getType ())
    {
    case EVENT_SOAP_TEL_GETEQU:
      resEqu = (rts2__getEquResponse *) event->getArg ();

      resEqu->radec->ra = radec.ra;
      resEqu->radec->dec = radec.dec;

      break;
    case EVENT_SOAP_TEL_GETALT:
      resAlt = (rts2__getAltResponse *) event->getArg ();

      resAlt->altaz->az = altaz.az;
      resAlt->altaz->alt = altaz.alt;

      break;
    case EVENT_SOAP_TEL_GET:
      resTel = (rts2__getTelescopeResponse *) event->getArg ();

      resTel->tel->target->ra = getValueDouble ("ra_tar");
      resTel->tel->target->dec = getValueDouble ("dec_tar");

      resTel->tel->mount->ra = getValueDouble ("ra_tel");
      resTel->tel->mount->dec = getValueDouble ("dec_tel");

      resTel->tel->astrometry->ra = getValueDouble ("ra");
      resTel->tel->astrometry->dec = getValueDouble ("dec");

      resTel->tel->err->ra = getValueDouble ("ra_corr");
      resTel->tel->err->dec = getValueDouble ("dec_corr");

      break;
    }

  Rts2DevClientTelescope::postEvent (event);
}

void
Rts2DevClientTelescopeSoap::getObs (struct ln_lnlat_posn *obs)
{
  obs->lng = getValueDouble ("longtitude");
  obs->lat = getValueDouble ("latitude");
}

void
Rts2DevClientTelescopeSoap::getEqu (struct ln_equ_posn *tel)
{
  tel->ra = getValueDouble ("ra");
  tel->dec = getValueDouble ("dec");
}

double
Rts2DevClientTelescopeSoap::getLocalSiderealDeg ()
{
  return getValueDouble ("siderealtime") * 15.0;
}

void
Rts2DevClientTelescopeSoap::getAltAz (struct ln_hrz_posn *hrz)
{
  struct ln_equ_posn pos;
  struct ln_lnlat_posn obs;
  double gst;

  getEqu (&pos);
  getObs (&obs);
  gst = getLocalSiderealDeg () - obs.lng;
  gst = ln_range_degrees (gst) / 15.0;

  ln_get_hrz_from_equ_sidereal_time (&pos, &obs, gst, hrz);
}

Rts2DevClientExecutorSoap::Rts2DevClientExecutorSoap (Rts2Conn * in_connection):Rts2DevClientExecutor
  (in_connection)
{
}

void
Rts2DevClientExecutorSoap::postEvent (Rts2Event * event)
{
  struct soapExecGetst *gets;
  struct rts2__getExecResponse *res;
  switch (event->getType ())
    {
    case EVENT_SOAP_EXEC_GETST:
      gets = (soapExecGetst *) event->getArg ();
      res = gets->res;
      res->obsid = getValueInteger ("obsid");
      fillTarget (getValueInteger ("current"), gets->in_soap, res->current);
      fillTarget (getValueInteger ("next"), gets->in_soap, res->next);
      break;
    }

  Rts2DevClientExecutor::postEvent (event);
}

Rts2DevClientDomeSoap::Rts2DevClientDomeSoap (Rts2Conn * in_connection):Rts2DevClientDome
  (in_connection)
{
}

void
Rts2DevClientDomeSoap::postEvent (Rts2Event * event)
{
  struct rts2__getDomeResponse *res;
  switch (event->getType ())
    {
    case EVENT_SOAP_DOME_GETST:
      res = (rts2__getDomeResponse *) event->getArg ();
      res->temp = getValueDouble ("temperature");
      res->humi = getValueDouble ("humidity");
      res->wind = getValueDouble ("windspeed");
      break;
    }
}

void
fillTarget (int in_tar_id, struct soap *in_soap, rts2__target * out_target)
{
  Target *an_target;
  struct ln_equ_posn pos;

  an_target = createTarget (in_tar_id);
  if (!an_target)
    {
      out_target->id = 0;
      out_target->type = "";
      out_target->name = "";
      out_target->radec = NULL;
      out_target->priority = nan ("f");
      return;
    }

  out_target->id = an_target->getTargetID ();
  out_target->name = std::string (an_target->getTargetName ());
  switch (an_target->getTargetType ())
    {
    case TYPE_OPORTUNITY:
      out_target->type = "oportunity";
      break;
    case TYPE_GRB:
      out_target->type = "grb";
      break;
    case TYPE_GRB_TEST:
      out_target->type = "grb_test";
      break;
    case TYPE_ELLIPTICAL:
      out_target->type = "solar system body";
      break;
    default:
    case TYPE_UNKNOW:
      out_target->type = "unknown";
      break;
    }
  an_target->getPosition (&pos);
  out_target->radec = soap_new_rts2__radec (in_soap, 1);
  out_target->radec->ra = pos.ra;
  out_target->radec->dec = pos.dec;
  out_target->priority = an_target->getBonus ();
}
