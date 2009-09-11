#include "rts2soapcli.h"
#include "../utils/rts2command.h"
#include "../utils/rts2config.h"
#include "../utilsdb/target.h"
#include "imghdr.h"

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

			resTel->tel->target->ra = getConnection ()->getValueDouble ("RASC");
			resTel->tel->target->dec = getConnection ()->getValueDouble ("DECL");

			resTel->tel->mount->ra = getConnection ()->getValueDouble ("MNT_RA");
			resTel->tel->mount->dec = getConnection ()->getValueDouble ("MNT_DEC");

			resTel->tel->astrometry->ra = getConnection ()->getValueDouble ("CUR_RA");
			resTel->tel->astrometry->dec = getConnection ()->getValueDouble ("CUR_DEC");

			resTel->tel->err->ra = getConnection ()->getValueDouble ("RA_CORR");
			resTel->tel->err->dec = getConnection ()->getValueDouble ("DEC_CORR");

			break;
	}

	Rts2DevClientTelescope::postEvent (event);
}


void
Rts2DevClientTelescopeSoap::getObs (struct ln_lnlat_posn *obs)
{
	obs->lng = getConnection ()->getValueDouble ("LONG");
	obs->lat = getConnection ()->getValueDouble ("LAT");
}


void
Rts2DevClientTelescopeSoap::getEqu (struct ln_equ_posn *tel)
{
	tel->ra = getConnection ()->getValueDouble ("CUR_RA");
	tel->dec = getConnection ()->getValueDouble ("CUR_DEC");
}


double
Rts2DevClientTelescopeSoap::getLocalSiderealDeg ()
{
	return getConnection ()->getValueDouble ("siderealtime") * 15.0;
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
	struct soapExecNext *nexts;
	struct soapExecNow *nows;
	struct rts2__getExecResponse *g_res;
	switch (event->getType ())
	{
		case EVENT_SOAP_EXEC_GETST:
			gets = (soapExecGetst *) event->getArg ();
			g_res = gets->res;
			g_res->obsid = getConnection ()->getValueInteger ("obsid");
			fillTarget (getConnection ()->getValueInteger ("current"), gets->in_soap, g_res->current);
			fillTarget (getConnection ()->getValueInteger ("next"), gets->in_soap, g_res->next);
			break;
		case EVENT_SOAP_EXEC_SET_NEXT:
			nexts = (soapExecNext *) event->getArg ();
			queCommand (new Rts2CommandExecNext (getMaster (), nexts->next));
			fillTarget (nexts->next, nexts->in_soap, nexts->res->target);
			break;
		case EVENT_SOAP_EXEC_SET_NOW:
			nows = (soapExecNow *) event->getArg ();
			queCommand (new Rts2CommandExecNow (getMaster (), nows->now));
			fillTarget (nows->now, nows->in_soap, nows->res->target);
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
			res->dome->temp = getConnection ()->getValueDouble ("DOME_TMP");
			res->dome->humi = getConnection ()->getValueDouble ("DOME_HUM");
			res->dome->wind = getConnection ()->getValueDouble ("WINDSPED");
			break;
	}
}


Rts2DevClientFocusSoap::Rts2DevClientFocusSoap (Rts2Conn * in_connection):Rts2DevClientFocus
(in_connection)
{
}


void
Rts2DevClientFocusSoap::postEvent (Rts2Event * event)
{
	struct rts2__getFocusResponse *res;
	struct soapFocusSet *set;
	struct soapFocusStep *steps;
	switch (event->getType ())
	{
		case EVENT_SOAP_FOC_GET:
			res = (rts2__getFocusResponse *) event->getArg ();
			res->focus->pos = getConnection ()->getValueInteger ("pos");
			break;
		case EVENT_SOAP_FOC_SET:
			set = (soapFocusSet *) event->getArg ();
			queCommand (new Rts2CommandSetFocus (this, set->pos));
			break;
		case EVENT_SOAP_FOC_STEP:
			steps = (soapFocusStep *) event->getArg ();
			queCommand (new Rts2CommandChangeFocus (this, steps->step));
			break;
	}
}


Rts2DevClientImgprocSoap::Rts2DevClientImgprocSoap (Rts2Conn * in_connection):Rts2DevClientImgproc
(in_connection)
{
}


