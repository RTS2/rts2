#include "rts2scriptelement.h"

#include "../utils/rts2config.h"
#include "../writers/rts2imagedb.h"

Rts2ScriptElement::Rts2ScriptElement (Rts2Script * in_script)
{
  script = in_script;
}

Rts2ScriptElement::~Rts2ScriptElement ()
{
}

void
Rts2ScriptElement::getDevice (char new_device[DEVICE_NAME_SIZE])
{
  script->getDefaultDevice (new_device);
}

int
Rts2ScriptElement::processImage (Rts2Image * image)
{
  return -1;
}

int
Rts2ScriptElement::defnextCommand (Rts2DevClient * client,
				   Rts2Command ** new_command,
				   char new_device[DEVICE_NAME_SIZE])
{
  return NEXT_COMMAND_NEXT;
}

int
Rts2ScriptElement::nextCommand (Rts2DevClientCamera * camera,
				Rts2Command ** new_command,
				char new_device[DEVICE_NAME_SIZE])
{
  return defnextCommand (camera, new_command, new_device);
}

int
Rts2ScriptElement::nextCommand (Rts2DevClientPhot * phot,
				Rts2Command ** new_command,
				char new_device[DEVICE_NAME_SIZE])
{
  return defnextCommand (phot, new_command, new_device);
}

Rts2ScriptElementExpose::Rts2ScriptElementExpose (Rts2Script * in_script, float in_expTime):
Rts2ScriptElement (in_script)
{
  expTime = in_expTime;
}

int
Rts2ScriptElementExpose::nextCommand (Rts2DevClientCamera * camera,
				      Rts2Command ** new_command,
				      char new_device[DEVICE_NAME_SIZE])
{
  *new_command =
    new Rts2CommandExposure (script->getMaster (), camera, EXP_LIGHT,
			     expTime);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementDark::Rts2ScriptElementDark (Rts2Script * in_script, float in_expTime):Rts2ScriptElement
  (in_script)
{
  expTime = in_expTime;
}

int
Rts2ScriptElementDark::nextCommand (Rts2DevClientCamera * camera,
				    Rts2Command ** new_command,
				    char new_device[DEVICE_NAME_SIZE])
{
  *new_command =
    new Rts2CommandExposure (script->getMaster (), camera, EXP_DARK, expTime);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementChange::Rts2ScriptElementChange (Rts2Script * in_script, double in_ra, double in_dec):Rts2ScriptElement
  (in_script)
{
  ra = in_ra;
  dec = in_dec;
}

int
Rts2ScriptElementChange::defnextCommand (Rts2DevClient * client,
					 Rts2Command ** new_command,
					 char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandChange (script->getMaster (), ra, dec);
  strcpy (new_device, "TX");
  return 0;
}

Rts2ScriptElementWait::Rts2ScriptElementWait (Rts2Script * in_script):Rts2ScriptElement
  (in_script)
{
}

int
Rts2ScriptElementWait::defnextCommand (Rts2DevClient * client,
				       Rts2Command ** new_command,
				       char new_device[DEVICE_NAME_SIZE])
{
  return NEXT_COMMAND_CHECK_WAIT;
}

Rts2ScriptElementFilter::Rts2ScriptElementFilter (Rts2Script * in_script, int in_filter):Rts2ScriptElement
  (in_script)
{
  filter = in_filter;
}

int
Rts2ScriptElementFilter::nextCommand (Rts2DevClientCamera * camera,
				      Rts2Command ** new_command,
				      char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandFilter (script->getMaster (), filter);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementAcquire::Rts2ScriptElementAcquire (Rts2Script * in_script, double in_precision, float in_expTime):Rts2ScriptElement
  (in_script)
{
  reqPrecision = in_precision;
  lastPrecision = nan ("f");
  expTime = in_expTime;
  processor = NULL;
  processingState = NEED_IMAGE;
  Rts2Config::instance ()->getString ("imgproc", "astrometry",
				      defaultImgProccess, 2000);
  obsId = -1;
  imgId = -1;
}

void
Rts2ScriptElementAcquire::postEvent (Rts2Event * event)
{
  Rts2ImageDb *image;
  switch (event->getType ())
    {
    case EVENT_OK_ASTROMETRY:
      image = (Rts2ImageDb *) event->getArg ();
      if (image->getObsId () == obsId && image->getImgId () == imgId)
	{
	  // we get bellow required errror?
	  double img_prec = image->getPrecision ();
	  if (isnan (img_prec))
	    {
	      processingState = FAILED;
	      break;
	    }
	  if (img_prec <= reqPrecision)
	    {
	      processingState = PRECISION_OK;
	    }
	  else
	    {
	      // test if precision is better then previous one..
	      if (isnan (lastPrecision) || img_prec < lastPrecision / 2)
		{
		  processingState = PRECISION_BAD;
		  lastPrecision = img_prec;
		}
	      else
		{
		  processingState = FAILED;
		}
	    }
	}
      break;
    case EVENT_NOT_ASTROMETRY:
      image = (Rts2ImageDb *) event->getArg ();
      if (image->getObsId () == obsId && image->getImgId () == imgId)
	{
	  processingState = FAILED;
	}
      break;
    }
  Rts2ScriptElement::postEvent (event);
}

int
Rts2ScriptElementAcquire::nextCommand (Rts2DevClientCamera * camera,
				       Rts2Command ** new_command,
				       char new_device[DEVICE_NAME_SIZE])
{
  // this code have to coordinate efforts to reach desired target with given precission
  // based on internal state, it have to take exposure, assure that image will be processed,
  // wait till astrometry ended, based on astrometry results decide if to
  // proceed with acqusition or if to cancel acqusition
  switch (processingState)
    {
    case NEED_IMAGE:
      *new_command =
	new Rts2CommandExposure (script->getMaster (), camera, EXP_LIGHT,
				 expTime);
      getDevice (new_device);
      processingState = WAITING_IMAGE;
      return NEXT_COMMAND_ACQUSITION_IMAGE;
    case WAITING_IMAGE:
    case WAITING_ASTROMETRY:
      return NEXT_COMMAND_WAITING;
    case FAILED:
      return NEXT_COMMAND_PRECISION_FAILED;
    case PRECISION_OK:
      return NEXT_COMMAND_PRECISION_OK;
    case PRECISION_BAD:
      processingState = NEED_IMAGE;
      // end of movemen will call nextCommand, as we should have waiting set to WAIT_MOVE
      return NEXT_COMMAND_RESYNC;
    }
  // that should not happen!
  syslog (LOG_ERR,
	  "Rts2ScriptElementAcquire::nextCommand unexpected processing state %i",
	  processingState);
  return NEXT_COMMAND_NEXT;
}

int
Rts2ScriptElementAcquire::processImage (Rts2Image * image)
{
  int ret;
  if (processingState != WAITING_IMAGE)
    {
      syslog (LOG_ERR,
	      "Rts2ScriptElementAcquire::processImage invalid processingState: %i",
	      processingState);
      return -1;
    }
  obsId = image->getObsId ();
  imgId = image->getImgId ();
  processor =
    new Rts2ConnImgProcess (script->getMaster (), NULL, defaultImgProccess,
			    image->getImageName ());
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
      processingState = WAITING_ASTROMETRY;
      script->getMaster ()->postEvent (new Rts2Event (EVENT_ACQUIRE_WAIT));
    }
  return 0;
}

Rts2ScriptElementWaitAcquire::Rts2ScriptElementWaitAcquire (Rts2Script * in_script):Rts2ScriptElement
  (in_script)
{
}

int
Rts2ScriptElementWaitAcquire::defnextCommand (Rts2DevClient * client,
					      Rts2Command ** new_command,
					      char
					      new_device[DEVICE_NAME_SIZE])
{
  return NEXT_COMMAND_WAIT_ACQUSITION;
}

Rts2ScriptElementMirror::Rts2ScriptElementMirror (Rts2Script * in_script, char *in_mirror_name, int in_mirror_pos):Rts2ScriptElement
  (in_script)
{
  mirror_pos = in_mirror_pos;
  mirror_name = new char[strlen (in_mirror_name) + 1];
  strcpy (mirror_name, in_mirror_name);
}

Rts2ScriptElementMirror::~Rts2ScriptElementMirror ()
{
  delete[]mirror_name;
}

void
Rts2ScriptElementMirror::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_MIRROR_FINISH:
      mirror_pos = -2;
      break;
    }
  Rts2ScriptElement::postEvent (event);
}

