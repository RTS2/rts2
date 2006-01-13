#include "rts2scriptelement.h"

#include "../utils/rts2config.h"
#include "../writers/rts2imagedb.h"
#include "../writers/rts2devclifoc.h"

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

Rts2ScriptElementBinning::Rts2ScriptElementBinning (Rts2Script * in_script, int in_bin):Rts2ScriptElement
  (in_script)
{
  bin = in_bin;
}

int
Rts2ScriptElementBinning::nextCommand (Rts2DevClientCamera * camera,
				       Rts2Command ** new_command,
				       char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandBinning (camera, bin, bin);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementBox::Rts2ScriptElementBox (Rts2Script * in_script, int in_x, int in_y, int in_w, int in_h):Rts2ScriptElement
  (in_script)
{
  x = in_x;
  y = in_y;
  w = in_w;
  h = in_h;
}

int
Rts2ScriptElementBox::nextCommand (Rts2DevClientCamera * camera,
				   Rts2Command ** new_command,
				   char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandBox (camera, 0, x, y, w, h);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementCenter::Rts2ScriptElementCenter (Rts2Script * in_script, int in_w, int in_h):Rts2ScriptElement
  (in_script)
{
  w = in_w;
  h = in_h;
}

int
Rts2ScriptElementCenter::nextCommand (Rts2DevClientCamera * camera,
				      Rts2Command ** new_command,
				      char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandCenter (camera, 0, w, h);
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
  *new_command = new Rts2CommandFilter (camera, filter);
  getDevice (new_device);
  return 0;
}

int
Rts2ScriptElementFilter::nextCommand (Rts2DevClientPhot * phot,
				      Rts2Command ** new_command,
				      char new_device[DEVICE_NAME_SIZE])
{
  *new_command = new Rts2CommandFilter (phot, filter);
  getDevice (new_device);
  return 0;
}

Rts2ScriptElementAcquire::Rts2ScriptElementAcquire (Rts2Script * in_script, double in_precision, float in_expTime):Rts2ScriptElement
  (in_script)
{
  reqPrecision = in_precision;
  lastPrecision = nan ("f");
  expTime = in_expTime;
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
  AcquireQuery *ac;
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
    case EVENT_ACQUIRE_QUERY:
      ac = (AcquireQuery *) event->getArg ();
      ac->count++;
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
    default:
      break;
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
  Rts2ConnImgProcess *processor;

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
  // save image before processing..
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
      processingState = WAITING_ASTROMETRY;
      script->getMaster ()->postEvent (new Rts2Event (EVENT_ACQUIRE_WAIT));
    }
  return 0;
}

void
Rts2ScriptElementAcquire::cancelCommands ()
{
  processingState = FAILED;
}

Rts2ScriptElementWaitAcquire::Rts2ScriptElementWaitAcquire (Rts2Script *
							    in_script,
							    int in_tar_id):
Rts2ScriptElement (in_script)
{
  tar_id = in_tar_id;
}

int
Rts2ScriptElementWaitAcquire::defnextCommand (Rts2DevClient * client,
					      Rts2Command ** new_command,
					      char
					      new_device[DEVICE_NAME_SIZE])
{
  AcquireQuery ac = AcquireQuery (tar_id);
  // detect is somebody plans to run A command..
  script->getMaster ()->
    postEvent (new Rts2Event (EVENT_ACQUIRE_QUERY, (void *) &ac));
  syslog (LOG_DEBUG, "Rts2ScriptElementWaitAcquire::defnextCommand %i (%s)",
	  ac.count, script->getDefaultDevice ());
  if (ac.count)
    return NEXT_COMMAND_WAIT_ACQUSITION;
  return NEXT_COMMAND_NEXT;
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
  askedFor = false;
}

Rts2ScriptElementSendSignal::~Rts2ScriptElementSendSignal ()
{
  if (askedFor)
    script->getMaster ()->
      postEvent (new Rts2Event (EVENT_SIGNAL, (void *) &sig));
}

void
Rts2ScriptElementSendSignal::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SIGNAL_QUERY:
      if (*(int *) event->getArg () == sig)
	{
	  askedFor = true;
	  *(int *) event->getArg () = -1;
	}
      break;
    }
  Rts2ScriptElement::postEvent (event);
}

