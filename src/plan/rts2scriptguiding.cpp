#include "rts2scriptguiding.h"
#include "rts2execcli.h"
#include "../utils/rts2config.h"
#include "../writers/rts2image.h"
#include "../writers/rts2devclifoc.h"

Rts2ScriptElementGuiding::Rts2ScriptElementGuiding (Rts2Script * in_script,
						    float init_exposure,
						    int in_endSignal):
Rts2ScriptElement (in_script)
{
  expTime = init_exposure;
  endSignal = in_endSignal;
  defaultImgProccess[0] = '\0';
  processingState = NO_IMAGE;
  Rts2Config::instance ()->getString (in_script->getDefaultDevice (),
				      "sextractor", defaultImgProccess, 2000);
  obsId = -1;
  imgId = -1;

  ra_mult = 1;
  dec_mult = 1;

  last_ra = nan ("f");
  last_dec = nan ("f");

  min_change = 0.001;
  Rts2Config::instance ()->getDouble ("guiding", "minchange", min_change);

  bad_change = min_change * 4;
  Rts2Config::instance ()->getDouble ("guiding", "badchange", bad_change);
}

Rts2ScriptElementGuiding::~Rts2ScriptElementGuiding (void)
{
}

void
Rts2ScriptElementGuiding::checkGuidingSign (double &last, double &mult,
					    double act)
{
  // we are guiding in right direction, if sign changes
  if ((last < 0 && act > 0) || (last > 0 && act < 0))
    {
      last = act;
      return;
    }
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementGuiding::checkGuidingSign last: %f mult: %f act: %f bad_change:%f",
	  last, mult, act, bad_change);
  // try to detect sign change
  if ((fabs (act) > 2 * fabs (last)) && (fabs (act) > bad_change))
    {
      mult *= -1;
      // last change was with wrong sign
      last = act * -1;
    }
  else
    {
      last = act;
    }
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementGuiding::checkGuidingSign last: %f mult: %f act: %f",
	  last, mult, act);
}

void
Rts2ScriptElementGuiding::postEvent (Rts2Event * event)
{
  Rts2ConnFocus *focConn;
  Rts2Image *image;
  double star_x, star_y;
  double star_ra, star_dec, star_sep;
  float flux;
  int ret;
  switch (event->getType ())
    {
    case EVENT_GUIDING_DATA:
      focConn = (Rts2ConnFocus *) event->getArg ();
      image = focConn->getImage ();
      if (image->getObsId () == obsId && image->getImgId () == imgId)
	{
	  ret = image->getBrightestOffset (star_x, star_y, flux);
	  if (ret)
	    {
	      syslog (LOG_DEBUG,
		      "Rts2ScriptElementGuiding::postEvent EVENT_GUIDING_DATA failed (numStars: %i)",
		      image->sexResultNum);
	      processingState = FAILED;
	    }
	  else
	    {
	      // guide..
	      syslog (LOG_DEBUG,
		      "Rts2ScriptElementGuiding::postEvent EVENT_GUIDING_DATA %lf %lf",
		      star_x, star_y);
	      ret =
		image->getOffset (star_x, star_y, star_ra, star_dec,
				  star_sep);
	      if (ret)
		{
		  syslog (LOG_DEBUG,
			  "Rts2ScriptElementGuiding::postEvent EVENT_GUIDING_DATA getOffset %i",
			  ret);
		  processingState = NEED_IMAGE;
		}
	      else
		{
		  GuidingParams *pars;
		  syslog (LOG_DEBUG,
			  "Rts2ScriptElementGuiding::postEvent EVENT_GUIDING_DATA offsets ra: %f dec: %f",
			  star_ra, star_dec);
		  if (fabs (star_dec) > min_change)
		    {
		      checkGuidingSign (last_dec, dec_mult, star_dec);
		      pars =
			new GuidingParams (DIR_NORTH, star_dec * dec_mult);
		      script->getMaster ()->
			postEvent (new
				   Rts2Event (EVENT_TEL_START_GUIDING,
					      (void *) pars));
		      delete pars;
		    }
		  if (fabs (star_ra) > min_change)
		    {
		      checkGuidingSign (last_ra, ra_mult, star_ra);
		      pars = new GuidingParams (DIR_EAST, star_ra * ra_mult);
		      script->getMaster ()->
			postEvent (new
				   Rts2Event (EVENT_TEL_START_GUIDING,
					      (void *) pars));
		      delete pars;
		    }
		  processingState = NEED_IMAGE;
		}
	    }
	}
      // que us for processing
      script->getMaster ()->
	postEvent (new Rts2Event (EVENT_QUE_IMAGE, (void *) image));
      break;
    }
  Rts2ScriptElement::postEvent (event);
}

int
Rts2ScriptElementGuiding::nextCommand (Rts2DevClientCamera * camera,
				       Rts2Command ** new_command,
				       char new_device[DEVICE_NAME_SIZE])
{
  int ret = endSignal;
  if (endSignal == -1)
    return NEXT_COMMAND_NEXT;
  switch (processingState)
    {
    case WAITING_IMAGE:
      *new_command = NULL;
      return NEXT_COMMAND_NEXT;
    case NO_IMAGE:
      script->getMaster ()->
	postEvent (new Rts2Event (EVENT_SIGNAL_QUERY, (void *) &ret));
      if (ret != -1)
	return NEXT_COMMAND_NEXT;
    case NEED_IMAGE:
      *new_command =
	new Rts2CommandExposure (script->getMaster (), camera, EXP_LIGHT,
				 expTime);
      getDevice (new_device);
      processingState = WAITING_IMAGE;
      return NEXT_COMMAND_KEEP;
    case FAILED:
      return NEXT_COMMAND_NEXT;
    }
  // should not happen!
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementGuiding::nextCommand invalid state: %i",
	  processingState);
  return NEXT_COMMAND_NEXT;
}

int
Rts2ScriptElementGuiding::processImage (Rts2Image * image)
{
  int ret;
  Rts2ConnFocus *processor;
  syslog (LOG_DEBUG, "Rts2ScriptElementGuiding::processImage state: %i",
	  processingState);
  if (processingState != WAITING_IMAGE)
    {
      syslog (LOG_ERR,
	      "Rts2ScriptElementGuiding::processImage invalid processingState: %i",
	      processingState);
      processingState = FAILED;
      return -1;
    }
  obsId = image->getObsId ();
  imgId = image->getImgId ();
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementGuiding::processImage defaultImgProccess: %s",
	  defaultImgProccess);
  processor =
    new Rts2ConnFocus (script->getMaster (), image, defaultImgProccess,
		       EVENT_GUIDING_DATA);
  image->saveImage ();
  ret = processor->init ();
  if (ret < 0)
    {
      delete processor;
      processor = NULL;
      processingState = FAILED;
    }
  else
    {
      script->getMaster ()->addConnection (processor);
      processingState = NEED_IMAGE;
    }
  syslog (LOG_DEBUG,
	  "Rts2ConnImgProcess::processImage executed processor %i %p", ret,
	  processor);
  return 0;
}

int
Rts2ScriptElementGuiding::waitForSignal (int in_sig)
{
  if (in_sig == endSignal)
    {
      endSignal = -1;
      return 1;
    }
  return 0;
}

void
Rts2ScriptElementGuiding::cancelCommands ()
{

}