int
Rts2ScriptElementMirror::defnextCommand (Rts2DevClient * client,
					 Rts2Command ** new_command,
					 char new_device[DEVICE_NAME_SIZE])
{
  if (mirror_pos == -1)
    return NEXT_COMMAND_WAIT_MIRROR;
  if (mirror_pos < 0)		// job finished..
    return NEXT_COMMAND_NEXT;
  // send signal..if at least one mirror will get it, wait for it, otherwise end imediately
  script->getMaster ()->
    postEvent (new Rts2Event (EVENT_MIRROR_SET, (void *) this));
  // somebody pick that job..
  if (mirror_pos == -1)
    {
      return NEXT_COMMAND_WAIT_MIRROR;
    }
  // nobody cared, find another..
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementMirror::defnextCommand nobody cared about mirror %s",
	  mirror_name);
  return NEXT_COMMAND_NEXT;
}

Rts2ScriptElementPhotometer::Rts2ScriptElementPhotometer (Rts2Script * in_script, int in_filter, float in_exposure, int in_count):Rts2ScriptElement
  (in_script)
{
  filter = in_filter;
  exposure = in_exposure;
  count = in_count;
}

int
Rts2ScriptElementPhotometer::nextCommand (Rts2DevClientPhot * phot,
					  Rts2Command ** new_command,
					  char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandIntegrate (phot, filter, exposure, count);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementSendSignal::Rts2ScriptElementSendSignal (Rts2Script * in_script, int in_sig):Rts2ScriptElement
  (in_script)
{
  sig = in_sig;
}

Rts2ScriptElementSendSignal::~Rts2ScriptElementSendSignal (void)
{
  script->getMaster ()->
    postEvent (new Rts2Event (EVENT_SIGNAL, (void *) &sig));
}

int
Rts2ScriptElementSendSignal::defnextCommand (Rts2DevClient * client,
					     Rts2Command ** new_command,
					     char
					     new_device[DEVICE_NAME_SIZE])
{
  // we will do job required for us in destructor..
  return NEXT_COMMAND_NEXT;
}

Rts2ScriptElementWaitSignal::Rts2ScriptElementWaitSignal (Rts2Script * in_script, int in_sig):Rts2ScriptElement
  (in_script)
{
  sig = in_sig;
}

int
Rts2ScriptElementWaitSignal::defnextCommand (Rts2DevClient * client,
					     Rts2Command ** new_command,
					     char
					     new_device[DEVICE_NAME_SIZE])
{
  if (sig == -1)
    return NEXT_COMMAND_NEXT;
  return NEXT_COMMAND_WAIT_SIGNAL;
}

int
Rts2ScriptElementWaitSignal::waitForSignal (int in_sig)
{
  if (sig == in_sig)
    {
      sig = -1;
      return 1;
    }
  return 0;
}