int
Rts2ScriptElementSendSignal::defnextCommand (Rts2DevClient * client,
					     Rts2Command ** new_command,
					     char
					     new_device[DEVICE_NAME_SIZE])
{
  // when some else script will wait reach point when it has to wait for
  // this signal, it will not wait as it will ask before enetring wait
  // if some script will send required signal
  script->getMaster ()->
    postEvent (new Rts2Event (EVENT_SIGNAL, (void *) &sig));
  askedFor = false;
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
  int ret;

  // not valid signal..
  if (sig == -1)
    return NEXT_COMMAND_NEXT;

  // nobody will send us a signal..end script
  ret = sig;
  script->getMaster ()->
    postEvent (new Rts2Event (EVENT_SIGNAL_QUERY, (void *) &ret));
  syslog (LOG_DEBUG, "Rts2ScriptElementWaitSignal::defnextCommand %i (%s)",
	  ret, script->getDefaultDevice ());
  if (ret != -1)
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

Rts2ScriptElementAcquireStar::Rts2ScriptElementAcquireStar (Rts2Script * in_script, int in_maxRetries, double in_precision, float in_expTime, double in_spiral_scale_ra, double in_spiral_scale_dec):
Rts2ScriptElementAcquire (in_script, in_precision, in_expTime)
{
  maxRetries = in_maxRetries;
  retries = 0;
  spiral_scale_ra = in_spiral_scale_ra;
  spiral_scale_dec = in_spiral_scale_dec;
  defaultImgProccess[0] = '\0';

  Rts2Config::instance ()->getString (in_script->getDefaultDevice (),
				      "sextractor", defaultImgProccess, 2000);
  spiral = new Rts2Spiral ();
}

Rts2ScriptElementAcquireStar::~Rts2ScriptElementAcquireStar (void)
{
  delete spiral;
}

void
Rts2ScriptElementAcquireStar::postEvent (Rts2Event * event)
{
  Rts2ConnFocus *focConn;
  Rts2Image *image;
  struct ln_equ_posn offset;
  int ret;
  short next_x, next_y;
  switch (event->getType ())
    {
    case EVENT_STAR_DATA:
      focConn = (Rts2ConnFocus *) event->getArg ();
      image = focConn->getImage ();
      // that was our image
      if (image->getObsId () == obsId && image->getImgId () == imgId)
	{
	  ret = getSource (image, offset.ra, offset.dec);
	  switch (ret)
	    {
	    case -1:
	      syslog (LOG_DEBUG,
		      "Rts2ScriptElementAcquireStar::postEvent EVENT_STAR_DATA failed (numStars: %i)",
		      image->sexResultNum);
	      if (retries >= maxRetries)
		{
		  processingState = FAILED;
		  break;
		}
	      retries++;
	      processingState = PRECISION_BAD;
	      // try some offset..
	      spiral->getNextStep (next_x, next_y);
	      offset.ra = spiral_scale_ra * next_x;
	      offset.dec = spiral_scale_dec * next_y;
	      script->getMaster ()->
		postEvent (new
			   Rts2Event (EVENT_ADD_FIXED_OFFSET,
				      (void *) &offset));
	      break;
	    case 0:
	      syslog (LOG_DEBUG,
		      "Rts2ScriptElementAcquireStar::offsets ra: %f dec: %f",
		      offset.ra, offset.dec);
	      processingState = PRECISION_OK;
	      break;
	    case 1:
	      syslog (LOG_DEBUG,
		      "Rts2ScriptElementAcquireStar::offsets ra: %f dec: %f",
		      offset.ra, offset.dec);
	      if (retries >= maxRetries)
		{
		  processingState = FAILED;
		  break;
		}
	      processingState = PRECISION_BAD;
	      script->getMaster ()->
		postEvent (new
			   Rts2Event (EVENT_ADD_FIXED_OFFSET,
				      (void *) &offset));
	      retries++;
	      break;
	    }
	  image->toAcquisition ();
	}
      break;
    }
  Rts2ScriptElementAcquire::postEvent (event);
}

int
Rts2ScriptElementAcquireStar::processImage (Rts2Image * image)
{
  int ret;
  Rts2ConnFocus *processor;
  if (processingState != WAITING_IMAGE)
    {
      syslog (LOG_ERR,
	      "Rts2ScriptElementAcquireStar::processImage invalid processingState: %i",
	      processingState);
      processingState = FAILED;
      return -1;
    }
  obsId = image->getObsId ();
  imgId = image->getImgId ();
  processor =
    new Rts2ConnFocus (script->getMaster (), image, defaultImgProccess,
		       EVENT_STAR_DATA);
  // save image before processing
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
      processingState = WAITING_ASTROMETRY;
      script->getMaster ()->postEvent (new Rts2Event (EVENT_ACQUIRE_WAIT));
    }
  return 0;
}