void
Rts2DevClientImgprocSoap::postEvent (Rts2Event * event)
{
	struct rts2__getImgprocResponse *res;
	switch (event->getType ())
	{
		case EVENT_SOAP_IMG_GET:
			res = (rts2__getImgprocResponse *) event->getArg ();
			res->imgproc->que = getConnection ()->getValueInteger ("que_size");
			res->imgproc->good = getConnection ()->getValueInteger ("good_images");
			res->imgproc->trash = getConnection ()->getValueInteger ("trash_images");
			res->imgproc->morning = getConnection ()->getValueInteger ("morning_images");
			break;
	}
}


Rts2DevClientFilterSoap::Rts2DevClientFilterSoap (Rts2Conn * in_connection):Rts2DevClientFilter
(in_connection)
{
}


void
Rts2DevClientFilterSoap::postEvent (Rts2Event * event)
{
	struct rts2__getFilterResponse *res;
	struct soapFilterSet *filters;
	switch (event->getType ())
	{
		case EVENT_SOAP_FW_GET:
			res = (rts2__getFilterResponse *) event->getArg ();
			res->filter->filter = getConnection ()->getValueInteger ("filter");
			break;
		case EVENT_SOAP_FW_SET:
			filters = (soapFilterSet *) event->getArg ();
			queCommand (new Rts2CommandFilter (this, filters->filter));
			break;
	}
}


Rts2DevClientCameraSoap::Rts2DevClientCameraSoap (Rts2Conn * in_connection):Rts2DevClientCamera
(in_connection)
{
}


void
Rts2DevClientCameraSoap::postEvent (Rts2Event * event)
{
	soapCameraGet *cam_get;
	soapCamerasGet *cams_get;
	class rts2__camera *cam = NULL;
	switch (event->getType ())
	{
		case EVENT_SOAP_CAMD_GET:
			cam_get = (soapCameraGet *) event->getArg ();
			cam = soap_new_rts2__camera (cam_get->in_soap, 1);

			cam_get->res->camera = cam;

			break;
		case EVENT_SOAP_CAMS_GET:
			cams_get = (soapCamerasGet *) event->getArg ();
			cam = soap_new_rts2__camera (cams_get->in_soap, 1);

		#ifdef WITH_FAST
			cams_get->res->cameras->camera.push_back (cam);
		#else
			cams_get->res->cameras->camera->push_back (cam);
		#endif

			break;
	}
	if (cam != NULL)
	{
		int status = getStatus ();
		cam->name = getName ();
		cam->exposure = getConnection ()->getValueDouble ("exposure");
		cam->focpos = getConnection ()->getValueInteger ("focpos");
		cam->temp = getConnection ()->getValueDouble ("CCD_TEMP");
		cam->filter = getConnection ()->getValueInteger ("filter");
		if (status & DEVICE_ERROR_MASK)
		{
			if (status & DEVICE_ERROR_MASK == DEVICE_ERROR_KILL)
				cam->status = rts2__cameraStatus__IDLE;
			else
				cam->status = rts2__cameraStatus__ERROR;
		}
		else
			switch (status)
			{
				case CAM_EXPOSING:
					if (getConnection ()->getValueInteger ("shutter") == SHUTTER_CLOSED)
						cam->status = rts2__cameraStatus__DARK;
					else
						cam->status = rts2__cameraStatus__IMAGE;
					break;
				case CAM_READING:
					cam->status = rts2__cameraStatus__READOUT;
					break;
				default:
					cam->status = rts2__cameraStatus__IDLE;
					break;
			}
	}
}


void
fillTarget (int in_tar_id, struct soap *in_soap, rts2__target * out_target)
{
	Target *an_target;
	struct ln_equ_posn pos;

	// that target does not exists..
	if (in_tar_id == -1)
	{
		nullTarget (out_target);
		return;
	}
	if (in_tar_id == -1)
	{
		nullTarget (out_target);
		return;
	}
	an_target = createTarget (in_tar_id, Rts2Config::instance ()->getObserver ());
	if (!an_target)
	{
		nullTarget (out_target);
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
		case TYPE_FLAT:
			out_target->type = "flat frames";
			break;
		case TYPE_DARK:
			out_target->type = "dark frames";
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


void
nullTarget (rts2__target * out_target)
{
	out_target->id = 0;
	out_target->type = "";
	out_target->name = "";
	out_target->radec = NULL;
	out_target->priority = nan ("f");
}
