#include "rts2soapcli.h"
#include "../utilsdb/target.h"

Rts2DevClientTelescopeSoap::Rts2DevClientTelescopeSoap (Rts2Conn * in_connection):Rts2DevClientTelescope
  (in_connection)
{
}

void
Rts2DevClientTelescopeSoap::postEvent (Rts2Event * event)
{
  struct rts2__getEquResponse *res;
  struct rts2__getTelescopeResponse *resTel;
  switch (event->getType ())
    {
    case EVENT_SOAP_TEL_GETEQU:
      res = (rts2__getEquResponse *) event->getArg ();
      res->radec->ra = getValueDouble ("ra");
      res->radec->dec = getValueDouble ("dec");
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

Rts2DevClientExecutorSoap::Rts2DevClientExecutorSoap (Rts2Conn * in_connection):Rts2DevClientExecutor
  (in_connection)
{
}

void
Rts2DevClientExecutorSoap::fillTarget (int in_tar_id,
				       rts2__target * out_target)
{
  Target *an_target;
  struct ln_equ_posn pos;
  const char *targetName;

  an_target = createTarget (in_tar_id);
  if (!an_target)
    {
      out_target->id = 0;
      out_target->type = NULL;
      out_target->name = NULL;
      out_target->radec = NULL;
      return;
    }

  out_target->id = an_target->getTargetID ();
  targetName = an_target->getTargetName ();
  out_target->name = (char *) malloc (strlen (targetName) + 1);
  strcpy (out_target->name, targetName);
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
    default:
    case TYPE_UNKNOW:
      out_target->type = "unknow";
      break;
    }
  an_target->getPosition (&pos);
  out_target->radec = new rts2__radec ();
  out_target->radec->ra = pos.ra;
  out_target->radec->dec = pos.dec;
}

void
Rts2DevClientExecutorSoap::postEvent (Rts2Event * event)
{
  struct rts2__getExecResponse *res;
  switch (event->getType ())
    {
    case EVENT_SOAP_EXEC_GETST:
      res = (rts2__getExecResponse *) event->getArg ();
      fillTarget (getValueInteger ("current"), res->current);
      fillTarget (getValueInteger ("next"), res->next);
      fillTarget (getValueInteger ("priority"), res->priority);
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