int
Rts2ScriptElementAcquireStar::getSource (Rts2Image * image, double &ra_offset,
					 double &dec_offset)
{
  int ret;
  double off_x, off_y;
  double sep;
  float flux;
  ret = image->getBrightestOffset (off_x, off_y, flux);
  if (ret || flux < 5000)
    return -1;
  ret = image->getOffset (off_x, off_y, ra_offset, dec_offset, sep);
  if (ret)
    return -1;
  ra_offset *= -1.0;
  dec_offset *= -1.0;
  if (sep < reqPrecision)
    return 0;
  return 1;
}

Rts2ScriptElementAcquireHam::Rts2ScriptElementAcquireHam (Rts2Script * in_script, int in_maxRetries, float in_expTime):Rts2ScriptElementAcquireStar (in_script, in_maxRetries, 0.05, in_expTime, 0.7,
			      0.3)
{
}

Rts2ScriptElementAcquireHam::~Rts2ScriptElementAcquireHam (void)
{
}

int
Rts2ScriptElementAcquireHam::getSource (Rts2Image * image, double &ra_off,
					double &dec_off)
{
  double ham_x, ham_y;
  double sep;
  int ret;
  ret = image->getHam (ham_x, ham_y);
  if (ret)
    return -1;			// continue HAM search..
  // change fixed offset
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementAcquireHam::getSource %lf %lf", ham_x, ham_y);
  ret = image->getOffset (ham_x, ham_y, ra_off, dec_off, sep);
  if (ret)
    return -1;
  syslog (LOG_DEBUG,
	  "Rts2ScriptElementAcquireHam::offsets ra: %f dec: %f",
	  ra_off, dec_off);
  if (sep < reqPrecision)
    return 0;
  return 1;
}

Rts2ScriptElementSearch::Rts2ScriptElementSearch (Rts2Script * in_script, double in_searchRadius, double in_searchSpeed):Rts2ScriptElement
  (in_script)
{
  searchRadius = in_searchRadius;
  searchSpeed = in_searchSpeed;
  processingState = NEED_SEARCH;
}

void
Rts2ScriptElementSearch::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_TEL_SEARCH_SUCCESS:
      processingState = SEARCH_OK;
      break;
    case EVENT_TEL_SEARCH_END:
    case EVENT_TEL_SEARCH_STOP:
      processingState = SEARCH_FAILED;
      break;
    }
  Rts2ScriptElement::postEvent (event);
}

int
Rts2ScriptElementSearch::nextCommand (Rts2DevClientPhot * phot,
				      Rts2Command ** new_command,
				      char new_device[DEVICE_NAME_SIZE])
{
  switch (processingState)
    {
    case NEED_SEARCH:
      script->getMaster ()->
	postEvent (new Rts2Event (EVENT_TEL_SEARCH_START, (void *) this));
      if (!isnan (searchRadius))
	{
	  // no one pick our task..lets move to next command
	  return NEXT_COMMAND_NEXT;
	}
      processingState = SEARCHING;
    case SEARCHING:
      return NEXT_COMMAND_WAIT_SEARCH;
    case SEARCH_OK:
      return NEXT_COMMAND_NEXT;
    case SEARCH_FAILED:
      // que end of script
      *new_command = new Rts2CommandFilter (phot, 0);
      getDevice (new_device);
      // filterMoveCommand will result in call of either
      // filterMoveFailed (calls now deleteScript) or filterMoveEnd
      // (calls next command)
      processingState = SEARCH_FAILED2;
      return NEXT_COMMAND_KEEP;
    case SEARCH_FAILED2:
      // so we get there..
      return NEXT_COMMAND_END_SCRIPT;
    }
  return 0;
}
