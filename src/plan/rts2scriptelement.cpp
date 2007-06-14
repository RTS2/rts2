#include <iterator>

#include "rts2devscript.h"
#include "rts2script.h"
#include "rts2connimgprocess.h"

#include "../utils/rts2config.h"
#include "../writers/rts2image.h"
#include "../writers/rts2devclifoc.h"

Rts2ScriptElement::Rts2ScriptElement (Rts2Script * in_script)
{
  script = in_script;
  startPos = script->getParsedStartPos ();
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

int
Rts2ScriptElement::getStartPos ()
{
  return startPos;
}

int
Rts2ScriptElement::getLen ()
{
  return len;
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
    new Rts2CommandExposure (script->getMaster (), camera, 0, EXP_LIGHT,
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
    new Rts2CommandExposure (script->getMaster (), camera, 0, EXP_DARK,
			     expTime);
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
  setChangeRaDec (in_ra, in_dec);
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

Rts2ScriptElementWaitAcquire::Rts2ScriptElementWaitAcquire (Rts2Script * in_script, int in_tar_id):
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
//#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG)
    << "Rts2ScriptElementWaitAcquire::defnextCommand " << ac.count
    << " (" << script->getDefaultDevice () << ") " << tar_id << sendLog;
//#endif
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
  logStream (MESSAGE_DEBUG)
    << "Rts2ScriptElementMirror::defnextCommand nobody cared about mirror "
    << mirror_name << sendLog;
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
#ifdef DEBUG_EXTRA
  logStream (MESSAGE_DEBUG) << "Rts2ScriptElementWaitSignal::defnextCommand "
    << ret << " (" << script->getDefaultDevice () << ")" << sendLog;
#endif
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

Rts2ScriptElementChangeValue::Rts2ScriptElementChangeValue (Rts2Script * in_script, const char *new_device, const char *chng_str):Rts2ScriptElement
  (in_script)
{
  deviceName = new char[strlen (new_device) + 1];
  strcpy (deviceName, new_device);
  std::string chng_s = std::string (chng_str);
  op = '\0';
  int
    op_end = 0;
  int
    i;
  std::string::iterator iter;
  for (iter = chng_s.begin (), i = 0; iter != chng_s.end (); iter++, i++)
    {
      char
	ch = *iter;
      if (ch == '+' || ch == '-' || ch == '=')
	{
	  if (op == '\0')
	    {
	      valName = chng_s.substr (0, i);
	      op = ch;
	    }
	  op_end = i;
	}
    }
  if (op == '\0')
    return;
  operand = chng_s.substr (op_end + 1);
}

Rts2ScriptElementChangeValue::~Rts2ScriptElementChangeValue (void)
{
  delete[]deviceName;
}

void
Rts2ScriptElementChangeValue::getDevice (char new_device[DEVICE_NAME_SIZE])
{
  strcpy (new_device, deviceName);
}

int
Rts2ScriptElementChangeValue::defnextCommand (Rts2DevClient * client,
					      Rts2Command ** new_command,
					      char
					      new_device[DEVICE_NAME_SIZE])
{
  if (op == '\0' || operand.size () == 0)
    return -1;
  *new_command = new Rts2CommandChangeValue (client, valName, op, operand);
  getDevice (new_device);
  return 0;
}
